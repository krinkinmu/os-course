#include "kmem_cache.h"
#include "kernel.h"
#include "string.h"
#include "error.h"
#include "vfs.h"


static struct kmem_cache *fs_entry_cache;
static struct fs_entry fs_root_entry;
static struct fs_node fs_root_node;

static LIST_HEAD(fs_mounts);
static LIST_HEAD(fs_types);


static struct fs_entry *vfs_entry_create(const char *name)
{
	struct fs_entry *entry = kmem_cache_alloc(fs_entry_cache);

	if (!entry)
		return 0;

	memset(entry, 0, sizeof(*entry));
	spinlock_init(&entry->lock);
	strcpy(entry->name, name);
	entry->refcount = 1;
	return entry;
}

static void __vfs_entry_detach(struct fs_entry *entry, struct fs_entry *dir)
{
	if (entry->cached) {
		rb_erase(&entry->link, &dir->children);
		entry->cached = false;
	}
}

void vfs_entry_detach(struct fs_entry *entry)
{
	struct fs_entry *parent = entry->parent;
	const bool enabled = spin_lock_irqsave(&parent->lock);
	__vfs_entry_detach(entry, parent);
	spin_unlock_irqrestore(&parent->lock, enabled);
}

struct fs_entry *vfs_entry_get(struct fs_entry *entry)
{
	if (entry) {	
		const bool enabled = spin_lock_irqsave(&entry->lock);
		++entry->refcount;
		spin_unlock_irqrestore(&entry->lock, enabled);
	}
	return entry;
}

void vfs_entry_put(struct fs_entry *entry)
{
	struct fs_entry *dir = entry->parent;

	const bool enabled = spin_lock_irqsave(&entry->lock);
	const int refcount = --entry->refcount;
	spin_unlock_irqrestore(&entry->lock, enabled);

	if (refcount != 0)
		return;

	spin_lock(&dir->lock);
	spin_lock(&entry->lock);

	if (entry->refcount) {
		spin_unlock_irqrestore(&entry->lock, false);
		spin_unlock_irqrestore(&dir->lock, enabled);
		return;
	}
	__vfs_entry_detach(entry, dir);

	spin_unlock_irqrestore(&entry->lock, false);
	spin_unlock_irqrestore(&dir->lock, enabled);

	vfs_entry_put(entry->parent);
	vfs_node_put(entry->node);
	kmem_cache_free(fs_entry_cache, entry);
}

void vfs_node_destroy(struct fs_node *node)
{ node->ops->release(node); }


static struct fs_entry *vfs_lookup(struct fs_entry *dir, const char *name,
			bool create, int *rc)
{
	struct fs_node *node = dir->node;
	struct rb_node **plink = &dir->children.root;
	struct rb_node *parent = 0;

	if (!node->ops->lookup) {
		*rc = -ENOTSUP;
		return 0;
	}

	const bool enabled = spin_lock_irqsave(&dir->lock);

	while (*plink) {
		struct fs_entry *entry = TREE_ENTRY(*plink, struct fs_entry,
			link);

		const int cmp = strcmp(entry->name, name);

		if (!cmp) {
			*rc = 0;
			vfs_entry_get(entry);
			spin_unlock_irqrestore(&dir->lock, enabled);
			return entry;
		}

		parent = *plink;
		if (cmp < 0)
			plink = &parent->right;
		else
			plink = &parent->left;
	}

	struct fs_entry *entry = vfs_entry_create(name);

	if (!entry) {
		spin_unlock_irqrestore(&dir->lock, enabled);
		*rc = -ENOMEM;
		return 0;
	}

	rb_link(&entry->link, parent, plink);
	rb_insert(&entry->link, &dir->children);
	entry->parent = vfs_entry_get(dir);
	entry->cached = true;
	spin_unlock_irqrestore(&dir->lock, enabled);

	*rc = node->ops->lookup(node, entry);
	if (*rc && !create) {
		vfs_entry_put(entry);
		return 0;
	}

	return entry;
}


struct vfs_walk_data {
	struct fs_entry *entry;
	char next[MAX_PATH_LEN];
	const char *tail;
};

static const char *vfs_path_skip_sep(const char *path)
{
	while (*path && *path == '/')
		++path;
	return path;
}

static const char *vfs_path_next_entry(char *next, const char *full)
{
	while (*full && *full != '/')
		*next++ = *full++;
	*next = '\0';
	return vfs_path_skip_sep(full);
}

static void vfs_walk_start(struct vfs_walk_data *data, const char *path)
{
	data->entry = vfs_entry_get(&fs_root_entry);
	data->tail = vfs_path_next_entry(data->next, vfs_path_skip_sep(path));
}

static void vfs_walk_stop(struct vfs_walk_data *data)
{
	vfs_entry_put(data->entry);
	memset(data, 0, sizeof(*data));
}

static int vfs_walk(struct vfs_walk_data *data)
{
	if (!strcmp(data->next, ""))
		return -ENOENT;

	if (!strcmp(data->next, ".")) {
		data->tail = vfs_path_next_entry(data->next, data->tail);
		return 0;
	}

	if (!strcmp(data->next, "..")) {
		struct fs_entry *parent = vfs_entry_get(data->entry->parent);

		vfs_entry_put(data->entry);
		data->entry = parent;
		data->tail = vfs_path_next_entry(data->next, data->tail);
		return 0;
	}

	struct fs_entry *entry;
	int rc;

	mutex_lock(&data->entry->node->mux);
	entry = vfs_lookup(data->entry, data->next, false, &rc);
	mutex_unlock(&data->entry->node->mux);

	if (!entry)
		return rc;

	vfs_entry_put(data->entry);
	data->entry = entry;
	data->tail = vfs_path_next_entry(data->next, data->tail);
	return 0;
}

static int vfs_walk_parent(struct vfs_walk_data *data)
{
	while (strcmp(data->tail, "")) {
		const int rc = vfs_walk(data);

		if (rc)
			return rc;
	}
	return 0;
}

static int vfs_walk_all(struct vfs_walk_data *data)
{
	while (strcmp(data->next, "")) {
		const int rc = vfs_walk(data);

		if (rc)
			return rc;
	}
	return 0;
}

static struct fs_entry *vfs_resolve_path(const char *path, int *rc)
{
	struct vfs_walk_data wd;

	vfs_walk_start(&wd, path);
	*rc = vfs_walk_all(&wd);
	if (*rc) {
		vfs_walk_stop(&wd);
		return 0;
	}

	struct fs_entry *entry = vfs_entry_get(wd.entry);

	vfs_walk_stop(&wd);
	*rc = 0;
	return entry;
}

static int vfs_root_lookup(struct fs_node *root, struct fs_entry *entry)
{
	struct list_head *head = &fs_mounts;
	struct list_head *ptr = head->next;

	(void) root;

	for (; ptr != head; ptr = ptr->next) {
		struct fs_mount *mount = LIST_ENTRY(ptr, struct fs_mount, link);
		const char *name = mount->name;

		if (!strcmp(name, entry->name)) {
			entry->node = vfs_node_get(mount->root);
			return 0;
		}
	}
	return -ENOENT;
}

static int vfs_root_iterate(struct fs_file *root, struct dir_iter_ctx *ctx)
{
	struct list_head *head = &fs_mounts;
	struct list_head *ptr = head->next;

	(void) root;

	for (int i = 0; i != ctx->offset && ptr != head; ptr = ptr->next);

	while (ptr != head) {
		struct fs_mount *mount = LIST_ENTRY(ptr, struct fs_mount, link);
		const char *name = mount->name;

		if (ctx->emit(ctx, name, strlen(name))) {
			++ctx->offset;
			break;
		}
	}
	return 0;
}

static struct fs_node_ops fs_root_node_ops = {
	.lookup = &vfs_root_lookup
};

static struct fs_file_ops fs_root_file_ops = {
	.iterate = &vfs_root_iterate
};


static void vfs_file_cleanup(struct fs_file *file)
{
	if (file->entry)
		vfs_entry_put(file->entry);

	file->entry = 0;
	file->node = 0;
	file->ops = 0;
	file->offset = 0;
}

static int vfs_file_open(const char *name, struct fs_file *file, bool create)
{
	struct vfs_walk_data wd;
	int rc;

	vfs_walk_start(&wd, name);
	rc = vfs_walk_parent(&wd);
	if (rc) {
		vfs_walk_stop(&wd);
		return rc;
	}

	struct fs_node *dir = wd.entry->node;
	struct fs_entry *entry;

	if (!dir->ops || !dir->ops->lookup) {
		vfs_walk_stop(&wd);
		return -ENOTSUP;
	}
	
	mutex_lock(&dir->mux);
	entry = vfs_lookup(wd.entry, wd.next, create, &rc);
	if (create && entry && !entry->node) {
		if (!dir->ops->create)
			rc = -ENOTSUP;
		else
			rc = dir->ops->create(dir, entry);
	}
	mutex_unlock(&dir->mux);
	vfs_walk_stop(&wd);

	if (rc) {
		vfs_entry_put(entry);
		return rc;
	}

	file->entry = entry;
	file->node = entry->node;
	file->ops = entry->node->fops;
	file->offset = 0;

	if (file->ops && file->ops->open) {
		rc = file->ops->open(file);
		if (rc)
			vfs_file_cleanup(file);
	}

	return rc;
}

int vfs_create(const char *name, struct fs_file *file)
{ return vfs_file_open(name, file, true); }

int vfs_open(const char *name, struct fs_file *file)
{ return vfs_file_open(name, file, false); }

int vfs_release(struct fs_file *file)
{
	if (file->ops && file->ops->release) {
		const int ret = file->ops->release(file);

		if (ret)
			return ret;
	}

	vfs_file_cleanup(file);
	return 0;
}

int vfs_read(struct fs_file *file, char *buffer, size_t size)
{
	if (file->ops && file->ops->read)
		return file->ops->read(file, buffer, size);
	return -ENOTSUP;
}

int vfs_write(struct fs_file *file, const char *buffer, size_t size)
{
	if (file->ops && file->ops->write)
		return file->ops->write(file, buffer, size);
	return -ENOTSUP;
}

int vfs_seek_default(struct fs_file *file, int offset, int whence)
{
	switch (whence) {
	case FSS_CUR:
		offset += file->offset;
		break;
	case FSS_END:
		offset += file->node->size;
		break;
	}

	offset = MAX(offset, 0);
	file->offset = offset;
	return offset;
}

int vfs_seek(struct fs_file *file, int offset, int whence)
{
	if (file->ops && file->ops->seek)
		return file->ops->seek(file, offset, whence);
	return -ENOTSUP;
}


struct readdir_ctx {
	struct dir_iter_ctx ctx;
	struct dirent *entries;
	size_t count;
	size_t pos;
};

static bool readdir_next_entry(struct dir_iter_ctx *ctx, const char *name,
			size_t len)
{
	struct readdir_ctx *rdctx = (struct readdir_ctx *)ctx;
	struct dirent *entries = rdctx->entries;
	size_t count = rdctx->count;
	size_t pos = rdctx->pos;

	if (pos != count) {
		memset(entries[pos].name, 0, MAX_PATH_LEN);
		strncpy(entries[pos].name, name, MINU(MAX_PATH_LEN - 1, len));
		++rdctx->pos;
		return true;
	}
	return false;
}

int vfs_readdir(struct fs_file *file, struct dirent *entries, size_t count)
{
	if (file->ops && file->ops->iterate) {
		struct readdir_ctx ctx;

		ctx.ctx.emit = &readdir_next_entry;
		ctx.ctx.offset = file->offset;
		ctx.entries = entries;
		ctx.count = count;
		ctx.pos = 0;

		const int rc = file->ops->iterate(file, &ctx.ctx);

		if (file->offset != ctx.ctx.offset) {
			const int ret = ctx.ctx.offset - file->offset;

			file->offset = ctx.ctx.offset;
			return ret;
		}
		return rc;
	}

	return -ENOTSUP;
}

int vfs_link(const char *oldname, const char *newname)
{
	int rc;
	struct fs_entry *oldentry = vfs_resolve_path(oldname, &rc);

	if (!oldentry)
		return rc;

	struct vfs_walk_data wd;

	vfs_walk_start(&wd, newname);
	rc = vfs_walk_parent(&wd);
	if (rc) {
		vfs_walk_stop(&wd);
		vfs_entry_put(oldentry);
		return rc;
	}

	struct fs_entry *dir = wd.entry;
	struct fs_node *node = dir->node;

	if (!node->ops->lookup || !node->ops->link) {
		vfs_walk_stop(&wd);
		vfs_entry_put(oldentry);
		return -ENOTSUP;
	}
	
	mutex_lock(&node->mux);
	struct fs_entry *newentry = vfs_lookup(dir, wd.next, true, &rc);

	if (!newentry) {
		mutex_unlock(&node->mux);
		vfs_walk_stop(&wd);
		vfs_entry_put(oldentry);
		return rc;
	}

	if (newentry->node) {
		mutex_unlock(&node->mux);
		vfs_walk_stop(&wd);
		vfs_entry_put(oldentry);
		vfs_entry_put(newentry);
		return -EEXIST;
	}

	rc = node->ops->link(oldentry, node, newentry);
	mutex_unlock(&node->mux);
	vfs_walk_stop(&wd);
	vfs_entry_put(oldentry);
	vfs_entry_put(newentry);
	return rc;
}

int vfs_unlink(const char *name)
{
	int rc;
	struct fs_entry *entry = vfs_resolve_path(name, &rc);

	if (!entry)
		return rc;

	struct fs_entry *dir = entry->parent;
	struct fs_node *node = dir->node;

	if (!node->ops->unlink) {
		vfs_entry_put(entry);
		return -ENOTSUP;
	}

	mutex_lock(&node->mux);
	rc = node->ops->unlink(node, entry);
	mutex_unlock(&node->mux);
	vfs_entry_put(entry);
	return rc;
}

int vfs_mkdir(const char *name)
{
	struct vfs_walk_data wd;
	int rc;

	vfs_walk_start(&wd, name);
	rc = vfs_walk_parent(&wd);
	if (rc) {
		vfs_walk_stop(&wd);
		return rc;
	}

	struct fs_entry *dir = wd.entry;
	struct fs_node *node = dir->node;

	if (!node->ops->lookup || !node->ops->mkdir) {
		vfs_walk_stop(&wd);
		return -ENOTSUP;
	}

	mutex_lock(&node->mux);
	struct fs_entry *entry = vfs_lookup(dir, wd.next, true, &rc);

	if (!entry) {
		mutex_unlock(&node->mux);
		vfs_walk_stop(&wd);
		return rc;
	}

	if (entry->node) {
		mutex_unlock(&node->mux);
		vfs_walk_stop(&wd);
		vfs_entry_put(entry);
		return -EEXIST;
	}

	rc = node->ops->mkdir(node, entry);
	mutex_unlock(&node->mux);
	vfs_walk_stop(&wd);
	vfs_entry_put(entry);
	return rc;
}

int vfs_rmdir(const char *name)
{
	int rc;
	struct fs_entry *entry = vfs_resolve_path(name, &rc);

	if (!entry)
		return rc;

	struct fs_entry *parent = entry->parent;
	struct fs_node *node = parent->node;

	if (!node->ops->rmdir) {
		vfs_entry_put(entry);
		return -ENOTSUP;
	}

	mutex_lock(&node->mux);
	rc = node->ops->rmdir(node, entry);
	mutex_unlock(&node->mux);
	vfs_entry_put(entry);
	return rc;
}

static struct fs_type *__vfs_lookup_filesystem(const char *name)
{
	struct list_head *head = &fs_types;
	struct list_head *ptr = head->next;

	for (; ptr != head; ptr = ptr->next) {
		struct fs_type *fs = LIST_ENTRY(ptr, struct fs_type, link);

		if (!strcmp(fs->name, name))
			return fs;
	}
	return 0;
}

static struct fs_mount *__vfs_lookup_mount(const char *name)
{
	struct list_head *head = &fs_mounts;
	struct list_head *ptr = head->next;

	for (; ptr != head; ptr = ptr->next) {
		struct fs_mount *mnt = LIST_ENTRY(ptr, struct fs_mount, link);

		if (!strcmp(mnt->name, name))
			return mnt;
	}
	return 0;
}

static struct fs_mount *__vfs_mount_create(struct fs_type *fs, const char *name)
{
	struct fs_mount *mnt;

	if (fs->ops->alloc)
		mnt = fs->ops->alloc();
	else
		mnt = kmem_alloc(sizeof(*mnt));

	if (mnt) {
		memset(mnt, 0, sizeof(*mnt));
		strncpy(mnt->name, name, MAX_PATH_LEN - 1);
		mnt->fs = fs;
	}

	return mnt;
}

static void __vfs_mount_destroy(struct fs_type *fs, struct fs_mount *mnt)
{
	if (fs->ops->free)
		fs->ops->free(mnt);
	else
		kmem_free(mnt);
}

int register_filesystem(struct fs_type *type)
{
	mutex_lock(&fs_root_node.mux);
	if (__vfs_lookup_filesystem(type->name)) {
		mutex_unlock(&fs_root_node.mux);
		return -EEXIST;
	}
	list_add_tail(&type->link, &fs_types);
	mutex_unlock(&fs_root_node.mux);
	return 0;
}

int unregister_filesystem(struct fs_type *type)
{
	mutex_lock(&fs_root_node.mux);
	if (type->refcount) {
		mutex_unlock(&fs_root_node.mux);
		return -EBUSY;
	}
	list_del(&type->link);
	mutex_unlock(&fs_root_node.mux);
	return 0;
}

static int __vfs_mount(const char *fs_name, const char *mount, const void *data,
			size_t size)
{
	struct fs_type *fs = __vfs_lookup_filesystem(fs_name);

	if (!fs)
		return -ENOENT;

	if (__vfs_lookup_mount(mount))
		return -EEXIST;

	struct fs_mount *mnt = __vfs_mount_create(fs, mount);

	if (!mnt)
		return -ENOMEM;

	const int ret = fs->ops->mount(mnt, data, size);

	if (ret) {
		__vfs_mount_destroy(mnt->fs, mnt);
		return ret;
	}

	list_add_tail(&mnt->link, &fs_mounts);
	++fs->refcount;
	return ret;
}

int vfs_mount(const char *fs_name, const char *mount, const void *data,
			size_t size)
{
	mutex_lock(&fs_root_node.mux);
	const int rc = __vfs_mount(fs_name, mount, data, size);
	mutex_unlock(&fs_root_node.mux);
	return rc;
}

static int __vfs_umount(const char *mount)
{
	struct fs_mount *mnt = __vfs_lookup_mount(mount);

	if (!mnt)
		return -ENOENT;

	list_del(&mnt->link);
	mnt->fs->ops->umount(mnt);
	--mnt->fs->refcount;
	__vfs_mount_destroy(mnt->fs, mnt);
	return 0;
}

int vfs_umount(const char *mount)
{
	mutex_lock(&fs_root_node.mux);
	const int rc = __vfs_umount(mount);
	mutex_unlock(&fs_root_node.mux);
	return rc;	
}

void setup_vfs(void)
{
	DBG_ASSERT((fs_entry_cache = KMEM_CACHE(struct fs_entry)) != 0);

	mutex_init(&fs_root_node.mux);
	fs_root_node.ops = &fs_root_node_ops;
	fs_root_node.fops = &fs_root_file_ops;

	fs_root_entry.node = &fs_root_node;
	fs_root_entry.parent = &fs_root_entry;
	fs_root_entry.refcount = 1;
}

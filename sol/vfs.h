#ifndef __VFS_H__
#define __VFS_H__

#include <stdbool.h>
#include <stddef.h>

#include "locking.h"
#include "rbtree.h"
#include "stdio.h"
#include "list.h"

#define MAX_PATH_LEN 256

struct fs_mount;
struct fs_entry;
struct fs_node;
struct fs_file;
struct dir_iter_ctx;


struct fs_type_ops {
	struct fs_mount *(*alloc)(void);
	void (*free)(struct fs_mount *);
	int (*mount)(struct fs_mount *, const void *, size_t);
	void (*umount)(struct fs_mount *);
};

struct fs_node_ops {
	int (*create)(struct fs_node *, struct fs_entry *);
	int (*link)(struct fs_entry *, struct fs_node *, struct fs_entry *);
	int (*unlink)(struct fs_node *, struct fs_entry *);
	int (*mkdir)(struct fs_node *, struct fs_entry *);
	int (*rmdir)(struct fs_node *, struct fs_entry *);
	int (*lookup)(struct fs_node *, struct fs_entry *);
	void (*release)(struct fs_node *);
};

enum fs_seek_base {
	FSS_SET,
	FSS_CUR,
	FSS_END
};

struct fs_file_ops {
	int (*open)(struct fs_file *);
	int (*release)(struct fs_file *);
	int (*read)(struct fs_file *, char *, size_t);
	int (*write)(struct fs_file *, const char *, size_t);
	int (*seek)(struct fs_file *, int off, int whence);
	int (*iterate)(struct fs_file *, struct dir_iter_ctx *);
};

/**
 * Filesystem driver provides struct fs_type to the kernel
 */
struct fs_type {
	struct list_head link;
	const char *name;
	struct fs_type_ops *ops;
	int refcount;
};

/**
 * struct fs_mount represents mount point
 */
struct fs_mount {
	struct list_head link;
	struct fs_type *fs;
	struct fs_node *root;
	char name[MAX_PATH_LEN];
};

/**
 * struct fs_node is analogue of disk filesystem inode, for every
 * filesystem entity (file, directory, etc..) there is an fs_node
 */
struct fs_node {
	struct mutex mux; // protects against concurrent dir ops
	struct fs_node_ops *ops;
	struct fs_file_ops *fops;
	struct spinlock lock; // protects refcount access, and probably size
	int refcount;
	int size;
};

void vfs_node_destroy(struct fs_node *node);

static inline void vfs_node_init(struct fs_node *node)
{
	mutex_init(&node->mux);
	spinlock_init(&node->lock);
	node->refcount = 1;
}

static inline struct fs_node *vfs_node_get(struct fs_node *node)
{
	if (node) {
		const bool enabled = spin_lock_irqsave(&node->lock);
		++node->refcount;
		spin_unlock_irqrestore(&node->lock, enabled);
	}
	return node;
}

static inline void vfs_node_put(struct fs_node *node)
{
	if (!node)
		return;

	const bool enabled = spin_lock_irqsave(&node->lock);
	const int refcount = --node->refcount;
	spin_unlock_irqrestore(&node->lock, enabled);

	if (!refcount)
		vfs_node_destroy(node);
}

/**
 * struct fs_entry represents chunk of file path, for example for
 * a path /usr/bin/python there might be three fs_entry structures:
 * usr, bin and python. So fs_entry structures reperesents a tree
 * of dirs.
 */
struct fs_entry {
	struct rb_node link; // protected by parent->node->mux
	struct rb_tree children; // protected by node->mux
	struct fs_entry *parent;
	struct fs_node *node;
	char name[MAX_PATH_LEN];
	bool cached; // protected by parent->node->mux
	struct spinlock lock; // protects refcount access
	int refcount;
};

void vfs_entry_detach(struct fs_entry *entry);
struct fs_entry *vfs_entry_get(struct fs_entry *entry);
void vfs_entry_put(struct fs_entry *entry);


/**
 * struct fs_file is so called file description
 */
struct fs_file {
	struct fs_entry *entry;
	struct fs_node *node;
	struct fs_file_ops *ops;
	int offset;
};

/**
 * Just an utility structure for directory iteration.
 */
struct dir_iter_ctx {
	bool (*emit)(struct dir_iter_ctx *, const char *, size_t);
	int offset;
};

struct dirent {
	char name[MAX_PATH_LEN];
};

int vfs_create(const char *name, struct fs_file *file);
int vfs_open(const char *name, struct fs_file *file);
int vfs_release(struct fs_file *file);
int vfs_read(struct fs_file *file, char *buffer, size_t size);
int vfs_write(struct fs_file *file, const char *buffer, size_t size);

int vfs_seek_default(struct fs_file *file, int off, int whence);
int vfs_seek(struct fs_file *file, int off, int whence);

int vfs_readdir(struct fs_file *file, struct dirent *entries, size_t count);

int vfs_link(const char *oldname, const char *newname);
int vfs_unlink(const char *name);
int vfs_mkdir(const char *name);
int vfs_rmdir(const char *name);

int register_filesystem(struct fs_type *type);
int unregister_filesystem(struct fs_type *type);
int vfs_mount(const char *fs_name, const char *mount, const void *data,
			size_t size);
int vfs_umount(const char *mount);
void setup_vfs(void);

#endif /*__VFS_H__*/

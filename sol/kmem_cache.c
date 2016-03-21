#include "kmem_cache.h"
#include "memory.h"
#include "stdio.h"
#include "list.h"


struct kmem_slab_ops {
	void *(*alloc)(struct kmem_cache *, struct kmem_slab *);
	void (*free)(struct kmem_cache *, struct kmem_slab *, void *);
};

struct kmem_slab {
	struct list_head link;
	const struct kmem_slab_ops *ops;
	struct kmem_cache *cache;
	struct page *pages;
	size_t free;
	size_t total;
};

struct kmem_cache_ops {
	struct kmem_slab *(*create)(struct kmem_cache *, struct page *pages);
	void (*destroy)(struct kmem_cache *, struct kmem_slab *);
};

struct kmem_cache {
	const struct kmem_cache_ops *ops;
	struct list_head part_list;
	struct list_head free_list;
	struct list_head full_list;
	size_t object_align;
	size_t object_size;
	int order;
};


static void kmem_cache_init(struct kmem_cache *cache)
{
	list_init(&cache->free_list);
	list_init(&cache->part_list);
	list_init(&cache->full_list);
}

static bool kmem_cache_grow(struct kmem_cache *cache)
{
	const pfn_t pfs = (pfn_t)1 << cache->order;
	struct page *pages = alloc_pages(cache->order);

	if (!pages)
		return false;

	struct kmem_slab *slab = cache->ops->create(cache, pages);

	if (!slab) {
		free_pages(pages, cache->order);
		return false;
	}

	for (pfn_t i = 0; i != pfs; ++i)
		pages[i].u.slab = slab;

	slab->cache = cache;
	slab->pages = pages;

	list_add(&slab->link, &cache->free_list);

	return true;
}

void kmem_cache_reap(struct kmem_cache *cache)
{
	LIST_HEAD(list);

	list_splice(&cache->free_list, &list);

	for (struct list_head *ptr = list.next; ptr != &list;) {
		struct kmem_slab *slab =
			LIST_ENTRY(ptr, struct kmem_slab, link);
		struct page *pages = slab->pages;

		ptr = ptr->next;
		if (cache->ops->destroy)
			cache->ops->destroy(cache, slab);

		free_pages(pages, cache->order);	
	}
}

void *kmem_cache_alloc(struct kmem_cache *cache)
{
	if (!list_empty(&cache->part_list)) {
		struct list_head *node = list_first(&cache->part_list);
		struct kmem_slab *slab =
			LIST_ENTRY(node, struct kmem_slab, link);

		void *ptr = slab->ops->alloc(cache, slab);

		--slab->free;

		if (!slab->free) {
			list_del(&slab->link);
			list_add(&slab->link, &cache->full_list);
		}
		return ptr;
	}

	if (list_empty(&cache->free_list) && !kmem_cache_grow(cache))
		return 0;

	struct list_head *node = list_first(&cache->free_list);
	struct kmem_slab *slab = LIST_ENTRY(node, struct kmem_slab, link);

	void *ptr = slab->ops->alloc(cache, slab); 

	--slab->free;

	list_del(&slab->link);
	list_add(&slab->link, &cache->part_list);

	return ptr;
}

static struct kmem_slab *kmem_get_slab(void *ptr)
{
	const pfn_t pfn = pa(ptr) >> PAGE_BITS;

	return pfn2page(pfn)->u.slab;
}

void kmem_cache_free(struct kmem_cache *cache, void *ptr)
{
	struct kmem_slab *slab = kmem_get_slab(ptr);

	slab->ops->free(cache, slab, ptr);
	++slab->free;

	if (slab->free == slab->total) {
		list_del(&slab->link);
		list_add(&slab->link, &cache->free_list);
		return;
	}

	if (slab->free == 1) {
		list_del(&slab->link);
		list_add(&slab->link, &cache->part_list);
	}
}


struct kmem_border_tag {
	struct kmem_border_tag *next;
};

struct kmem_small_slab {
	struct kmem_slab common;
	struct kmem_border_tag *free_list;
};

struct kmem_small_cache {
	struct kmem_cache common;
	size_t padded_size;
};


static void *kmem_small_slab_alloc(struct kmem_cache *c, struct kmem_slab *p)
{
	struct kmem_small_slab *slab = (struct kmem_small_slab *)p;
	struct kmem_border_tag *tag = slab->free_list;

	(void) c;

	slab->free_list = tag->next;

	return ((char *)tag) - c->object_size;
}

static void kmem_small_slab_free(struct kmem_cache *c,
			struct kmem_slab *s, void *ptr)
{
	struct kmem_small_slab *slab = (struct kmem_small_slab *)s;
	struct kmem_border_tag *tag = (void *)((char *)ptr + c->object_size);

	(void) c;

	tag->next = slab->free_list;
	slab->free_list = tag;
}

static const struct kmem_slab_ops small_slab_ops = {
	.alloc = kmem_small_slab_alloc,
	.free = kmem_small_slab_free
};

static struct kmem_slab *kmem_small_slab_create(struct kmem_cache *cache,
			struct page *page)
{
	struct kmem_small_cache *small = (struct kmem_small_cache *)cache;

	const pfn_t pages = (pfn_t)1 << cache->order;
	const size_t size = PAGE_SIZE * pages;
	const size_t off = size - sizeof(struct kmem_small_slab);

	char *vaddr = va(page2pfn(page) << PAGE_BITS);
	struct kmem_small_slab *slab = (struct kmem_small_slab *)(vaddr + off);

	slab->common.ops = &small_slab_ops;
	slab->common.total = 0;
	slab->common.free = 0;
	slab->free_list = 0;

	const size_t sz = small->padded_size;

	for (char *ptr = vaddr; ptr + sz <= (char *)slab; ptr += sz) {
		kmem_small_slab_free(cache, &slab->common, ptr);
		++slab->common.total;
		++slab->common.free;
	}

	return (struct kmem_slab *)slab;
}

static const struct kmem_cache_ops small_cache_ops = {
	.create = kmem_small_slab_create
};

static struct kmem_small_cache kmem_small_cache_cache;

static void kmem_small_cache_init(struct kmem_small_cache *cache,
			size_t size, size_t align)
{
	const size_t sz = sizeof(struct kmem_border_tag);
	const size_t al = ALIGN_OF(struct kmem_border_tag);

	size = ALIGN(size, al);
	align = MAXU(align, al);

	cache->common.ops = &small_cache_ops;
	cache->common.object_size = size;
	cache->common.order = 0;

	cache->padded_size = ALIGN(size + sz, align);

	kmem_cache_init(&cache->common);
}

static struct kmem_cache *kmem_small_cache_create(size_t size, size_t align)
{
	struct kmem_small_cache *cache =
		kmem_cache_alloc((struct kmem_cache *)&kmem_small_cache_cache);

	if (!cache)
		return 0;

	kmem_small_cache_init(cache, size, align);

	return (struct kmem_cache *)cache;
}

static void kmem_small_cache_setup(void)
{
	kmem_small_cache_init(&kmem_small_cache_cache,
		sizeof(struct kmem_small_cache),
		ALIGN_OF(struct kmem_small_cache));
}


struct kmem_tag {
	struct kmem_tag *next;
	void *ptr;
};

struct kmem_large_slab {
	struct kmem_slab common;
	struct kmem_tag *free;
	struct kmem_tag *busy;
};

struct kmem_large_cache {
	struct kmem_cache common;
	struct kmem_cache *slab_cache;
	struct kmem_cache *tag_cache;
};


static void *kmem_large_slab_alloc(struct kmem_cache *c, struct kmem_slab *s)
{
	struct kmem_large_slab *slab = (struct kmem_large_slab *)s;

	(void) c;

	struct kmem_tag *tag = slab->free;
	void *ptr = tag->ptr;

	slab->free = tag->next;
	tag->next = slab->busy;
	slab->busy = tag;

	return ptr;
}

static void kmem_large_slab_free(struct kmem_cache *c, struct kmem_slab *s,
			void *ptr)
{
	struct kmem_large_slab *slab = (struct kmem_large_slab *)s;

	(void) c;

	struct kmem_tag *tag = slab->busy;

	slab->busy = tag->next;
	tag->next = slab->free;
	tag->ptr = ptr;
	slab->free = tag;
}

static const struct kmem_slab_ops large_slab_ops = {
	.alloc = kmem_large_slab_alloc,
	.free = kmem_large_slab_free
};

static void kmem_large_slab_release(struct kmem_large_cache *cache,
			struct kmem_large_slab *slab)
{	
	while (slab->free) {
		struct kmem_tag *tag = slab->free;

		slab->free = tag->next;
		kmem_cache_free(cache->tag_cache, tag);
	}
	kmem_cache_free(cache->slab_cache, slab);
}

static struct kmem_slab *kmem_large_slab_create(struct kmem_cache *c,
			struct page *page)
{
	struct kmem_large_cache *cache = (struct kmem_large_cache *)c;
	struct kmem_large_slab *slab = kmem_cache_alloc(cache->slab_cache);

	if (!slab)
		return 0;

	const pfn_t pfs = (pfn_t)1 << c->order;
	const size_t count = (pfs << PAGE_BITS) / c->object_size;
	char *ptr = va(page2pfn(page) << PAGE_BITS);

	slab->common.ops = &large_slab_ops;
	slab->common.total = count;
	slab->common.free = count;

	for (size_t i = 0; i != count; ++i, ptr += c->object_size) {
		struct kmem_tag *tag = kmem_cache_alloc(cache->tag_cache);

		if (!tag) {
			kmem_large_slab_release(cache, slab);
			return 0;
		}

		tag->next = slab->free;
		tag->ptr = ptr;
		slab->free = tag;
	}

	return (struct kmem_slab *)slab;
}

static void kmem_large_slab_destroy(struct kmem_cache *c, struct kmem_slab *s)
{
	struct kmem_large_cache *cache = (struct kmem_large_cache *)c;
	struct kmem_large_slab *slab = (struct kmem_large_slab *)s;

	kmem_large_slab_release(cache, slab);
}

static const struct kmem_cache_ops large_cache_ops = {
	.create = kmem_large_slab_create,
	.destroy = kmem_large_slab_destroy
};

static struct kmem_small_cache kmem_large_cache_cache;
static struct kmem_small_cache kmem_large_slab_cache;
static struct kmem_small_cache kmem_large_tag_cache;

static void kmem_large_cache_init(struct kmem_large_cache *cache,
			size_t size, size_t align)
{
	const size_t object_size = ALIGN(size, align);

	int order = 0;

	for (; order != BUDDY_ORDERS; ++order) {
		const size_t bytes = (PAGE_SIZE << order);

		if (bytes / object_size >= 8)
			break;
	}

	cache->common.object_size = object_size;
	cache->common.order = order;
	cache->common.ops = &large_cache_ops;

	cache->slab_cache = (struct kmem_cache *)&kmem_large_slab_cache;
	cache->tag_cache = (struct kmem_cache *)&kmem_large_tag_cache;

	kmem_cache_init(&cache->common);
}

static struct kmem_cache *kmem_large_cache_create(size_t size, size_t align)
{
	struct kmem_large_cache *cache =
		kmem_cache_alloc((struct kmem_cache *)&kmem_large_cache_cache);

	if (!cache)
		return 0;

	kmem_large_cache_init(cache, size, align);

	return (struct kmem_cache *)cache;
}

static void kmem_large_cache_setup(void)
{
	kmem_small_cache_init(&kmem_large_cache_cache,
		sizeof(struct kmem_large_cache),
		ALIGN_OF(struct kmem_large_cache));

	kmem_small_cache_init(&kmem_large_slab_cache,
		sizeof(struct kmem_large_slab),
		ALIGN_OF(struct kmem_large_slab));

	kmem_small_cache_init(&kmem_large_tag_cache,
		sizeof(struct kmem_tag),
		ALIGN_OF(struct kmem_tag));
}


struct kmem_cache *kmem_cache_create(size_t size, size_t align)
{
	struct kmem_cache *cache;

	if (size <= PAGE_SIZE / 8)
		cache = kmem_small_cache_create(size, align);
	else
		cache = kmem_large_cache_create(size, align);

	if (!cache)
		return 0;


	return cache;
}

void kmem_cache_destroy(struct kmem_cache *cache)
{
	kmem_cache_reap(cache);

	if (!list_empty(&cache->part_list))
		return;

	if (!list_empty(&cache->full_list))
		return;

	kmem_cache_free(kmem_get_slab(cache)->cache, cache);
}


static const size_t kmem_size[] = {
	8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128,
	256, 384, 512, 640, 768, 1024, 1280, 1536, 1792, 2048, 3072, 4096,
	8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024
};

#define KMEM_POOLS (sizeof(kmem_size)/sizeof(kmem_size[0]))
static struct kmem_cache *kmem_pool[KMEM_POOLS];


static int kmem_cache_index(size_t size)
{
	for (int i = 0; i != KMEM_POOLS; ++i) {
		if (size <= kmem_size[i])
			return i;
	}
	return -1;
}

void *kmem_alloc(size_t size)
{
	const int i = kmem_cache_index(size);

	if (i == -1)
		return 0;

	return kmem_cache_alloc(kmem_pool[i]);
}

void kmem_free(void *ptr)
{
	if (!ptr)
		return;

	struct kmem_slab *slab = kmem_get_slab(ptr);

	if (!slab)
		return;

	kmem_cache_free(slab->cache, ptr);
}

void setup_alloc(void)
{
	kmem_small_cache_setup();
	kmem_large_cache_setup();

	for (int i = 0; i != KMEM_POOLS; ++i) {
		kmem_pool[i] = kmem_cache_create(kmem_size[i], sizeof(void *));
		DBG_ASSERT(kmem_pool[i] != 0);
	}
}

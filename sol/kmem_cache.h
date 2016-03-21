#ifndef __KMEM_CACHE_H__
#define __KMEM_CACHE_H__

#include "kernel.h"

struct kmem_cache;

struct kmem_cache *kmem_cache_create(size_t size, size_t align);
void kmem_cache_destroy(struct kmem_cache *cache);
void *kmem_cache_alloc(struct kmem_cache *cache);
void kmem_cache_free(struct kmem_cache *cache, void *ptr);
void kmem_cache_reap(struct kmem_cache *cache);

void *kmem_alloc(size_t size);
void kmem_free(void *ptr);

void setup_alloc(void);

#define KMEM_CACHE(type) \
	kmem_cache_create(sizeof(type), ALIGN_OF(type))

#endif /*__KMEM_CACHE_H__*/

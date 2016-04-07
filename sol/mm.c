#include "kmem_cache.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "mm.h"

#include <stdbool.h>


static struct kmem_cache *mm_cachep;

static struct page *alloc_page_table(void)
{
	struct page *page = alloc_pages(0);

	if (page)
		memset(page_addr(page), 0, PAGE_SIZE);
	return page;
}

static void free_page_table(struct page *pt)
{
	free_pages(pt, 0);
}

static struct mm *alloc_mm(void)
{
	struct mm *mm = kmem_cache_alloc(mm_cachep);

	if (mm)
		memset(mm, 0, sizeof(*mm));
	return mm;
}

static void free_mm(struct mm *mm)
{
	kmem_cache_free(mm_cachep, mm);
}

struct mm *create_mm(void)
{
	struct mm *mm = alloc_mm();

	if (!mm)
		return 0;

	struct page *pt = alloc_page_table();

	if (!pt) {
		free_mm(mm);
		return 0;
	}

	/*
	 * kernel part is shared between the all threads, so we need to
	 * copy at least kernel part of page table, to make it usable.
	 */
	const size_t offset = pml4_i(HIGH_BASE) * sizeof(pte_t);

	memcpy((char *)page_addr(pt) + offset,
		(char *)va(load_pml4()) + offset, PAGE_SIZE - offset);
	mm->pt = pt;

	return mm;
}

void release_mm(struct mm *mm)
{
	free_page_table(mm->pt);
	free_mm(mm);
}

void setup_mm(void)
{
	DBG_ASSERT((mm_cachep = KMEM_CACHE(struct mm)) != 0);
}

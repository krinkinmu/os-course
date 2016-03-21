#include "paging.h"
#include "string.h"
#include "error.h"
#include "stdio.h"


static int pt_index(virt_t vaddr, int level)
{
	switch (level) {
	case 4:
		return pml4_i(vaddr);
	case 3:
		return pml3_i(vaddr);
	case 2:
		return pml2_i(vaddr);
	case 1:
		return pml1_i(vaddr);
	}

	DBG_ASSERT(0 && "Unreachable");
	return 0;
}

static virt_t pt_iter_addr(const struct pt_iter *iter)
{
	const virt_t addr = (virt_t)iter->idx[3] << 39 |
			(virt_t)iter->idx[2] << 30 |
			(virt_t)iter->idx[1] << 21 |
			(virt_t)iter->idx[0] << 12;

	return canonical(addr);
}

struct pt_iter *pt_iter_set(struct pt_iter *iter, pte_t *pml4, virt_t addr)
{
	memset(iter, 0, sizeof(*iter));

	if (!pml4)
		return iter;

	int level = PT_MAX_LEVEL;
	int idx = pml4_i(addr);
	pte_t pte = pml4[idx];

	iter->pt[level] = pml4;
	iter->idx[level] = idx;

	while (pte_present(pte) && level != 0 && !pte_large(pte)) {
		const phys_t paddr = pte_phys(pte);
		pte_t *pt = va(paddr);

		idx = pt_index(addr, level--);
		pte = pt[idx];

		iter->idx[level] = idx;
		iter->pt[level] = pt;
	}

	iter->addr = pt_iter_addr(iter);
	iter->level = level;

	return iter;
}

struct pt_iter *pt_iter_next(struct pt_iter *iter)
{
	int level = iter->level;
	int idx = iter->idx[level];

	while (idx == PT_SIZE - 1 && level != PT_MAX_LEVEL)
		idx = iter->idx[++level];

	if (idx == PT_SIZE - 1)
		return 0;

	pte_t pte = iter->pt[level][++idx];

	iter->idx[level] = idx;

	while (pte_present(pte) && level != 0 && !pte_large(pte)) {
		const phys_t paddr = pte_phys(pte);
		pte_t *pt = va(paddr);

		iter->idx[--level] = 0;
		iter->pt[level] = pt;
		pte = pt[0];
	}
	iter->level = level;

	while (level != 0) {
		iter->pt[--level] = 0;
		iter->idx[level] = 0;
	}
	iter->addr = pt_iter_addr(iter);

	return iter;
}

bool pt_iter_present(const struct pt_iter *iter)
{
	const int level = iter->level;
	const int index = iter->idx[level];

	return pte_present(iter->pt[level][index]);
}

bool pt_iter_large(const struct pt_iter *iter)
{
	const int level = iter->level;
	const int index = iter->idx[level];

	return pte_large(iter->pt[level][index]);
}

static struct page *alloc_page_table(pte_t flags)
{
	struct page *page =
		(flags & PTE_LOW) ? __alloc_pages(0, NT_LOW) : alloc_pages(0);

	if (page) {
		memset(va(page_paddr(page)), 0, PAGE_SIZE);
		page->u.refcount = 0;
	}
	return page;
}

static void free_page_table(struct page *page)
{
	free_pages(page, 0);
}

static void pt_release_pml2(pte_t *pml2, virt_t from, virt_t to)
{
	virt_t vaddr = from;

	for (int i = pml2_i(from); vaddr != to; ++i) {
		const pte_t pte = pml2[i];
		const virt_t bytes =
			MINU(PML1_SIZE - (vaddr & PML1_MASK), to - vaddr);	
		const pfn_t pages = bytes >> PAGE_BITS;

		if (pte_present(pte) && !pte_large(pte)) {
			const pfn_t pfn = pte_phys(pte) >> PAGE_BITS;
			struct page *pt = pfn2page(pfn);

			pt->u.refcount -= pages;

			if (pt->u.refcount == 0) {
				pml2[i] = 0;
				free_page_table(pt);
			}
		}
		vaddr += bytes;	
	}
}

static int pt_populate_pml2(pte_t *pml2, virt_t from, virt_t to, pte_t flags)
{
	virt_t vaddr = from;

	for (int i = pml2_i(from); vaddr != to; ++i) {
		const virt_t bytes =
			MINU(PML1_SIZE - (vaddr & PML1_MASK), to - vaddr);
		const pfn_t pages = bytes >> PAGE_BITS;

		if (!pte_present(pml2[i]) && !pte_large(flags)) {
			struct page *pt = alloc_page_table(flags);

			if (!pt) {
				pt_release_pml2(pml2, from, vaddr);
				return -ENOMEM;
			}

			pt->u.refcount += pages;
			pml2[i] = page_paddr(pt) | flags;
		} else if (pte_present(pml2[i]) && !pte_large(pml2[i])) {
			const pfn_t pfn = pte_phys(pml2[i]) >> PAGE_BITS;
			struct page *pt = pfn2page(pfn);

			pt->u.refcount += pages;
		}
		vaddr += bytes;
	}

	return 0;
}

static void pt_release_pml3(pte_t *pml3, virt_t from, virt_t to)
{
	virt_t vaddr = from;

	for (int i = pml3_i(from); vaddr != to; ++i) {
		const pte_t pte = pml3[i];
		const virt_t bytes = MINU(PML2_SIZE - (vaddr & PML2_MASK),
					to - vaddr);
		const pfn_t pages = bytes >> PAGE_BITS;

		if (pte_present(pte)) {
			const phys_t paddr = pte_phys(pte);
			const pfn_t pfn = paddr >> PAGE_BITS;
			struct page *pt = pfn2page(pfn);

			pt_release_pml2(va(paddr), vaddr, vaddr + bytes);
			pt->u.refcount -= pages;

			if (pt->u.refcount == 0) {
				pml3[i] = 0;
				free_page_table(pt);
			}
		}
		vaddr += bytes;	
	}
}

static int pt_populate_pml3(pte_t *pml3, virt_t from, virt_t to, pte_t flags)
{
	virt_t vaddr = from;

	for (int i = pml3_i(from); vaddr != to; ++i) {
		struct page *pt;
		phys_t paddr;
		const virt_t bytes =
			MINU(PML2_SIZE - (vaddr & PML2_MASK), to - vaddr);
		const pfn_t pages = bytes >> PAGE_BITS;

		if (!pte_present(pml3[i])) {
			pt = alloc_page_table(flags);

			if (!pt) {
				pt_release_pml3(pml3, from, vaddr);
				return -ENOMEM;
			}

			paddr = page_paddr(pt);
			pml3[i] = paddr | (flags & ~PTE_LARGE);
		} else {
			const pte_t pte = pml3[i];

			paddr = pte_phys(pte);
			pt = pfn2page(paddr >> PAGE_BITS);
		}
		pt->u.refcount += pages;

		const int rc = pt_populate_pml2(va(paddr), vaddr, vaddr + bytes,
					flags);

		if (rc) {
			pt_release_pml3(pml3, from, vaddr);
			pt->u.refcount -= pages;

			if (pt->u.refcount == 0) {
				pml3[i] = 0;
				free_page_table(pt);
			}

			return rc;
		}
		vaddr += bytes;
	}

	return 0;
}

static void pt_release_pml4(pte_t *pml4, virt_t from, virt_t to)
{
	virt_t vaddr = from;

	for (int i = pml4_i(from); vaddr != to; ++i) {
		const pte_t pte = pml4[i];
		const virt_t bytes =
			MINU(PML3_SIZE - (vaddr & PML3_MASK), to - vaddr);
		const pfn_t pages = bytes >> PAGE_BITS;

		if (pte_present(pte)) {
			const phys_t paddr = pte_phys(pte);
			const pfn_t pfn = paddr >> PAGE_BITS;
			struct page *pt = pfn2page(pfn);

			pt_release_pml3(va(paddr), vaddr, vaddr + bytes);
			pt->u.refcount -= pages;

			if (pt->u.refcount == 0) {
				pml4[i] = 0;
				free_page_table(pt);
			}
		}
		vaddr += bytes;	
	}
}

static int pt_populate_pml4(pte_t *pml4, virt_t from, virt_t to, pte_t flags)
{
	virt_t vaddr = from;

	for (int i = pml4_i(from); vaddr < to; ++i) {
		struct page *pt;
		phys_t paddr;
		const virt_t bytes =
			MINU(PML3_SIZE - (vaddr & PML3_MASK), to - vaddr);
		const pfn_t pages = bytes >> PAGE_BITS;

		if (!pte_present(pml4[i])) {
			pt = alloc_page_table(flags);

			if (!pt) {
				pt_release_pml4(pml4, from, vaddr);
				return -ENOMEM;
			}

			paddr = page_paddr(pt);
			pml4[i] = paddr | (flags & ~PTE_LARGE);
		} else {
			const pte_t pte = pml4[i];

			paddr = pte_phys(pte);
			pt = pfn2page(paddr >> PAGE_BITS);
		}
		pt->u.refcount += pages;

		const int rc = pt_populate_pml3(va(paddr), vaddr, vaddr + bytes,
					flags);

		if (rc) {
			pt_release_pml4(pml4, from, vaddr);
			pt->u.refcount -= pages;

			if (pt->u.refcount == 0) {
				pml4[i] = 0;
				free_page_table(pt);
			}

			return rc;
		}

		vaddr += bytes;
	}

	return 0;
}

int __pt_populate_range(pte_t *pml4, virt_t from, virt_t to, pte_t flags)
{
	DBG_ASSERT(linear(from) < linear(to));
	DBG_ASSERT((flags & ~PTE_FLAGS) == 0);
	DBG_ASSERT(pml4 != 0);

	from = ALIGN_DOWN(linear(from), PAGE_SIZE);
	to = ALIGN(linear(to), PAGE_SIZE);

	return pt_populate_pml4(pml4, from, to, flags | PTE_PRESENT);
}

void __pt_release_range(pte_t *pml4, virt_t from, virt_t to)
{
	DBG_ASSERT(linear(from) < linear(to));
	DBG_ASSERT(pml4 != 0);

	from = ALIGN_DOWN(linear(from), PAGE_SIZE);
	to = ALIGN(linear(to), PAGE_SIZE);

	pt_release_pml4(pml4, from, to);
}

static int map_range_large(pte_t *pml4, virt_t from, virt_t to, phys_t phys,
			pte_t flags)
{
	const int rc = __pt_populate_range(pml4, from, to, flags | PTE_LARGE);
	const pte_t page_flags = PTE_WRITE | PTE_PRESENT;

	if (rc)
		return rc;

	struct pt_iter iter;

	for_each_slot_in_range(pml4, from, to, iter) {
		const int level = iter.level;
		const int index = iter.idx[level];
		pte_t *pt = iter.pt[level];

		DBG_ASSERT(level == 1 || level == 0);
		DBG_ASSERT(!pte_present(pt[index]));

		if (level == 1) {
			pt[index] = phys | page_flags | PTE_LARGE;
			phys += PML1_SIZE;
		} else {
			pt[index] = phys | page_flags;
			phys += PAGE_SIZE;
		}
	}

	return 0;
}

#define KMAP_ORDERS 16

struct kmap_range {
	struct list_head link;
	pfn_t pages;
};


static struct kmap_range all_kmap_ranges[KMAP_PAGES];
static struct list_head free_kmap_ranges[KMAP_ORDERS];

static int kmap_order(pfn_t pages)
{ return MIN(ilog2(pages), KMAP_ORDERS - 1); }

static struct kmap_range *virt2kmap(virt_t vaddr)
{
	const pfn_t range = (vaddr - KMAP_BASE) >> PAGE_BITS;

	return &all_kmap_ranges[range];
}

static virt_t kmap2virt(struct kmap_range *range)
{
	const pfn_t page = range - all_kmap_ranges;

	return KMAP_BASE + (page << PAGE_BITS);
}

static struct kmap_range *kmap_find_free_range(int order, pfn_t pages)
{
	struct list_head *head = &free_kmap_ranges[order];
	struct list_head *ptr = head->next;

	for (; ptr != head; ptr = ptr->next) {
		struct kmap_range *range = LIST_ENTRY(ptr, struct kmap_range,
					link);

		if (range->pages >= pages)
			return range;
	}
	return 0;
}

static void kmap_free_range(struct kmap_range *range, pfn_t pages)
{
	if (range > all_kmap_ranges) {
		struct kmap_range *prev = range - (range - 1)->pages;

		if (!list_empty(&prev->link)) {
			list_del(&prev->link);
			pages += prev->pages;
			range = prev;
		}
	}

	if (range + pages < all_kmap_ranges + KMAP_PAGES) {
		struct kmap_range *next = range + pages;

		if (!list_empty(&next->link)) {
			list_del(&next->link);
			pages += next->pages;
		}
	}

	(range + pages - 1)->pages = range->pages = pages;
	list_add(&range->link, free_kmap_ranges + kmap_order(pages));
}

static struct kmap_range *kmap_alloc_range(pfn_t pages)
{
	for (int order = kmap_order(pages); order < KMAP_ORDERS; ++order) {
		struct kmap_range *range = kmap_find_free_range(order, pages);

		if (!range)
			continue;

		const pfn_t range_pages = range->pages;

		list_del(&range->link);
		list_init(&range->link);
		(range + pages - 1)->pages = range->pages = pages;

		if (range_pages > pages)
			kmap_free_range(range + pages, range_pages - pages);

		return range;
	}
	return 0;
}

void *kmap(struct page **pages, size_t count)
{
	struct kmap_range *range = kmap_alloc_range(count);

	if (!range)
		return 0;

	const virt_t from = kmap2virt(range);
	const virt_t to = from + (count << PAGE_BITS);
	pte_t *pt = va(load_pml4());
	struct pt_iter iter;
	size_t i = 0;

	for_each_slot_in_range(pt, from, to, iter) {
		const phys_t paddr = page_paddr(pages[i++]);
		const int level = iter.level;
		const int idx = iter.idx[level];

		iter.pt[level][idx] = paddr | PTE_WRITE | PTE_PRESENT;
		flush_tlb_addr(iter.addr);
	}

	return (void *)from;
}

void kunmap(void *vaddr)
{
	struct kmap_range *range = virt2kmap((virt_t)vaddr);
	const pfn_t count = range->pages;

	const virt_t from = (virt_t)vaddr;
	const virt_t to = from + (count << PAGE_BITS);
	pte_t *pt = va(load_pml4());
	virt_t virt = from;
	struct pt_iter iter;

	for_each_slot_in_range(pt, from, to, iter) {
		const int level = iter.level;
		const int idx = iter.idx[level];

		iter.pt[level][idx] = 0;
		flush_tlb_addr(virt);
		virt += PAGE_SIZE;
	}
	kmap_free_range(range, range->pages);
}

static int setup_kmap_mapping(pte_t *pml4)
{
	for (int i = 0; i != KMAP_ORDERS; ++i)
		list_init(&free_kmap_ranges[i]);

	kmap_free_range(all_kmap_ranges, KMAP_PAGES);

	return __pt_populate_range(pml4, KMAP_BASE, KMAP_BASE + KMAP_SIZE,
				PTE_WRITE | PTE_LOW);
}

static int setup_fixed_mapping(pte_t *pml4)
{
	const virt_t bytes = max_pfns() << PAGE_BITS;

	return map_range_large(pml4, HIGH_BASE, HIGH_BASE + bytes, 0,
				PTE_WRITE | PTE_LOW);
}

static int setup_kernel_mapping(pte_t *pml4)
{
	const virt_t bytes = KERNEL_SIZE;

	return map_range_large(pml4, KERNEL_BASE, KERNEL_BASE + bytes, 0,
				PTE_WRITE | PTE_LOW);
}

void setup_paging(void)
{
	struct page *page = alloc_page_table(PTE_LOW);

	DBG_ASSERT(page != 0);

	const phys_t paddr = page_paddr(page);
	pte_t *pt = va(paddr);

	DBG_ASSERT(setup_fixed_mapping(pt) == 0);
	DBG_ASSERT(setup_kernel_mapping(pt) == 0);
	DBG_ASSERT(setup_kmap_mapping(pt) == 0);
	store_pml4(paddr);
}

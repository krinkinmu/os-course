#ifndef __PAGING_H__
#define __PAGING_H__

#include "kernel.h"
#include "memory.h"

#include <stdbool.h>
#include <stdint.h>


typedef uint64_t pte_t;

#define PTE_PRESENT  ((pte_t)BIT_CONST(0))
#define PTE_WRITE    ((pte_t)BIT_CONST(1))
#define PTE_USER     ((pte_t)BIT_CONST(2))
#define PTE_LARGE    ((pte_t)BIT_CONST(7))
#define PTE_LOW      ((pte_t)BIT_CONST(9))
#define PTE_FLAGS    (PTE_PRESENT | PTE_WRITE | PTE_USER | PTE_LARGE | PTE_LOW)

#define PTE_PT_FLAGS       (PTE_WRITE | PTE_USER)
#define PTE_LARGE_PT_FLAGS (PTE_PT_FLAGS | PTE_LARGE)

#define PT_SIZE     (PAGE_SIZE / sizeof(pte_t))
#define PML1_PAGES  ((pfn_t)PT_SIZE)
#define PML1_SIZE   ((virt_t)PML1_PAGES << PAGE_BITS)
#define PML1_MASK   (PML1_SIZE - 1)
#define PML2_PAGES  (PML1_PAGES * PT_SIZE)
#define PML2_SIZE   ((virt_t)PML2_PAGES << PAGE_BITS)
#define PML2_MASK   (PML2_SIZE - 1)
#define PML3_PAGES  (PML2_PAGES * PT_SIZE)
#define PML3_SIZE   ((virt_t)PML3_PAGES << PAGE_BITS)
#define PML3_MASK   (PML3_SIZE - 1)
#define PML4_PAGES  (PML3_PAGES * PT_SIZE)
#define PML4_SIZE   ((virt_t)PML4_PAGES << PAGE_BITS)
#define PML4_MASK   (PML4_SIZE - 1)

static inline bool pte_present(pte_t pte)
{ return (pte & PTE_PRESENT) != 0; }

static inline bool pte_write(pte_t pte)
{ return (pte & PTE_WRITE) != 0; }

static inline bool pte_user(pte_t pte)
{ return (pte & PTE_USER) != 0; }

static inline bool pte_large(pte_t pte)
{ return (pte & PTE_LARGE) != 0; }

static inline phys_t pte_phys(pte_t pte)
{ return (phys_t)(pte & (pte_t)BITS_CONST(47, 12)); }

static inline int pml4_i(virt_t vaddr)
{ return (vaddr & (virt_t)BITS_CONST(47, 39)) >> 39; }

static inline int pml3_i(virt_t vaddr)
{ return (vaddr & (virt_t)BITS_CONST(38, 30)) >> 30; }

static inline int pml2_i(virt_t vaddr)
{ return (vaddr & (virt_t)BITS_CONST(29, 21)) >> 21; }

static inline int pml1_i(virt_t vaddr)
{ return (vaddr & (virt_t)BITS_CONST(20, 12)) >> 12; }

static inline int page_off(virt_t vaddr)
{ return (vaddr & PAGE_MASK); }


#define PT_MAX_LEVEL 3

struct pt_iter {
	int level;
	pte_t *pt[PT_MAX_LEVEL + 1];
	int idx[PT_MAX_LEVEL + 1];
	virt_t addr;
};

struct pt_iter *pt_iter_set(struct pt_iter *iter, pte_t *pt, virt_t addr);
struct pt_iter *pt_iter_next(struct pt_iter *iter);
bool pt_iter_present(const struct pt_iter *iter);
bool pt_iter_large(const struct pt_iter *iter);

#define for_each_slot_in_range(pml4, from, to, iter) \
	for (struct pt_iter *__FE_iter_p = pt_iter_set(&(iter), pml4, from); \
		__FE_iter_p && __FE_iter_p->addr < to; \
		__FE_iter_p = pt_iter_next(__FE_iter_p))

int __pt_populate_range(pte_t *pml4, virt_t from, virt_t to, pte_t flags);
void __pt_release_range(pte_t *pml4, virt_t from, virt_t to);

static inline int pt_populate_range(pte_t *pml4, virt_t from, virt_t to)
{ return __pt_populate_range(pml4, from, to, PTE_PT_FLAGS); }

static inline int pt_populate_range_large(pte_t *pml4, virt_t from, virt_t to)
{ return __pt_populate_range(pml4, from, to, PTE_LARGE_PT_FLAGS); }

static inline void pt_release_range(pte_t *pml4, virt_t from, virt_t to)
{ __pt_release_range(pml4, from, to); }

static inline void get_page(struct page *page)
{ ++page->u.refcount; }

static inline void put_page(struct page *page)
{
	if (--page->u.refcount == 0)
		free_pages(page, 0);
}

static inline virt_t canonical(virt_t addr)
{
	if ((addr & BIT_CONST(47)) == 0)
		return addr;
	return addr | ((~(uintptr_t)0) << 48);
}

static inline virt_t linear(virt_t addr)
{
	return addr & BITS_CONST(47, 0);
}

static inline void store_pml4(phys_t pml4)
{ __asm__ volatile ("movq %0, %%cr3" : : "a"(pml4) : "memory"); }

static inline phys_t load_pml4(void)
{
	phys_t pml4;

	__asm__ volatile ("movq %%cr3, %0" : "=a"(pml4));
	return pml4;
}

static inline void flush_tlb_addr(virt_t vaddr)
{ __asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory"); }

void *kmap(struct page **pages, size_t count);
void kunmap(void *ptr);


void setup_paging(void);

#endif /*__PAGING_H__*/

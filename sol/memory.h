#ifndef __MEMORY_H__
#define __MEMORY_H__

#define PAGE_BITS         12
#define PAGE_SIZE         (1 << 12)
#define PAGE_MASK         (PAGE_SIZE - 1)
#define KERNEL_BASE       0xffffffff80000000
#define HIGH_BASE         0xffff800000000000
#define PHYSICAL_BASE     0x0000000000000000
#define MAX_PHYS_SIZE     BIT_CONST(36)       // max 0.5GB of page structs

#define KERNEL_CODE       0x18
#define KERNEL_DATA       0x20

#define KERNEL_PHYS(x)    ((x) - KERNEL_BASE)
#define KERNEL_VIRT(x)    ((x) + KERNEL_BASE)
#define PA(x)             ((x) - HIGH_BASE)
#define VA(x)             ((x) + HIGH_BASE)

#ifndef __ASM_FILE__

#define BUDDY_ORDERS      12
#define BUDDY_ORDER_BITS  8ul
#define PAGE_NODE_BITS    8ul
#define PAGE_NODE_MASK    (BIT_CONST(PAGE_NODE_BITS) - 1)
#define PAGE_BUSY_BIT     PAGE_NODE_BITS
#define PAGE_BUSY_MASK    BIT_CONST(PAGE_BUSY_BIT)
#define PADDR_BITS        48

#ifdef CONFIG_KERNEL_SIZE
#define KERNEL_SIZE       CONFIG_KERNEL_SIZE
#else
#define KERNEL_SIZE       (3ul * 512ul * 1024ul * 1024ul)
#endif

#ifdef CONFIG_KMAP_SIZE
#define KMAP_SIZE         CONFIG_KMAP_SIZE
#else
#define KMAP_SIZE         (512ul * 1024ul * 1024ul - PAGE_SIZE)
#endif

#define KMAP_BASE         (KERNEL_BASE + KERNEL_SIZE)
#define LOWMEM_SIZE       (4ul * 1024ul * 1024ul * 1024ul)
#define KERNEL_PAGES      (KERNEL_SIZE / PAGE_SIZE)
#define KMAP_PAGES        (KMAP_SIZE / PAGE_SIZE)

#include <stdint.h>

#include "locking.h"
#include "balloc.h"
#include "list.h"

typedef uintptr_t pfn_t;
typedef uintptr_t phys_t;
typedef uintptr_t virt_t;

static inline phys_t kernel_phys(const void *addr)
{ return KERNEL_PHYS((virt_t)addr); }

static inline void *kernel_virt(phys_t paddr)
{ return (void *)KERNEL_VIRT(paddr); }

static inline phys_t pa(const void *vaddr)
{ return (phys_t)PA((virt_t)vaddr); }

static inline void *va(phys_t paddr)
{ return (void *)VA(paddr); }


struct memory_node;
struct kmem_slab;

struct page {
	unsigned long flags;
	struct list_head link;

	union {
		struct kmem_slab *slab;
		unsigned long refcount;
		int order;
	} u;
};

static inline int page_node_id(const struct page *page)
{ return page->flags & PAGE_NODE_MASK; }

static inline bool page_busy(const struct page *page)
{ return (page->flags & PAGE_BUSY_MASK) != 0; }

static inline bool page_free(const struct page *page)
{ return !page_busy(page); }

static inline void page_set_busy(struct page *page)
{ page->flags |= PAGE_BUSY_MASK; }

static inline void page_set_free(struct page *page)
{ page->flags &= ~PAGE_BUSY_MASK; }

static inline int page_get_order(const struct page *page)
{ return page->u.order; }

static inline void page_set_order(struct page *page, int order)
{ page->u.order = order; }


enum node_type {
	NT_LOW,
	NT_HIGH,
	NT_COUNT
};

struct memory_node {
	struct list_head link;
	struct page *mmap;
	struct spinlock lock;
	pfn_t begin_pfn;
	pfn_t end_pfn;
	int id;
	enum node_type type;

	struct list_head free_list[BUDDY_ORDERS];
};

void memory_free_region(unsigned long long addr, unsigned long long size);

struct memory_node *memory_node_get(int id);
pfn_t max_pfns(void);
struct page *pfn2page(pfn_t pfn);
pfn_t page2pfn(const struct page *page);
struct page *alloc_pages_node(int order, struct memory_node *node);
void free_pages_node(struct page *pages, int order, struct memory_node *node);
struct page *__alloc_pages(int order, int type);
struct page *alloc_pages(int order);
void free_pages(struct page *pages, int order);

static inline struct memory_node *page_node(const struct page *page)
{ return memory_node_get(page_node_id(page)); }

static inline phys_t page_paddr(struct page *page)
{ return (phys_t)page2pfn(page) << PAGE_BITS; }

static inline void *page_addr(struct page *page)
{ return va(page_paddr(page)); }

void setup_memory(void);
void setup_buddy(void);

#endif /*__ASM_FILE__*/

#endif /*__MEMORY_H__*/

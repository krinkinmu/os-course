#ifndef __BOOT_ALLOCATOR_H__
#define __BOOT_ALLOCATOR_H__

#include <stddef.h>
#include <stdbool.h>

typedef void (*region_fptr_t)(unsigned long long addr, unsigned long long size);

void balloc_for_each_region(region_fptr_t exec);
void balloc_for_each_free_region(region_fptr_t exec);

void balloc_add_region(unsigned long long addr, unsigned long long size);
void balloc_reserve_region(unsigned long long addr, unsigned long long size);

long long balloc_alloc_aligned(unsigned long long low, unsigned long long high,
			size_t size, size_t align);
long long balloc_alloc(unsigned long long low, unsigned long long high,
			size_t size);
void balloc_free(unsigned long long addr, size_t size);

#endif /*__BOOT_ALLOCATOR_H__*/

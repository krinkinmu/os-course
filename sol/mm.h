#ifndef __MM_H__
#define __MM_H__

#include "memory.h"
#include "paging.h"


struct mm {
	struct page *pt;
	uintptr_t stack_pointer;
};


struct mm *create_mm(void);
void release_mm(struct mm *mm);
void setup_mm(void);

#endif /*__MM_H__*/

#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

#include <stdint.h>

void backtrace(uint64_t rbp, uintptr_t stack_begin, uintptr_t stack_end);

#endif /*__BACKTRACE_H__*/

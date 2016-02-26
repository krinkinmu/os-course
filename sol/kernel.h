#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stddef.h>
#include <stdint.h>

#define CONTAINER_OF(ptr, type, member) \
	(type *)( (char *)(ptr) - offsetof(type, member) )

#define MIN_CONST(l, r)		((l) < (r) ? (l) : (r))
#define MAX_CONST(l, r)		((l) > (r) ? (l) : (r))
#define BIT_CONST(b)		((uintmax_t)1 << (b))
#define BIT(b)			BIT_CONST(b)
#define ALL_BITS		(~((uintmax_t)0))
#define BITS_CONST(hi, lo)	((BIT_CONST((hi) + 1) - 1) & (ALL_BITS << (lo)))
#define BITS(hi, lo)		BITS_CONST(hi, lo)


static inline intmax_t MIN(intmax_t l, intmax_t r)
{ return MIN_CONST(l, r); }

static inline uintmax_t MINU(uintmax_t l, uintmax_t r)
{ return MIN_CONST(l, r); }

static inline intmax_t MAX(intmax_t l, intmax_t r)
{ return MAX_CONST(l, r); }

static inline uintmax_t MAXU(uintmax_t l, uintmax_t r)
{ return MAX_CONST(l, r); }

#endif /*__KERNEL_H__*/

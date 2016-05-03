#ifndef __VSINKPRINTF_H__
#define __VSINKPRINTF_H__

#include <stddef.h>
#include <stdarg.h>

struct vsinkprintf_sink {
	void (*write)(struct vsinkprintf_sink *, const char *, size_t);
};

void vsinkprintf(struct vsinkprintf_sink *sink,
			const char *fmt, va_list args);

#endif /*__VSNPRINTF_H__*/

#include "string.h"

void *memcpy(void *dst, const void *src, size_t size)
{
	char *d = dst;
	const char *s = src;

	while (size--)
		*d++ = *s++;
	return dst;
}

static void *memcpy_r(void *dst, const void *src, size_t size)
{
	char *d = dst;
	const char *s = src;

	d += size;
	s += size;
	while (size--)
		*(--d) = *(--s);
	return dst;
}

void *memmove(void *dst, const void *src, size_t size)
{ return dst > src ? memcpy_r(dst, src, size) : memcpy(dst, src, size); }

void *memset(void *dst, int value, size_t size)
{
	char *ptr = dst;

	while (size--)
		*ptr++ = value;
	return dst;
}

int memcmp(const void *lptr, const void *rptr, size_t size)
{
	const char *l = lptr;
	const char *r = rptr;

	while (size && *l == *r) {
		++l;
		++r;
		--size;
	}

	return size ? *l - *r : 0;
}

size_t strlen(const char *str)
{
	const char *pos = str;

	while (*pos) ++pos;
	return pos - str;
}

char *strchr(const char *str, int c)
{
	while (*str && *str != c)
		++str;
	return *str ? (char *)str : 0;
}

int strcmp(const char *l, const char *r)
{
	while (*l == *r && *l) {
		++l;
		++r;
	}
	return *l - *r;
}

char *strncpy(char *dst, const char *src, size_t size)
{
	char *ret = dst;

	while (size-- && *src)
		*dst++ = *src++;
	return ret;
}

char *strcpy(char *dst, const char *src)
{
	char *ret = dst;

	while (*src)
		*dst++ = *src++;
	*dst = 0;
	return ret;
}

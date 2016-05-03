#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t size);
void *memmove(void *dst, const void *src, size_t size);
void *memset(void *dst, int value, size_t size);
int memcmp(const void *l, const void *r, size_t size);
size_t strlen(const char *str);
char *strchr(const char *str, int c);
int strcmp(const char *l, const char *r);
char *strncpy(char *dst, const char *src, size_t size);
char *strcpy(char *dst, const char *src);

#endif /*__STRING_H__*/

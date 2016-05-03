#ifndef __IOPORT_H__
#define __IOPORT_H__

#include <stdint.h>

static inline void out8(unsigned short port, uint8_t data)
{ __asm__ volatile("outb %0, %1" : : "a"(data), "d"(port)); }

static inline uint8_t in8(unsigned short port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "d"(port));
	return value;
}

static inline void out16(unsigned short port, uint16_t data)
{ __asm__ volatile("outw %0, %1" : : "a"(data), "d"(port)); }

static inline uint16_t in16(unsigned short port)
{
	uint16_t value;

	__asm__ volatile("inw %1, %0" : "=a"(value) : "d"(port));
	return value;
}

static inline void out32(unsigned short port, uint32_t data)
{ __asm__ volatile("outl %0, %1" : : "a"(data), "d"(port)); }

static inline uint32_t in32(unsigned short port)
{
	uint32_t value;

	__asm__ volatile("inl %1, %0" : "=a"(value) : "d"(port));
	return value;
}

#endif /* __IOPORT_H__ */

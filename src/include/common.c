// common.c -- Defines some global functions.

#include "common.h"
#include "../lib/printf.h"

void outb(const u16int port, const u8int value) {
	__asm__ __volatile__("outb %1, %0" : : "dN" (port), "a" (value));
}

u8int inb(const u16int port) {
	u8int ret;
	__asm__ __volatile__("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

u16int inw(const u16int port) {
	u16int ret;
	__asm__ __volatile__("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

void* memcpy(void* dst, const void* src, u32int num) {
	s8int* dp = dst;
	const s8int* sp = src;
	while (num) {
		*dp = *sp;
		--num, ++dp, ++sp;
	}
	return dst;
}

void* memset(void* dst, const u8int val, u32int num) {
	u8int* ptr = dst;
	while (num) {
		*ptr = val;
		--num, ++ptr;
	}
	return dst;
}

int strcmp(const char* str1, const char* str2) {
	while (*str1 && (*str1 == *str2)) {
		++str1, ++str2;
	}
	return *(const u8int*)str1-*(const u8int*)str2;
}

char* strcpy(char* dst, const char* src) {
	s8int* ret = dst;
	while (*dst = *src) {
		++dst, ++src;
	}
	return ret;
}

char* strcat(char* dst, const char* src) {
	s8int* ret = dst;
	while (*dst) {
		++dst;
	}
	while (*dst = *src) {
		++dst, ++src;
	}
	return ret;
}

extern void panic(const char* message, const char* file, u32int line) {
	// We encountered a massive problem and have to stop.
	__asm__ __volatile__("cli"); // Disable interrupts.

	printf("PANIC(%s) at %s:%d\n",message, file, line);
	// Halt by going into an infinite loop.
	for(;;);
}

extern void panicAssert(const char* file, u32int line, const char* desc) {
	// An assertion failed, and we have to panic.
	__asm__ __volatile__("cli"); // Disable interrupts.

	printf("ASSERTION-FAILED(%s) at %s:%d\n", desc,file,line);
	// Halt by going into an infinite loop.
	for(;;);
}
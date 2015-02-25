// common.c -- Defines some global functions.

#include "common.h"

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
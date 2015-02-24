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
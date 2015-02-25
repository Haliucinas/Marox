// common.h -- Defines typedefs and some global functions.

#ifndef COMMON_H
#define COMMON_H

// Some nice typedefs, to standardise sizes across platforms.
// These typedefs are written for 32-bit X86.
typedef unsigned int u32int;
typedef int s32int;
typedef unsigned short u16int;
typedef short s16int;
typedef unsigned char u8int;
typedef char s8int;

// Write a byte out to the specified port.
void outb(const u16int, const u8int);
// Read a byte from the specified port.
u8int inb(const u16int);
// Read a word --/--.
u16int inw(const u16int);
// Copy len bytes from src to dest.
void* memcpy(void*, const void*, u32int);
// Write len copies of val into dest.
void* memset(void*, const u8int, u32int);
// Compare two strings. Should return -1 if 
// str1 < str2, 0 if they are equal or 1 otherwise.
int strcmp(const char*, const char*);
// Copy the NULL-terminated string src into dest, and
// return dest.
char* strcpy(char*, const char*);
// Concatenate the NULL-terminated string src onto
// the end of dest, and return dest.
char* strcat(char*, const char*);

#endif // COMMON_H
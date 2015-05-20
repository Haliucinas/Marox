#ifndef MAROX_STRING_H
#define MAROX_STRING_H

#include "marox.h"

void *memset(void *b, int c, size_t len);
void memcpy(void *dst, const void *src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
int strlen(char* s);

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);

unsigned int atoi(char*);

#endif /* MAROX_STRING_H */

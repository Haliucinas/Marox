#ifndef PRINTF_H
#define PRINTF_H

#include "stdarg.h"
#include "../include/common.h"
#include "../include/console.h"

int vsprintf(s8int*, const s8int*, va_list);

int sprintf(char *str, const char *format, ...);

int printf(const s8int*, ...);

int printfAt(const u8int, const u8int, const s8int*, ...);

#endif // PRINTF_H
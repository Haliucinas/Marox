// console.h -- Defines the interface for console.c

#ifndef CONSOLE_H
#define CONSOLE_H

#include "common.h"

#define BLACK 0x0
#define BLUE 0x1
#define GREEN 0x2
#define CYAN 0x3
#define RED 0x4
#define MAGENTA 0x5
#define BROWN 0x6
#define LIGHT_GRAY 0x7
#define DARK_GRAY 0x8
#define LIGHT_BLUE 0x9
#define LIGHT_GREEN 0xA
#define LIGHT_CYAN 0xB
#define LIGHT_RED 0xC
#define LIGHT_MAGENTA 0xD
#define LIGHT_BROWN 0xE
#define WHITE 0xF

// Clear the console to all black.
void consoleClear();

// Write a single character out to the console.
void consolePut(const char);

// Output a null-terminated ASCII string to the monitor.
void consoleWrite(const char*);

// Set text color
void consoleColorText(const u8int);

// Set background color
void consoleColorBG(const u8int);

#endif // CONSOLE_H
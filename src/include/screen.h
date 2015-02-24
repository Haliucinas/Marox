// screen.h -- Defines the interface for screen.c

#ifndef SCREEN_H
#define SCREEN_H

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

// Clear the screen to all black.
void screenClear();

// Write a single character out to the screen.
void screenPut(const char);

// Output a null-terminated ASCII string to the monitor.
void screenWrite(const char*);

void screenWriteHex(const u32int);

void screenWriteDec(const u32int);

// Set text color
void screenColorText(const u8int);

// Set background color
void screenColorBG(const u8int);

#endif // SCREEN_H
#ifndef MAROX_SCREEN_H
#define MAROX_SCREEN_H

#include "marox.h"

#define VIDEO_ADDR (0xB8000 + KERNEL_VBASE)

enum { VIDEO_ROWS = 25, VIDEO_COLS = 80 };

/* Screen device I/O ports */
enum { REG_CURSOR_HIGH = 0xE, REG_CURSOR_LOW = 0xF,
        REG_SCREEN_CTRL = 0x3D4, REG_SCREEN_DATA = 0x3D5 };

typedef enum {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    AMBER,
    GRAY,
    DGRAY,
    LBLUE,
    LGREEN,
    LCYAN,
    LRED,
    LMAGENTA,
    LBROWN,
    WHITE
} console_color_t;

enum { WHITE_ON_BLACK = 0x0F, WHITE_ON_RED = 0x4F };


void kGetChar(unsigned int *row, unsigned int *col);
void kSetCursor(unsigned int row, unsigned int col);
char kPutChar(char ch);
void kClearScreen();
void kSetGlobalAttr(uint8_t fg, uint8_t bg);

#endif /* MAROX_SCREEN_H */

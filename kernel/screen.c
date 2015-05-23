#include "screen.h"
#include "string.h"
#include "io.h"

/* global attribute for all chars printed to console */
static uint8_t g_attr = WHITE_ON_BLACK;

static unsigned int getCursorOffset();
static void setCursorOffset(unsigned int offset);
static unsigned int screenOffset(int row, int col);
static unsigned int scrollScreen(unsigned int offset);

char kPutChar(char ch) {
    char* vidmem = (char *) VIDEO_ADDR;

    unsigned int offset = getCursorOffset();

    if (ch == '\n') {
        int row = offset / (VIDEO_COLS * 2);
        offset = screenOffset(row + 1, 0);
    } else if (ch == '\b') {
        offset -= 2;
        vidmem[offset] = ' ';
        vidmem[offset+1] = g_attr;
    } else {
        vidmem[offset] = ch;
        vidmem[offset+1] = g_attr;
        offset += 2;
    }

    offset = scrollScreen(offset);

    setCursorOffset(offset);

    return ch;
}

void kGetChar(unsigned int *row, unsigned int *col) {
    unsigned int offset = getCursorOffset() / 2;
    *row = offset / VIDEO_COLS;
    *col = offset % VIDEO_COLS;
}

void kSetCursor(unsigned int row, unsigned int col) {
    unsigned int offset = screenOffset(row, col);
    setCursorOffset(offset);
}

void kClearScreen() {
    char *vidmem = (char *) VIDEO_ADDR;
    unsigned int i = 0;

    while (i < ((VIDEO_COLS * VIDEO_ROWS) * 2)) {
        // low byte = ASCII char, high byte = style
        vidmem[i++] = ' ';
        vidmem[i++] = WHITE_ON_BLACK;
    }
    
    setCursorOffset(0);
}

void kSetGlobalAttr(uint8_t fg, uint8_t bg) {
    g_attr = bg << 8 | fg;
}


static unsigned int getCursorOffset() {
    unsigned int offset = 0;
    outPortB(REG_SCREEN_CTRL, REG_CURSOR_HIGH);
    offset = inPortB(REG_SCREEN_DATA) << 8;
    outPortB(REG_SCREEN_CTRL, REG_CURSOR_LOW);
    offset += inPortB(REG_SCREEN_DATA);

    /* return offset * sizeof character cell (2) */
    return offset * 2;
}

static void setCursorOffset(unsigned int offset) {
    // convert from cell offset to char offset
    offset /= 2;
    outPortB(REG_SCREEN_CTRL, REG_CURSOR_HIGH);
    outPortB(REG_SCREEN_DATA, (uint8_t)(offset >> 8));
    outPortB(REG_SCREEN_CTRL, REG_CURSOR_LOW);
    outPortB(REG_SCREEN_DATA, (uint8_t)(offset));
}

static unsigned int screenOffset(int row, int col) {
    unsigned int offset = 0;
    offset = (row * VIDEO_COLS + col) * 2;
    return offset;
}

static unsigned int scrollScreen(unsigned int offset) {
    if (offset < VIDEO_ROWS * VIDEO_COLS * 2) {
        return offset;
    }

    // copy each row to the row above it
    for (unsigned int i = 1; i < VIDEO_ROWS; ++i) {
        memcpy((char*)(VIDEO_ADDR + screenOffset(i-1, 0)),
               (char*)(VIDEO_ADDR + screenOffset(i, 0)),
                VIDEO_COLS * 2);
    }

    // zero out last line
    char* lastLine = (char*)(VIDEO_ADDR + screenOffset(VIDEO_ROWS - 1, 0));
    for (unsigned int i = 0; i < VIDEO_COLS * 2; ++i) {
        lastLine[i] = 0;
    }

    // move cursor offset up 1 row
    offset -= 2 * VIDEO_COLS;

    return offset;
}

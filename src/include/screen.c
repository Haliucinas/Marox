// screen.c -- Defines functions for writing to the monitor.

#include "screen.h"

// The VGA framebuffer starts at 0xB8000.
u16int* videoMem = (u16int*)0xB8000;

// Color attribute 4 bits text | 4 bits background
u8int colorAttrib = (BLACK << 4) | (WHITE & 0x0F);
// Default black and white color attribute
u8int bwAttrib = (BLACK << 4) | (WHITE & 0x0F);

// Stores the cursor position.
u8int cursorX = 0;
u8int cursorY = 0;

// Updates the hardware cursor.
static void moveCursor() {
	// The screen is 80 characters wide...
	u16int cursorLocation = cursorY * 80 + cursorX;
	outb(0x3D4, 14); // Tell the VGA board we are setting the high cursor byte.
	outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
	outb(0x3D4, 15); // Tell the VGA board we are setting the low cursor byte.
	outb(0x3D5, cursorLocation); // Send the low cursor byte.
}

// Scrolls the text on the screen up by one line.
static void scroll() {
	u16int blank = 0x20 /* space */ | (bwAttrib << 8);

	// Row 25 is the end, this means we need to scroll up
	if(cursorY >= 25) {
		// Move the current text chunk that makes up the screen
		// back in the buffer by a line
		for (int i = 0; i < 24*80; ++i) {
			videoMem[i] = videoMem[i+80];
		}

		// The last line should now be blank. Do this by writing
		// 80 spaces to it.
		for (int i = 24*80; i < 25*80; ++i) {
			videoMem[i] = blank;
		}
		// The cursor should now be on the last line.
		cursorY = 24;
	}
}

// Clears the screen, by copying lots of spaces to the framebuffer.
void screenClear() {
	u16int blank = 0x20 /* space */ | (bwAttrib << 8);

	for (int i = 0; i < 80*25; ++i) {
		videoMem[i] = blank;
	}

	// Move the hardware cursor back to the start.
	cursorX = 0;
	cursorY = 0;
	moveCursor();
}

// Writes a single character out to the screen.
void screenPut(const char chr) {
	u16int* location;

	// Handle a backspace, by moving the cursor back one space
	if (chr == 0x08 && cursorX) {
		--cursorX;
	} else if (chr == 0x09) {
		// Handle a tab by increasing the cursor's X, but only to a point
		// where it is divisible by 8.
		cursorX = (cursorX+8) & ~(8-1);
	} else if (chr == 0xA) {
		// Handle newline by moving cursor back to left and increasing the row
		cursorX = 0;
		++cursorY;
	} else if (chr == 0xD) { 
		// Handle carriage return
		cursorX = 0;
	} else if(chr >= ' ') {
		// Handle any other printable character.
		location = videoMem + (cursorY*80 + cursorX);
		*location = colorAttrib << 8 | chr;
		++cursorX;
	}

	// Check if we need to insert a new line because we have reached the end
	// of the screen.
	if (cursorX >= 80) {
		cursorX = 0;
		++cursorY;
	}

	// Scroll the screen if needed.
	scroll();
	// Move the hardware cursor.
	moveCursor();
}

void screenWrite(const char* str) {
	while (*str) {
		screenPut(*(str));
		++str;
	}
}

void screenWriteHex(const u32int num) {
	if (num) {
		screenWriteHex(num / 0x10);
		u32int tmp = num % 0x10;
		if (tmp > 0x9) {
			screenPut(tmp + 0x57);
		} else {
			screenPut(tmp + 0x30);
		}
	} else {
		screenWrite("0x");
	}
}

void screenWriteDec(const u32int num) {
	if (num) {
		screenWriteDec(num / 10);
		screenPut((num % 10) + 0x30);
	}
}

void screenColorText(const u8int col) {
	colorAttrib = (colorAttrib & 0xF0) | col & 0x0F;
}

void screenColorBG(const u8int col) {
	colorAttrib = col << 4 | (colorAttrib & 0x0F);
}
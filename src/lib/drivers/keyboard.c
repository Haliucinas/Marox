#include "keyboard.h"
#include "../../include/isr.h"
#include "../printf.h"

void keyboardCallback() {
	static u8int scancode;
	u8int status;

	/* Read keyboard status */
	status = inb(0x64);
	scancode = inb(0x60);

	printf("Scan code %x %s.\n", 
		scancode & 0x7F,
		scancode & 0x80 ? "Released" : "Pressed");
}

void initKeyboard() {
	registerInterruptHandler(IRQ1, &keyboardCallback);
}
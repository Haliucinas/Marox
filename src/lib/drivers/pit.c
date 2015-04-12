// pit.c -- Initialises the PIT, and handles clock updates.

#include "pit.h"
#include "../../include/isr.h"
#include "../../include/task.h"
#include "../printf.h"

void initTimer(u32int frequency) {
	// Firstly, register our timer callback.
	registerInterruptHandler2(IRQ0, &switchTask);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	u32int divisor = 1193180 / frequency;

	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	u8int l = (u8int)(divisor & 0xFF);
	u8int h = (u8int)((divisor>>8) & 0xFF);

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);
}
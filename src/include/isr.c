// isr.c -- High level interrupt service routines and interrupt request handlers.

#include "common.h"
#include "isr.h"
#include "../lib/printf.h"

isrT interruptHandlers[256];

void registerInterruptHandler(u8int n, isrT handler) {
	interruptHandlers[n] = handler;
}

void registerInterruptHandler2(u8int n, isr2T handler) {
    interruptHandlers[n] = (isrT)handler;
}

void isrHandler(registers regs) {
	
	// This line is important. When the processor extends the 8-bit interrupt number
	// to a 32bit value, it sign-extends, not zero extends. So if the most significant
	// bit (0x80) is set, regs.intNo will be very large (about 0xffffff80).
	u8int intNo = regs.intNo & 0xFF;
	if (interruptHandlers[intNo] != 0) {
		isrT handler = interruptHandlers[intNo];
		handler(regs);
	} else {
		printf("Unhandled interrupt: no. %x\n", regs.intNo);
		PANIC("Unhandled interrupt");
    }
}

// This gets called from our ASM interrupt handler stub.
void irqHandler(registers regs) {
	// Send an EOI (end of interrupt) signal to the PICs.
	// If this interrupt involved the slave.
	if (regs.intNo >= 40) {
		// Send reset signal to slave.
		outb(0xA0, 0x20);
	}
	// Send reset signal to master. (As well as slave, if necessary).
	outb(0x20, 0x20);

	if (interruptHandlers[regs.intNo] != 0) {
		isrT handler = interruptHandlers[regs.intNo];
		handler(regs);
    }

}
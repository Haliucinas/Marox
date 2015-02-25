// isr.h -- Interface and structures for high level interrupt service routines.

#ifndef ISR_H
#define ISR_H

#include "common.h"

typedef struct {
	u32int ds; // Data segment selector
	u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	u32int intNo, errCode; // Interrupt number and error code (if applicable)
	u32int eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers;

#endif // ISR_H
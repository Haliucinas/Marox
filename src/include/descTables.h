// descTables.h - Defines the interface for initialising the GDT and IDT.
//                Also defines needed structures.

#ifndef DESCTABLES_H
#define DESCTABLES_H

#include "common.h"

// Initialisation function is publicly accessible.
// Initialisation routine - zeroes all the interrupt service routines,
// initialises the GDT and IDT.
void initDescTables();

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
struct gdtEntryStruct {
	u16int limitLow;	// The lower 16 bits of the limit.
	u16int baseLow;		// The lower 16 bits of the base.
	u8int baseMiddle;	// The next 8 bits of the base.
	u8int access;		// Access flags, determine what ring this segment can be used in.
	u8int granularity;
	u8int baseHigh;		// The last 8 bits of the base.
} __attribute__((packed));

typedef struct gdtEntryStruct gdtEntry;

// A struct describing an interrupt gate.
struct idtEntryStruct {
    u16int baseLo;		// The lower 16 bits of the address to jump to when this interrupt fires.
    u16int sel;			// Kernel segment selector.
    u8int always0;		// This must always be zero.
    u8int flags;		// More flags. See documentation.
    u16int baseHi;		// The upper 16 bits of the address to jump to.
} __attribute__((packed));

typedef struct idtEntryStruct idtEntry;

// This struct describes a *DT pointer. It points to the start of
// our array of *DT entries, and is in the format required by the
// lgdt and lidt instruction.
struct ptrStruct {
	u16int limit;			// The upper 16 bits of all selector limits.
	u32int base;			// The address of the first gdt_entry_t struct.
} __attribute__((packed));

typedef struct ptrStruct dtPtr;

// These extern directives let us access the addresses of our ASM ISR handlers.
extern void isr0(); // Division by zero exception
extern void isr1(); // Debug exception
extern void isr2(); // Non maskable interrupt
extern void isr3(); // Breakpoint exception
extern void isr4(); // 'Into detected overflow'
extern void isr5(); // Out of bounds exception
extern void isr6(); // Invalid opcode exception
extern void isr7(); // No coprocessor exception
extern void isr8(); // Double fault (pushes an error code)
extern void isr9(); // Coprocessor segment overrun
extern void isr10(); // Bad TSS (pushes an error code)
extern void isr11(); // Segment not present (pushes an error code)
extern void isr12(); // Stack fault (pushes an error code)
extern void isr13(); // General protection fault (pushes an error code)
extern void isr14(); // Page fault (pushes an error code)
extern void isr15(); // Unknown interrupt exception
extern void isr16(); // Coprocessor fault
extern void isr17(); // Alignment check exception
extern void isr18(); // Machine check exception
// These are all reserved
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#endif // DESCTABLES_H
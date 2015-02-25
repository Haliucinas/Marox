// descTables.c - Initialises the GDT and IDT, and defines the 
//                default ISR and IRQ handler.

#include "descTables.h"

// Lets us access our ASM functions from our C code.
extern void gdtFlush(const u32int);

// Internal function prototypes.
static void initGdt();
static void gdtSetGate(const s32int, const u32int, const u32int, const u8int, const u8int);

gdtEntry gdtEntries[5];
dtPtr gdtPtr;

void initDescTables() {
	// Initialise the global descriptor table.
	initGdt();
}

static void initGdt() {
	gdtPtr.limit = (sizeof(gdtEntry)*5)-1;
	gdtPtr.base = (u32int)&gdtEntries;

	memset(&gdtEntries, 0, sizeof(gdtEntry)*5);

	gdtSetGate(0, 0, 0, 0, 0); // Null segment
	gdtSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
	gdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
	gdtSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
	gdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

	gdtFlush((u32int)&gdtPtr);
}

static void gdtSetGate(const s32int num, const u32int base, const u32int limit, const u8int access, const u8int gran) {
	gdtEntries[num].baseLow = base & 0xFFFF;
	gdtEntries[num].baseMiddle = (base >> 16) & 0xFF;
	gdtEntries[num].baseHigh = (base >> 24) & 0xFF;

	gdtEntries[num].limitLow = limit & 0xFFFF;
	gdtEntries[num].granularity = (limit >> 16) & 0x0F;

	gdtEntries[num].granularity |= gran & 0xF0;
	gdtEntries[num].access = access;
}
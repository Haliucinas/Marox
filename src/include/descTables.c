// descTables.c - Initialises the GDT and IDT, and defines the 
//                default ISR and IRQ handler.

#include "descTables.h"

// Lets us access our ASM functions from our C code.
extern void gdtFlush(const u32int);
extern void idtFlush(const u32int);

// Internal function prototypes.
static void initGdt();
static void initIdt();
static void gdtSetGate(const s32int, const u32int, const u32int, const u8int, const u8int);
static void idtSetGate(const u8int, const u32int, const u16int, const u8int);

gdtEntry gdtEntries[5];
idtEntry idtEntries[256];
dtPtr gdtPtr, idtPtr;

void initDescTables() {
	// Initialise the global descriptor table.
	initGdt();
	// Initialise the interrupt descriptor table.
	initIdt();
}

static void initGdt() {
	gdtPtr.limit = (sizeof(gdtEntry)*5)-1;
	gdtPtr.base = (u32int)&gdtEntries;

	memset(&gdtEntries, 0, sizeof(gdtEntry)*5);

	gdtSetGate(0, 0, 0, 0, 0); // Null segment. This must be present, or bad things will happen.
	gdtSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
	gdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
	gdtSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
	gdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

	gdtFlush((u32int)&gdtPtr);
}

static void initIdt() {
	idtPtr.limit = (sizeof(idtEntry)*256)-1;
	idtPtr.base = (u32int)&idtEntries;

	memset(&idtEntries, 0, sizeof(idtEntry)*256);

	// Remap the irq table.
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

	idtSetGate(0, (u32int)isr0 , 0x08, 0x8E);
	idtSetGate(1, (u32int)isr1 , 0x08, 0x8E);
	idtSetGate(2, (u32int)isr2 , 0x08, 0x8E);
	idtSetGate(3, (u32int)isr3 , 0x08, 0x8E);
	idtSetGate(4, (u32int)isr4 , 0x08, 0x8E);
	idtSetGate(5, (u32int)isr5 , 0x08, 0x8E);
	idtSetGate(6, (u32int)isr6 , 0x08, 0x8E);
	idtSetGate(7, (u32int)isr7 , 0x08, 0x8E);
	idtSetGate(8, (u32int)isr8 , 0x08, 0x8E);
	idtSetGate(9, (u32int)isr9 , 0x08, 0x8E);
	idtSetGate(10, (u32int)isr10, 0x08, 0x8E);
	idtSetGate(11, (u32int)isr11, 0x08, 0x8E);
	idtSetGate(12, (u32int)isr12, 0x08, 0x8E);
	idtSetGate(13, (u32int)isr13, 0x08, 0x8E);
	idtSetGate(14, (u32int)isr14, 0x08, 0x8E);
	idtSetGate(15, (u32int)isr15, 0x08, 0x8E);
	idtSetGate(16, (u32int)isr16, 0x08, 0x8E);
	idtSetGate(17, (u32int)isr17, 0x08, 0x8E);
	idtSetGate(18, (u32int)isr18, 0x08, 0x8E);
	idtSetGate(19, (u32int)isr19, 0x08, 0x8E);
	idtSetGate(20, (u32int)isr20, 0x08, 0x8E);
	idtSetGate(21, (u32int)isr21, 0x08, 0x8E);
	idtSetGate(22, (u32int)isr22, 0x08, 0x8E);
	idtSetGate(23, (u32int)isr23, 0x08, 0x8E);
	idtSetGate(24, (u32int)isr24, 0x08, 0x8E);
	idtSetGate(25, (u32int)isr25, 0x08, 0x8E);
	idtSetGate(26, (u32int)isr26, 0x08, 0x8E);
	idtSetGate(27, (u32int)isr27, 0x08, 0x8E);
	idtSetGate(28, (u32int)isr28, 0x08, 0x8E);
	idtSetGate(29, (u32int)isr29, 0x08, 0x8E);
	idtSetGate(30, (u32int)isr30, 0x08, 0x8E);
	idtSetGate(31, (u32int)isr31, 0x08, 0x8E);
	idtSetGate(32, (u32int)irq0, 0x08, 0x8E);
	idtSetGate(33, (u32int)irq1, 0x08, 0x8E);
	idtSetGate(34, (u32int)irq2, 0x08, 0x8E);
	idtSetGate(35, (u32int)irq3, 0x08, 0x8E);
	idtSetGate(36, (u32int)irq4, 0x08, 0x8E);
	idtSetGate(37, (u32int)irq5, 0x08, 0x8E);
	idtSetGate(38, (u32int)irq6, 0x08, 0x8E);
	idtSetGate(39, (u32int)irq7, 0x08, 0x8E);
	idtSetGate(40, (u32int)irq8, 0x08, 0x8E);
	idtSetGate(41, (u32int)irq9, 0x08, 0x8E);
	idtSetGate(42, (u32int)irq10, 0x08, 0x8E);
	idtSetGate(43, (u32int)irq11, 0x08, 0x8E);
	idtSetGate(44, (u32int)irq12, 0x08, 0x8E);
	idtSetGate(45, (u32int)irq13, 0x08, 0x8E);
	idtSetGate(46, (u32int)irq14, 0x08, 0x8E);
	idtSetGate(47, (u32int)irq15, 0x08, 0x8E);

	idtFlush((u32int)&idtPtr);
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

static void idtSetGate(const u8int num, const u32int base, const u16int sel, const u8int flags) {
	idtEntries[num].baseLo = base & 0xFFFF;
	idtEntries[num].baseHi = (base >> 16) & 0xFFFF;

	idtEntries[num].sel = sel;
	idtEntries[num].always0 = 0;
	// We must uncomment the OR below when we get to using user-mode.
	// It sets the interrupt gate's privilege level to 3.
	idtEntries[num].flags = flags /* | 0x60 */;
}
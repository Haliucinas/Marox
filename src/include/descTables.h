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
	u16int limitLow;		// The lower 16 bits of the limit.
	u16int baseLow;		// The lower 16 bits of the base.
	u8int baseMiddle;		// The next 8 bits of the base.
	u8int access;			// Access flags, determine what ring this segment can be used in.
	u8int granularity;
	u8int baseHigh;		// The last 8 bits of the base.
} __attribute__((packed));

typedef struct gdtEntryStruct gdtEntry;

// This struct describes a *DT pointer. It points to the start of
// our array of *DT entries, and is in the format required by the
// lgdt and lidt instruction.
struct ptrStruct {
	u16int limit;			// The upper 16 bits of all selector limits.
	u32int base;			// The address of the first gdt_entry_t struct.
} __attribute__((packed));

typedef struct ptrStruct dtPtr;

#endif // DESCTABLES_H
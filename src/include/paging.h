// paging.h -- Defines the interface for and structures relating to paging.

#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "isr.h"

#define STACK_TOP 0xE0000000

typedef struct {
	u32int present    : 1;   // Page present in memory
	u32int rw         : 1;   // Read-only if clear, readwrite if set
	u32int user       : 1;   // Supervisor level only if clear
	u32int accessed   : 1;   // Has the page been accessed since last refresh?
	u32int dirty      : 1;   // Has the page been written to since last refresh?
	u32int unused     : 7;   // Amalgamation of unused and reserved bits
	u32int frame      : 20;  // Frame address (shifted right 12 bits)
} pageT;

typedef struct {
	pageT pages[1024];
} pageTableT;

typedef struct {
	/*
		Array of pointers to pagetables.
	*/
	pageTableT* tables[1024];
	/*
		Array of pointers to the pagetables above, but gives their *physical*
		location, for loading into the CR3 register.
	*/
	u32int tablesPhysical[1024];

	/*
		The physical address of tablesPhysical. This comes into play
		when we get our kernel heap allocated and the directory
		may be in a different location in virtual memory.
	*/
	u32int physicalAddr;
} pageDirT;

extern void copyPagePhysical(u32int source, u32int destination);

/*
	Sets up the environment, page directories etc and
	enables paging.
*/
void initPaging();

/*
	Causes the specified page directory to be loaded into the
	CR3 register.
*/
void switchPageDir(pageDirT*);

/*
	Retrieves a pointer to the page required.
	If make == 1, if the page-table in which this page should
	reside isn't created, create it!
*/
pageT* getPage(u32int, const int, pageDirT*);

/*
	Handler for page faults.
*/
void pageFault(registers);

/*
	Makes a copy of a page directory.
*/
pageDirT* cloneDir(pageDirT*);

#endif // PAGING_H
// paging.c -- Defines the interface for and structures relating to paging.

#include "paging.h"
#include "../lib/kheap.h"
#include "../lib/printf.h"

// The kernel's page directory
pageDirT* kernelDir = 0;

// The current page directory;
pageDirT* currentDir = 0;

// A bitset of frames - used or free.
u32int* frames;
u32int nframes;

// Defined in kheap.c
extern u32int placementAddr;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the frames bitset
static void setFrame(const u32int frameAddr) {
	u32int frame = frameAddr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clearFrame(const u32int frameAddr) {
	u32int frame = frameAddr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static u32int testFrame(const u32int frameAddr) {
	u32int frame = frameAddr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static u32int firstFrame() {
	for (int i = 0; i < INDEX_FROM_BIT(nframes); ++i) {
		if (frames[i] != 0xFFFFFFFF) { // nothing free, exit early.
			// at least one bit is free here.
			for (int j = 0; j < 32; ++j) {
				u32int toTest = 0x1 << j;
				if (!(frames[i]&toTest)) {
					return i*4*8+j;
				}
			}
		}
	}
}

// Function to allocate a frame.
void allocFrame(pageT* page, const int isKernel, const int isWriteable) {
	if (page->frame != 0) {
		return;
	} else {
		u32int idx = firstFrame();
		if (idx == (u32int)-1) {
			PANIC("No free frames");
		}
		setFrame(idx*0x1000);
		page->present = 1;
		page->rw = (isWriteable) ? 1 : 0;
		page->user = (isKernel) ? 0 : 1;
		page->frame = idx;
	}
}

// Function to deallocate a frame.
void freeFrame(pageT* page) {
	u32int frame;
	if (!(frame=page->frame)) {
		return;
	} else {
		clearFrame(frame);
		page->frame = 0x0;
	}
}

void initPaging() {
	// The size of physical memory. For the moment we 
	// assume it is 16MB big.
	u32int memEndPage = 0x1000000;

	nframes = memEndPage / 0x1000;
	frames = (u32int*)kNew(INDEX_FROM_BIT(nframes));
	memset(frames, 0, INDEX_FROM_BIT(nframes));

	// Let's make a page directory.
	kernelDir = (pageDirT*)kNewA(sizeof(pageDirT));
	currentDir = kernelDir;

	// We need to identity map (phys addr = virt addr) from
	// 0x0 to the end of used memory, so we can access this
	// transparently, as if paging wasn't enabled.
	// NOTE that we use a while loop here deliberately.
	// inside the loop body we actually change placement_address
	// by calling kmalloc(). A while loop causes this to be
	// computed on-the-fly rather than once at the start.
	int i = 0;
	while (i < placementAddr) {
		// Kernel code is readable but not writeable from userspace.
		allocFrame(getPage(i, 1, kernelDir), 0, 0);
		i += 0x1000;
	}
	// Before we enable paging, we must register our page fault handler.
	registerInterruptHandler(14, pageFault);

	// Now, enable paging!
	switchPageDir(kernelDir);
}

void switchPageDir(pageDirT* dir) {
    currentDir = dir;
    __asm__ __volatile__("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    u32int cr0;
    __asm__ __volatile__("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    __asm__ __volatile__("mov %0, %%cr0":: "r"(cr0));
}

pageT* getPage(u32int address, const int make, pageDirT* dir) {
	// Turn the address into an index.
	address /= 0x1000;
	// Find the page table containing this address.
	u32int tableIdx = address / 1024;
	if (dir->tables[tableIdx]) { // If this table is already assigned
		return &dir->tables[tableIdx]->pages[address%1024];
	} else if (make) {
		u32int tmp;
		dir->tables[tableIdx] = (pageTableT*)kNewAP(sizeof(pageTableT), &tmp);
		dir->tablesPhysical[tableIdx] = tmp | 0x7; // PRESENT, RW, US.
		return &dir->tables[tableIdx]->pages[address%1024];
	} else {
		return 0;
	}
}


void pageFault(registers regs) {
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	u32int faultingAddr;
	__asm__ __volatile__("mov %%cr2, %0" : "=r" (faultingAddr));

	// The error code gives us details of what happened.
	int present = !(regs.errCode & 0x1); // Page not present
	int rw = regs.errCode & 0x2;         // Write operation?
	int us = regs.errCode & 0x4;         // Processor was in user-mode?
	int reserved = regs.errCode & 0x8;   // Overwritten CPU-reserved bits of page entry?
	int id = regs.errCode & 0x10;        // Caused by an instruction fetch?

	// Output an error message.
	printf("Page fault! ( ");
	if (present) {printf("present ");}
	if (rw) {printf("read-only ");}
	if (us) {printf("user-mode ");}
	if (reserved) {printf("reserved ");}
	printf(") at ");
	printf("%x\n", faultingAddr);
	PANIC("Page fault");
}
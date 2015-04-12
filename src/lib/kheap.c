// kheap.c -- Kernel heap functions, also provides
//            a placement malloc() for use before the heap is 

#include "kheap.h"

// end is defined in the linker script.
extern u32int end;
u32int placementAddr = (u32int)&end;
// Defined in kernel.c
extern u32int maxHeapSize;

u32int kNewInternal(const u32int sz, const int align, u32int *physAddr) {
	// This will eventually call malloc() on the kernel heap.
	// For now, though, we just assign memory at placement_address
	// and increment it by sz. Even when we've coded our kernel
	// heap, this will be useful for use before the heap is initialised.
	if (align == 1 && (placementAddr & 0x00000FFF)) {
		// Align the placement address;
		placementAddr &= 0xFFFFF000;
		placementAddr += 0x1000;
	}
	if (physAddr) {
		*physAddr = placementAddr;
	}
	u32int tmp = placementAddr;
	if (tmp >= (u32int)&end + maxHeapSize) {
        PANIC("Heap too large");
    }
	placementAddr += sz;
	return tmp;
}

u32int kNewA(const u32int sz) {
	return kNewInternal(sz, 1, 0);
}

u32int kNewP(const u32int sz, u32int* phys) {
	return kNewInternal(sz, 0, phys);
}

u32int kNewAP(const u32int sz, u32int* phys) {
	return kNewInternal(sz, 1, phys);
}

u32int kNew(const u32int sz) {
	return kNewInternal(sz, 0, 0);
}
// kheap.h -- Interface for kernel heap functions, also provides
//            a placement malloc() for use before the heap is 

#ifndef KHEAP_H
#define KHEAP_H

#include "../include/common.h"

/*
	Allocate a chunk of memory, sz in size. If align == 1,
	the chunk must be page-aligned. If phys != 0, the physical
	location of the allocated chunk will be stored into phys.

	This is the internal version of kmalloc. More user-friendly
	parameter representations are available in kmalloc, kmalloc_a,
	kmalloc_ap, kmalloc_p.
*/
u32int kNewInternal(const u32int, const int, u32int*);

/*
	Allocate a chunk of memory, sz in size. The chunk must be
	page aligned.
*/
u32int kNewA(const u32int);

/*
	Allocate a chunk of memory, sz in size. The physical address
	is returned in phys. Phys MUST be a valid pointer to u32int!
*/
u32int kNewP(const u32int, u32int*);

/*
	Allocate a chunk of memory, sz in size. The physical address 
	is returned in phys. It must be page-aligned.
*/
u32int kNewAP(const u32int, u32int*);

/*
	General allocation function.
*/
u32int kNew(const u32int);

#endif // KHEAP_H
// kheap.h -- Interface for kernel heap functions, also provides
//			a placement malloc() for use before the heap is 
//			initialised.

#ifndef KHEAP_H
#define KHEAP_H

#include "../include/common.h"
#include "../include/orderedArray.h"
#include "../include/paging.h"

#define KHEAP_START 0xC0000000
#define KHEAP_INITIAL_SIZE 0x100000

#define HEAP_INDEX_SIZE 0x20000
#define HEAP_MAGIC 0x123890AB
#define HEAP_MIN_SIZE 0x70000

/*
	Size information for a hole/block
*/
typedef struct {
	u32int magic; // Magic number, used for error checking and identification.
	u8int isHole; // 1 if this is a hole. 0 if this is a block.
	u32int size; // size of the block, including the end footer.
} headerT;

typedef struct {
	u32int magic; // Magic number, same as in header_t.
	headerT* header; // Pointer to the block header.
} footerT;

typedef struct {
	orderedArrayT index;
	u32int startAddr; // The start of our allocated space.
	u32int endAddr; // The end of our allocated space. May be expanded up to max_address.
	u32int maxAddr; // The maximum address the heap can be expanded to.
	u8int supervisor; // Should extra pages requested by us be mapped as supervisor-only?
	u8int readonly; // Should extra pages requested by us be mapped as read-only?
} heapT;

/*
	Create a new heap.
*/
heapT* createHeap(u32int, u32int, u32int, u8int, u8int);

/*
	Allocates a contiguous region of memory 'size' in size. If page_align==1, it creates that block starting
	on a page boundary.
*/
void* new(u32int, u8int, heapT*);

/*
	Releases a block allocated with 'alloc'.
*/
void free(void* p, heapT* heap);

/*
	Allocate a chunk of memory, sz in size. If align == 1,
	the chunk must be page-aligned. If phys != 0, the physical
	location of the allocated chunk will be stored into phys.

	This is the internal version of kNew. More user-friendly
	parameter representations are available in kNew, kNewA,
	kNewAP, kNewP.
*/
u32int kNewInternal(u32int sz, int align, u32int* phys);

/*
	Allocate a chunk of memory, sz in size. The chunk must be
	page aligned.
*/
u32int kNewA(u32int sz);

/*
	Allocate a chunk of memory, sz in size. The physical address
	is returned in phys. Phys MUST be a valid pointer to u32int!
*/
u32int kNewP(u32int sz, u32int* phys);

/*
	Allocate a chunk of memory, sz in size. The physical address 
	is returned in phys. It must be page-aligned.
*/
u32int kNewAP(u32int sz, u32int* phys);

/*
	General allocation function.
*/
u32int kNew(u32int sz);

/*
	General deallocation function.
*/
void kFree(void* p);

#endif // KHEAP_H

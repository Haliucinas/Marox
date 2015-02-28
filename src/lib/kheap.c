// kheap.c -- Kernel heap functions, also provides
//			a placement malloc() for use before the heap is 
//			initialised.

#include "kheap.h"

// end is defined in the linker script.
extern u32int end;
u32int placementAddr = (u32int)&end;
extern pageDirT* kernelDir;
heapT* kheap = 0;

u32int kNewInternal(u32int sz, int align, u32int* phys) {
	if (kheap != 0) {
		void* addr = new(sz, (u8int)align, kheap);
		if (phys != 0) {
			pageT* page = getPage((u32int)addr, 0, kernelDir);
			*phys = page->frame*0x1000 + (u32int)addr & 0xFFF;
		}
		return (u32int)addr;
	} else {
		if (align == 1 && (placementAddr & 0xFFFFF000)) {
			// Align the placement address;
			placementAddr &= 0xFFFFF000;
			placementAddr += 0x1000;
		} if (phys) {
			*phys = placementAddr;
		}
		u32int tmp = placementAddr;
		placementAddr += sz;
		return tmp;
	}
}

void kFree(void* p) {
	free(p, kheap);
}

u32int kNewA(u32int sz) {
	return kNewInternal(sz, 1, 0);
}

u32int kNewP(u32int sz, u32int* phys) {
	return kNewInternal(sz, 0, phys);
}

u32int kNewAP(u32int sz, u32int* phys) {
	return kNewInternal(sz, 1, phys);
}

u32int kNew(u32int sz) {
	return kNewInternal(sz, 0, 0);
}

static void expand(u32int newSize, heapT* heap) {
	// Sanity check.
	ASSERT(newSize > heap->endAddr - heap->startAddr);

	// Get the nearest following page boundary.
	if (newSize & 0xFFFFF000 != 0) {
		newSize &= 0xFFFFF000;
		newSize += 0x1000;
	}

	// Make sure we are not overreaching ourselves.
	ASSERT(heap->startAddr+newSize <= heap->maxAddr);

	// This should always be on a page boundary.
	u32int oldSize = heap->endAddr-heap->startAddr;

	u32int i = oldSize;
	while (i < newSize) {
		newFrame(getPage(heap->startAddr+i, 1, kernelDir), (heap->supervisor) ? 1 : 0, (heap->readonly) ? 0 : 1);
		i += 0x1000 /* page size */;
	}
	heap->endAddr = heap->startAddr+newSize;
}

static u32int contract(u32int newSize, heapT* heap) {
	// Sanity check.
	ASSERT(newSize < heap->endAddr-heap->startAddr);

	// Get the nearest following page boundary.
	if (newSize & 0x1000) {
		newSize &= 0x1000;
		newSize += 0x1000;
	}

	// Don't contract too far!
	if (newSize < HEAP_MIN_SIZE) {
		newSize = HEAP_MIN_SIZE;
	}

	u32int oldSize = heap->endAddr-heap->startAddr;
	u32int i = oldSize - 0x1000;
	while (newSize < i) {
		freeFrame(getPage(heap->startAddr+i, 0, kernelDir));
		i -= 0x1000;
	}

	heap->endAddr = heap->startAddr + newSize;
	return newSize;
}

static s32int findSmallestHole(u32int size, u8int pageAlign, heapT* heap) {
	// Find the smallest hole that will fit.
	u32int iterator = 0;
	while (iterator < heap->index.size) {
		headerT* header = (headerT*)lookupOrderedArray(iterator, &heap->index);
		// If the user has requested the memory be page-aligned
		if (pageAlign > 0) {
			// Page-align the starting point of this header.
			u32int location = (u32int)header;
			s32int offset = 0;
			if ((location+sizeof(headerT)) & 0xFFFFF000 != 0) {
				offset = 0x1000 /* page size */  - (location+sizeof(headerT))%0x1000;
			}
			s32int holeSize = (s32int)header->size - offset;
			// Can we fit now?
			if (holeSize >= (s32int)size) {
				break;
			}
		} else if (header->size >= size) {
			break;
		}
		++iterator;
	}
	// Why did the loop exit?
	if (iterator == heap->index.size) {
		return -1; // We got to the end and didn't find anything.
	} else {
		return iterator;
	}
}

static s8int headerTLessThan(void* a, void* b) {
	return (((headerT*)a)->size < ((headerT*)b)->size) ? 1 : 0;
}

heapT* createHeap(u32int start, u32int endAddr, u32int max, u8int supervisor, u8int readonly) {
	heapT* heap = (heapT*)kNew(sizeof(heapT));

	// All our assumptions are made on startAddress and endAddress being page-aligned.
	ASSERT(start % 0x1000 == 0);
	ASSERT(endAddr % 0x1000 == 0);
	
	// Initialise the index.
	heap->index = placeOrderedArray((void*)start, HEAP_INDEX_SIZE, &headerTLessThan);
	
	// Shift the start address forward to resemble where we can start putting data.
	start += sizeof(typeT)*HEAP_INDEX_SIZE;

	// Make sure the start address is page-aligned.
	if (start & 0xFFFFF000 != 0) {
		start &= 0xFFFFF000;
		start += 0x1000;
	}
	// Write the start, end and max addresses into the heap structure.
	heap->startAddr = start;
	heap->endAddr = endAddr;
	heap->maxAddr = max;
	heap->supervisor = supervisor;
	heap->readonly = readonly;

	// We start off with one large hole in the index.
	headerT *hole = (headerT *)start;
	hole->size = endAddr-start;
	hole->magic = HEAP_MAGIC;
	hole->isHole = 1;
	insertOrderedArray((void*)hole, &heap->index);	 

	return heap;
}

void* new(u32int size, u8int pageAlign, heapT *heap) {

	// Make sure we take the size of header/footer into account.
	u32int newSize = size + sizeof(headerT) + sizeof(footerT);
	// Find the smallest hole that will fit.
	s32int iterator = findSmallestHole(newSize, pageAlign, heap);

	if (iterator == -1) { // If we didn't find a suitable hole
		// Save some previous data.
		u32int oldLength = heap->endAddr - heap->startAddr;
		u32int oldEndAddr = heap->endAddr;

		// We need to allocate some more space.
		expand(oldLength+newSize, heap);
		u32int newLength = heap->endAddr-heap->startAddr;

		// Find the endmost header. (Not endmost in size, but in location).
		iterator = 0;
		// Vars to hold the index of, and value of, the endmost header found so far.
		u32int idx = -1; u32int value = 0x0;
		while (iterator < heap->index.size) {
			u32int tmp = (u32int)lookupOrderedArray(iterator, &heap->index);
			if (tmp > value) {
				value = tmp;
				idx = iterator;
			}
			++iterator;
		}

		// If we didn't find ANY headers, we need to add one.
		if (idx == -1) {
			headerT* header = (headerT*)oldEndAddr;
			header->magic = HEAP_MAGIC;
			header->size = newLength - oldLength;
			header->isHole = 1;
			footerT* footer = (footerT*) (oldEndAddr + header->size - sizeof(footerT));
			footer->magic = HEAP_MAGIC;
			footer->header = header;
			insertOrderedArray((void*)header, &heap->index);
		} else {
			// The last header needs adjusting.
			headerT* header = lookupOrderedArray(idx, &heap->index);
			header->size += newLength - oldLength;
			// Rewrite the footer.
			footerT* footer = (footerT*)((u32int)header + header->size - sizeof(footerT) );
			footer->header = header;
			footer->magic = HEAP_MAGIC;
		}
		// We now have enough space. Recurse, and call the function again.
		return new(size, pageAlign, heap);
	}

	headerT* origHoleHeader = (headerT*)lookupOrderedArray(iterator, &heap->index);
	u32int origHolePos = (u32int)origHoleHeader;
	u32int origHoleSize = origHoleHeader->size;
	// Here we work out if we should split the hole we found into two parts.
	// Is the original hole size - requested hole size less than the overhead for adding a new hole?
	if (origHoleSize-newSize < sizeof(headerT)+sizeof(footerT)) {
		// Then just increase the requested size to the size of the hole we found.
		size += origHoleSize-newSize;
		newSize = origHoleSize;
	}

	// If we need to page-align the data, do it now and make a new hole in front of our block.
	if (pageAlign && origHolePos & 0xFFFFF000) {
		u32int newLocation = origHolePos + 0x1000 /* page size */ - (origHolePos&0xFFF) - sizeof(headerT);
		headerT* holeHeader = (headerT*)origHolePos;
		holeHeader->size = 0x1000 /* page size */ - (origHolePos&0xFFF) - sizeof(headerT);
		holeHeader->magic = HEAP_MAGIC;
		holeHeader->isHole = 1;
		footerT* holeFooter = (footerT*)((u32int)newLocation - sizeof(footerT) );
		holeFooter->magic = HEAP_MAGIC;
		holeFooter->header = holeHeader;
		origHolePos	= newLocation;
		origHoleSize = origHoleSize - holeHeader->size;
	} else {
		// Else we don't need this hole any more, delete it from the index.
		removeOrderedArray(iterator, &heap->index);
	}

	// Overwrite the original header...
	headerT* blockHeader = (headerT*)origHolePos;
	blockHeader->magic = HEAP_MAGIC;
	blockHeader->isHole = 0;
	blockHeader->size = newSize;
	// ...And the footer
	footerT* blockFooter = (footerT*)(origHolePos + sizeof(headerT) + size);
	blockFooter->magic = HEAP_MAGIC;
	blockFooter->header	= blockHeader;

	// We may need to write a new hole after the allocated block.
	// We do this only if the new hole would have positive size...
	if (origHoleSize - newSize > 0) {
		headerT *holeHeader = (headerT*) (origHolePos + sizeof(headerT) + size + sizeof(footerT));
		holeHeader->magic = HEAP_MAGIC;
		holeHeader->isHole = 1;
		holeHeader->size = origHoleSize - newSize;
		footerT* holeFooter = (footerT*)((u32int)holeHeader + origHoleSize - newSize - sizeof(footerT) );
		if ((u32int)holeFooter < heap->endAddr) {
			holeFooter->magic = HEAP_MAGIC;
			holeFooter->header = holeHeader;
		}
		// Put the new hole in the index;
		insertOrderedArray((void*)holeHeader, &heap->index);
	}
	
	// ...And we're done!
	return (void *)((u32int)blockHeader+sizeof(headerT));
}

void free(void* p, heapT* heap) {
	// Exit gracefully for null pointers.
	if (p == 0) {
		return;
	}

	// Get the header and footer associated with this pointer.
	headerT* header = (headerT*)((u32int)p - sizeof(headerT));
	footerT* footer = (footerT*)((u32int)header + header->size - sizeof(footerT));

	// Sanity checks.
	ASSERT(header->magic == HEAP_MAGIC);
	ASSERT(footer->magic == HEAP_MAGIC);

	// Make us a hole.
	header->isHole = 1;

	// Do we want to add this header into the 'free holes' index?
	char doAdd = 1;

	// Unify left
	// If the thing immediately to the left of us is a footer...
	footerT* testFooter = (footerT*) ( (u32int)header - sizeof(footerT) );
	if (testFooter->magic == HEAP_MAGIC && testFooter->header->isHole == 1) {
		u32int cacheSize = header->size; // Cache our current size.
		header = testFooter->header;	 // Rewrite our header with the new one.
		footer->header = header;		  // Rewrite our footer to point to the new header.
		header->size += cacheSize;	   // Change the size.
		doAdd = 0;					   // Since this header is already in the index, we don't want to add it again.
	}

	// Unify right
	// If the thing immediately to the right of us is a header...
	headerT* testHeader = (headerT*) ( (u32int)footer + sizeof(footerT) );
	if (testHeader->magic == HEAP_MAGIC && testHeader->isHole) {
		header->size += testHeader->size; // Increase our size.
		testFooter = (footerT*)((u32int)testHeader + // Rewrite it's footer to point to our header.
									testHeader->size - sizeof(footerT));
		footer = testFooter;
		// Find and remove this header from the index.
		u32int iterator = 0;
		while ((iterator < heap->index.size) && (lookupOrderedArray(iterator, &heap->index) != (void*)testHeader)) {
			iterator++;
		}

		// Make sure we actually found the item.
		ASSERT(iterator < heap->index.size);
		// Remove it.
		removeOrderedArray(iterator, &heap->index);
	}

	// If the footer location is the end address, we can contract.
	if ((u32int)footer+sizeof(footerT) == heap->endAddr) {
		u32int oldLength = heap->endAddr-heap->startAddr;
		u32int newLength = contract( (u32int)header - heap->startAddr, heap);
		// Check how big we will be after resizing.
		if (header->size - (oldLength-newLength) > 0) {
			// We will still exist, so resize us.
			header->size -= oldLength-newLength;
			footer = (footerT*) ( (u32int)header + header->size - sizeof(footerT) );
			footer->magic = HEAP_MAGIC;
			footer->header = header;
		} else {
			// We will no longer exist :(. Remove us from the index.
			u32int iterator = 0;
			while ((iterator < heap->index.size) && (lookupOrderedArray(iterator, &heap->index) != (void*)testHeader)) {
				iterator++;
			}
			// If we didn't find ourselves, we have nothing to remove.
			if (iterator < heap->index.size) {
				removeOrderedArray(iterator, &heap->index);
			}
		}
	}

	// If required, add us to the index.
	if (doAdd == 1) {
		insertOrderedArray((void*)header, &heap->index);
	}

}

// orderedArray.c -- Implementation for creating, inserting and deleting
//				   from ordered arrays.

#include "orderedArray.h"
#include "../lib/kheap.h"

s8int standardLessThanPredicate(typeT a, typeT b) {
	return (a < b) ? 1 : 0;
}

orderedArrayT createOrderedArray(u32int maxSize, lessThanPredicateT lessThan) {
	orderedArrayT teRet;
	teRet.array = (void*)kNew(maxSize*sizeof(typeT));
	memset(teRet.array, 0, maxSize*sizeof(typeT));
	teRet.size = 0;
	teRet.maxSize = maxSize;
	teRet.lessThan = lessThan;
	return teRet;
}

orderedArrayT placeOrderedArray(void* addr, u32int maxSize, lessThanPredicateT lessThan) {
	orderedArrayT teRet;
	teRet.array = (typeT*)addr;
	memset(teRet.array, 0, maxSize*sizeof(typeT));
	teRet.size = 0;
	teRet.maxSize = maxSize;
	teRet.lessThan = lessThan;
	return teRet;
}

void destroyOrderedArray(orderedArrayT* array) {
	kFree(array->array);
}

void insertOrderedArray(typeT item, orderedArrayT* array) {
	ASSERT(array->lessThan);
	u32int iterator = 0;
	while (iterator < array->size && array->lessThan(array->array[iterator], item)) {
		iterator++;
	}
	if (iterator == array->size) { // just add at the end of the array.
		array->array[array->size++] = item;
	} else {
		typeT tmp = array->array[iterator];
		array->array[iterator] = item;
		while (iterator < array->size) {
			++iterator;
			typeT tmp2 = array->array[iterator];
			array->array[iterator] = tmp;
			tmp = tmp2;
		}
		++(array->size);
	}
}

typeT lookupOrderedArray(u32int i, orderedArrayT* array) {
	ASSERT(i < array->size);
	return array->array[i];
}

void removeOrderedArray(u32int i, orderedArrayT* array) {
	while (i < array->size) {
		array->array[i] = array->array[i+1];
		++i;
	}
	--(array->size);
}

// ordered_array.c -- Implementation for creating, inserting and deleting
//                    from ordered arrays.

#include "orderedArray.h"
#include "../lib/kheap.h"

s8int standardLessthanPredicate(const typeT a, const typeT b) {
	return (a < b) ? 1 : 0;
}

orderedArrayT createOrderedArray(const u32int maxSize, const lessthanPredicateT lessThan) {
	orderedArrayT toRet;
	toRet.array = (void*)kNew(maxSize*sizeof(typeT));
	memset(toRet.array, 0, maxSize*sizeof(typeT));
	toRet.size = 0;
	toRet.maxSize = maxSize;
	toRet.lessThan = lessThan;
	return toRet;
}

orderedArrayT placeOrderedArray(const void* addr, const u32int maxSize, const lessthanPredicateT lessThan) {
	orderedArrayT toRet;
	toRet.array = (typeT*)addr;
	memset(toRet.array, 0, maxSize*sizeof(typeT));
	toRet.size = 0;
	toRet.maxSize = maxSize;
	toRet.lessThan = lessThan;
	return toRet;
}

void destroyOrderedArray(orderedArrayT* array) {
	//kfree(array->array);
}

void insertOrderedArray(const typeT item, orderedArrayT* array) {
	ASSERT(array->lessThan);
	u32int iterator = 0;
	while (iterator < array->size && array->lessThan(array->array[iterator], item)) {
		++iterator;
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
		array->size++;
	}
}

typeT lookupOrderedArray(const u32int i, const orderedArrayT* array) {
	ASSERT(i < array->size);
	return array->array[i];
}

void removeOrderedRrray(u32int i, orderedArrayT* array) {
	while (i < array->size) {
		array->array[i] = array->array[i+1];
		++i;
	}
	array->size--;
}
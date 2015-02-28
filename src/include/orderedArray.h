// orderedArray.h -- Interface for creating, inserting and deleting
//                   from ordered arrays.

#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include "common.h"

/*
	This array is insertion sorted - it always remains in a sorted state (between calls).
	It can store anything that can be cast to a void* -- so a u32int, or any pointer.
*/
typedef void* typeT;

/*
	A predicate should return nonzero if the first argument is less than the second. Else 
	it should return zero.
*/
typedef s8int (*lessThanPredicateT)(typeT,typeT);

typedef struct {
    typeT* array;
    u32int size;
    u32int maxSize;
    lessThanPredicateT lessThan;
} orderedArrayT;

/*
	A standard less than predicate.
*/
s8int standardLessThanPredicate(typeT, typeT);

/*
	Create an ordered array.
*/
orderedArrayT createOrderedArray(u32int, lessThanPredicateT);
orderedArrayT placeOrderedArray(void*, u32int, lessThanPredicateT);

/*
	Destroy an ordered array.
*/
void destroyOrderedArray(orderedArrayT*);

/*
	Add an item into the array.
*/
void insertOrderedArray(typeT, orderedArrayT*);

/*
	Lookup the item at index i.
*/
typeT lookupOrderedArray(u32int, orderedArrayT*);

/*
	Deletes the item at location i from the array.
*/
void removeOrderedArray(u32int, orderedArrayT*);

#endif // ORDERED_ARRAY_H
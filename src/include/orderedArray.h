// orderedArray.h -- Interface for creating, inserting and deleting
//                    from ordered arrays.

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
typedef s8int (*lessthanPredicateT)(const typeT, const typeT);
typedef struct {
	typeT* array;
	u32int size;
	u32int maxSize;
	lessthanPredicateT lessThan;
} orderedArrayT;

/*
	A standard less than predicate.
*/
s8int standardLessthanPredicate(const typeT a, const typeT b);

/*
	Create an ordered array.
*/
orderedArrayT createOrderedArray(const u32int, const lessthanPredicateT);
orderedArrayT placeOrderedArray(const void*, const u32int, const lessthanPredicateT);

/*
	Destroy an ordered array.
*/
void destroyOrderedArray(orderedArrayT* array);

/*
	Add an item into the array.
*/
void insertOrderedArray(const typeT item, orderedArrayT* array);

/*
   Lookup the item at index i.
*/
typeT lookupOrderedArray(const u32int i, const orderedArrayT* array);

/*
	Deletes the item at location i from the array.
*/
void removeOrderedArray(u32int i, orderedArrayT* array);

#endif // ORDERED_ARRAY_H
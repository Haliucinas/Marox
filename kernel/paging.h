#ifndef MAROX_PAGING_H
#define MAROX_PAGING_H

#include "marox.h"

void pagingInit(void);

uintptr_t physToVirt(uintptr_t phys);
uintptr_t virtToPhys(uintptr_t virt);

#endif /* MAROX_PAGING_H */

#ifndef MAROX_IDT_H
#define MAROX_IDT_H

#include "int.h"

void idtInit();
void idtSetIntGate(uint8_t num, uintptr_t base, unsigned dpl);
void installIntHandler(int interrupt, int_handler_t handler);

#endif /* MAROX_IDT_H */

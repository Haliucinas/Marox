#ifndef MAROX_GDT_H
#define MAROX_GDT_H

#include <stdint.h>

void setKernelStack(uint32_t sp);
void gdtInit();

#endif /* MAROX_GDT_H */

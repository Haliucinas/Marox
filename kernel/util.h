#ifndef MAROX_UTIL_H
#define MAROX_UTIL_H

#include "marox.h"

uint32_t getESP(void);
uint32_t getEFlags(void);
uint32_t getRing(void);
void kreboot(void);
void khalt(void);

#endif /* MAROX_UTIL_H */

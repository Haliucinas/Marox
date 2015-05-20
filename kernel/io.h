#ifndef MAROX_IO_H
#define MAROX_IO_H

#include <stdint.h>

uint8_t inPortB(uint16_t port);
void outPortB(uint16_t port, uint8_t data);
uint16_t inPortWrite(uint16_t port);
void outPortWrite(uint16_t port, uint16_t data);
uint32_t inPortRead(uint16_t port);
uint32_t outPortRead(uint16_t port, uint32_t data);
void delayIO(void);

#endif /* MAROX_IO_H */

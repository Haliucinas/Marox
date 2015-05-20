#ifndef MAROX_IRQ_H
#define MAROX_IRQ_H

#include "int.h"

enum {
    IRQ_TIMER = 0,
    IRQ_KEYBOARD = 1,
    IRQ_RTC = 8,
    IRQ_MOUSE = 12
};

void irqInit();
void initIrqHandler(unsigned int irq, int_handler_t handler);
void enableIrq(unsigned int irq);
void disableIrq(unsigned int irq);
bool irqEnabled(unsigned int irq);

#endif /* MAROX_IRQ_H */

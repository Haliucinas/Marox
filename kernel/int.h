#ifndef MAROX_INT_H
#define MAROX_INT_H

#include <stdbool.h>

enum { EFLAGS_INTERRUPT_FLAG = 1 << 9 };

struct regs;
typedef void (*int_handler_t)(struct regs *r);

bool interruptsEnabled(void);

bool begIntAtomic(void);
void endIntAtomic(bool);

void cli(void);
void sti(void);

#endif /* MAROX_INT_H */

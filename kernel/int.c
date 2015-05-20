#include "assert.h"
#include "int.h"

extern uint32_t getEFlags(void);

bool interruptsEnabled(void) {
    uint32_t eflags = getEFlags();
    return (eflags & EFLAGS_INTERRUPT_FLAG) != 0;
}

void cli() {
    KASSERT(interruptsEnabled());
    __asm__ volatile ("cli");
}

void sti() {
    KASSERT(!interruptsEnabled());
    __asm__ volatile("sti");
}

bool begIntAtomic(void) {
    bool enabled = interruptsEnabled();
    if (enabled) {
        cli();
    }
    return enabled;
}

void endIntAtomic(bool iFlag) {
    KASSERT(!interruptsEnabled());
    if (iFlag) {
        sti();
    }
}

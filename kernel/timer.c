#include "irq.h"
#include "io.h"
#include "thread.h"
#include "timer.h"

void setTimerFrequency(unsigned int hz) {
    /* cmd = channel 0, LSB then MSB, Square Wave Mode, 16-bit counter */
    uint8_t cmd = 0x36;
    unsigned int divisor = PIT_FREQ_HZ / hz;
    outPortB(PIT_CMD_REG, cmd);            /* Set command byte */
    outPortB(PIT_DATA_REG0, divisor & 0xFF); /* Set low byte of divisor */
    outPortB(PIT_DATA_REG0, divisor >> 8);   /* Set high byte of divisor */
}

/* global count of system ticks (uptime) */
static uint32_t g_numTicks = 0;

int g_need_reschedule = false;


/* getter for global system tick count */
uint32_t getTicks(void) {
    return g_numTicks;
}

/* Handles timer interrupt.
 * By default, the timer fires at 18.222hz
 */
void timerHandler(struct regs *r) {
    (void)r; // prevent 'unused' parameter warning
    ++g_numTicks;

    if (getCurrentThread() && getCurrentThread()->id == 5) {
        DEBUGF("%s\n", "timerHandler in user!");
    }

    /* if the current thread has outlived the quantum, add it to the
     * run queue and schedule a new thread */
    thread_t* current = getCurrentThread();
    if (current) {
        if (++current->numTicks > THREAD_QUANTUM && preemptionEnabled()) {
            // DEBUGF("preempting thread %d\n", current->id);
            makeRunnable(current);
            g_need_reschedule = true;
        }
    }
}

/* installs timerHandler into IRQ0 */
void timerInit() {
    setTimerFrequency(TICKS_PER_SEC);
    initIrqHandler(IRQ_TIMER, timerHandler);
    enableIrq(IRQ_TIMER);
}

void delay(unsigned int ticks) {
    unsigned int eticks = g_numTicks + ticks;
    while (g_numTicks < eticks);
}

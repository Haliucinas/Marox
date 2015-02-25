// isr.c -- High level interrupt service routines and interrupt request handlers.

#include "isr.h"
#include "../lib/printf.h"

void isrHandler(registers regs) {
    printf("Recieved interrupt: no. %d code %x\n", regs.intNo, regs.errCode);
}
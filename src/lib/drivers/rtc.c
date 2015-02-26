#include "rtc.h"
#include "../printf.h"
#include "../../include/isr.h"
#include "../../include/common.h"

static u8int get(u8int reg) {
	outb(0x70, reg);
	return inb(0x71);
}

static void set(u8int reg, u8int val) {
	outb(0x70, reg);
	outb(0x71, val);
}

static u8int bcd2dec(u8int bcd) {
	return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void clockCallback() {
	static int ticks = 0;
	u8int chr = get(0xc);
	if ((++ticks) % 1024 == 0) {
		printf("Clock: 20%d-%d-%d %d:%d:%d\n",
			bcd2dec(get(0x09)), 
			bcd2dec(get(0x08)),
			bcd2dec(get(0x07)),
			bcd2dec(get(0x04)), 
			bcd2dec(get(0x02)),
			bcd2dec(get(0x00)));
		ticks = 0;
	}
}

void initClock() {
	registerInterruptHandler(IRQ8, &clockCallback);
	// This should NOT be interrupted
	__asm__ __volatile__("cli");
	u8int status = get(0x8B);
	set(0x8B, status | 0x40);
	__asm__ __volatile__("sti");
}
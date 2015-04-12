#include "rtc.h"
#include "../printf.h"
#include "../../include/isr.h"
#include "../../include/common.h"

clockT clock;

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

static void loadTime(clockT* clock) {
	clock->seconds = bcd2dec(get(0x00));
	clock->minutes = bcd2dec(get(0x02));
	clock->hours = bcd2dec(get(0x04));
	clock->weekday = bcd2dec(get(0x06));
	clock->dayOfMonth = bcd2dec(get(0x07));
	clock->month = bcd2dec(get(0x08));
	clock->year = bcd2dec(get(0x09));
	clock->century = bcd2dec(get(0x32));
}

static u8int intNum(u8int num) {
	u8int counter = 0;
	do {
		++counter;
		num /= 10;
	} while (num);
	return counter;
}

void clockCallback() {
	static int ticks = 0;
	if ((++ticks) % 1024 == 0) {
		loadTime(&clock);
		consoleColorBG(DARK_GRAY);
		u32int size = 5+intNum(clock.century)+intNum(clock.year)+intNum(clock.month)+intNum(clock.dayOfMonth)+intNum(clock.hours)+intNum(clock.minutes)+intNum(clock.seconds);

		/*printfAt(80-size, 24, "%d%d-%d-%d %d:%d:%d",
			clock.century,
			clock.year,
			clock.month,
			clock.dayOfMonth,
			clock.hours,
			clock.minutes,
			clock.seconds);*/
		sprintf(timeBuf, "%d%d-%02d-%02d %02d:%02d:%02d",
			clock.century,
			clock.year,
			clock.month,
			clock.dayOfMonth,
			clock.hours,
			clock.minutes,
			clock.seconds);
		printfAt(timeBuf, 61, 24);
		consoleColorBG(BLACK);
		ticks = 0;
	}
	get(0xc);
}

void initClock() {
	registerInterruptHandler(IRQ8, &clockCallback);
	// This should NOT be interrupted
	__asm__ __volatile__("cli");
	u8int status = get(0x8B);
	set(0x8B, status | 0x40);
	__asm__ __volatile__("sti");
}
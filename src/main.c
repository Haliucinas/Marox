// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/descTables.h"
#include "lib/printf.h"
#include "lib/drivers/pit.h"
#include "lib/drivers/keyboard.h"
#include "lib/drivers/rtc.h"
#include "include/paging.h"
#include "lib/kheap.h"
#include "include/multiboot.h"

#define OS_NAME "Marox OS"

void sayHello() {
	const char* welcome = (char*)"Welcome to ";
	const char* end = (char*)"!\n";

	consoleColorBG(DARK_GRAY);
	printfAt(welcome, 0, 0);
	consoleColorText(LIGHT_GRAY);
	printfAt(OS_NAME, 11, 0);
	consoleColorText(WHITE);
	printfAt(end, 19, 0);
	consoleColorBG(BLACK);
}

int kmain(struct multiboot* mboot) {

	// All our initialisation calls will go in here.
	initDescTables();
	initPaging();

	consoleClear();
	sayHello();

	__asm__ __volatile__("sti");
	initKeyboard(); // Init keyboard
	initClock(); // Init CMOS clock
	initTimer(50); // Init timer to 50Hz

	//u32int *ptr = (u32int*)0xA0000000;
	//u32int do_page_fault = *ptr;

	return 0xdeadbeef;
}
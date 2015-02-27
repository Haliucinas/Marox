// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/descTables.h"
#include "lib/printf.h"
#include "lib/drivers/pit.h"
#include "lib/drivers/keyboard.h"
#include "lib/drivers/rtc.h"
#include "include/paging.h"

#define OS_NAME "Marox OS"

void sayHello() {
	const char* welcome = (char*)"Welcome to ";
	const char* end = (char*)"!\n";

	consoleColorBG(DARK_GRAY);
	printfAt(0, 0, "%s", welcome);
	consoleColorText(LIGHT_GRAY);
	printfAt(11, 0, "%s", OS_NAME);
	consoleColorText(WHITE);
	printfAt(19, 0, "%s", end);
	consoleColorBG(BLACK);

}

struct multiboot; // to avoid compiler warning. We don't need it now
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

	return 0xdeadbeef;
}
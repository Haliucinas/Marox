// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/descTables.h"
#include "lib/printf.h"
#include "lib/drivers/pit.h"
#include "lib/drivers/keyboard.h"
#include "lib/drivers/rtc.h"

struct multiboot; // to avoid compiler warning. We don't need it now
int kmain(struct multiboot* mboot) {

	// All our initialisation calls will go in here.
	initDescTables();

	char* welcome = (char*)"Welcome to ";
	char* os = (char*)"Marox OS";
	char* end = (char*)"!\n";

	consoleClear();
	printf("%s", welcome);
	consoleColorText(LIGHT_GRAY);
	printf("%s", os);
	consoleColorText(WHITE);
	printf("%s", end);

	__asm__ __volatile__("sti");
	initKeyboard(); // Init keyboard
	initClock(); // Init CMOS clock
	initTimer(50); // Init timer to 50Hz

	return 0xdeadbeef;
}
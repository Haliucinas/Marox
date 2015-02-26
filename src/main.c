// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/descTables.h"
#include "lib/printf.h"
#include "lib/drivers/pit.h"

struct multiboot; // to avoid compiler warning. We don't need it now
int kmain(struct multiboot* mboot) {

	// All our initialisation calls will go in here.
	initDescTables();

	char* welcome = (char*)"Welcome to ";
	char* os = (char*)"Marox OS";
	char* end = (char*)"!\n";

	consoleClear();
	for (int i = 1; i < 15; ++i) {
		printf("%s", welcome);
		consoleColorText(i);
		printf("%s", os);
		consoleColorText(WHITE);
		printf("%s", end);
	}

	__asm__ __volatile__("sti");
	initTimer(50);

	return 0xdeadbeef;
}
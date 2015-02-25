// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/console.h"

struct multiboot; // to avoid compiler warning. We don't need it now
int kmain(struct multiboot* mboot) {
	// All our initialisation calls will go in here.
	char* welcome = (char*)"Welcome to ";
	char* os = (char*)"Marox OS";
	char* end = (char*)"!\n";

	consoleClear();
	for (int i = 1; i < 15; ++i) {
		consoleWrite(welcome);
		consoleColorText(i);
		consoleWrite(os);
		consoleColorText(WHITE);
		consoleWrite(end);
	}

	return 0xdeadbeef;
}
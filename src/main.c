// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/screen.h"

struct multiboot; // to avoid compiler warning. We don't need it now
int kmain(struct multiboot* mboot) {
	// All our initialisation calls will go in here.
	char* welcome = (char*)"Welcome to ";
	char* os = (char*)"Marox OS";
	char* end = (char*)"!\n";

	screenClear();
	for (int i = 1; i < 15; ++i) {
		screenWrite(welcome);
		screenColorText(i);
		screenWrite(os);
		screenColorText(WHITE);
		screenWrite(end);
	}

	return 0xdeadbeef;
}
// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

struct multiboot; // to avoid compiler warning. We don't need it now
int kmain(struct multiboot* mboot) {
	// All our initialisation calls will go in here.
	return 0xdeadbeef;
}
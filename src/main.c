// main.c -- Defines the C-code kernel entry point, calls initialisation routines.

#include "include/descTables.h"
#include "lib/printf.h"
#include "lib/drivers/pit.h"
#include "lib/drivers/keyboard.h"
#include "lib/drivers/rtc.h"
#include "include/paging.h"
#include "lib/kheap.h"
#include "include/multiboot.h"
#include "include/task.h"

#define OS_NAME "Marox OS"

u32int initEsp;
u32int maxHeapSize = 0x100000;


int i = 0;

void plus() {
	while (1) printf("Thread %d made i = %d\n", getPid(), ++i);
}

void minus() {
	while (1) printf("Thread %d made i = %d\n", getPid(), --i);
}

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

int kmain(struct multiboot* mboot, u32int initStack) {
	initEsp = initStack;
	consoleClear();
	sayHello();
	// All our initialisation calls will go in here.
	initDescTables();
	printf("[ %2kOK%15k ] Descriptor tables initialised.\n");
	initPaging();
	printf("[ %2kOK%15k ] Paging enabled.\n");
	initTasking();
	printf("[ %2kOK%15k ] Tasking enabled.\n");

	__asm__ __volatile__("sti");
	initKeyboard(); // Init keyboard
	printf("[ %2kOK%15k ] Keyboard driver started.\n");
	initClock(); // Init CMOS clock
	printf("[ %2kOK%15k ] Clock driver started.\n");
	initTimer(1); // Init timer to 50Hz
	printf("[ %2kOK%15k ] Timer started.\n");

	//u32int *ptr = (u32int*)0xA0000000;
	//u32int do_page_fault = *ptr;

	createTask(&plus);
	createTask(&minus);

	return 0xdeadbeef;
}
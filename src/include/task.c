// task.c - Implements the functionality needed to multitask.

#include "task.h"
#include "paging.h"
#include "common.h"
#include "../lib/kheap.h"

// The currently running task.
volatile taskT* currentTask;

// The start of the task linked list.
volatile taskT* readeQueue;

// Some externs are needed to access members in paging.c...
extern pageDirT* kernelDir;
extern pageDirT* currentDir;
extern void allocFrame(pageT*,int,int);
extern u32int initEsp;
extern u32int readEip();

// The next available process ID.
u32int nextPid = 1;
u32int newDirAddr;

void initTasking() {
	// Rather important stuff happening, no interrupts please!
	__asm__ __volatile__("cli");

	// Relocate the stack so we know where it is.
	moveStack((void*)STACK_TOP, 0x2000);

	// Initialise the first task (kernel task)
	currentTask = readeQueue = (taskT*)kNew(sizeof(taskT));
	currentTask->id = nextPid++;
	currentTask->esp = 0;
	currentTask->pageDir = currentDir;
	currentTask->next = 0;

	// Reenable interrupts.
	__asm__ __volatile__("sti");
}

void moveStack(void* newStackStart, u32int size) {
  // Allocate some space for the new stack.
  for( u32int i = (u32int)newStackStart;
	   i >= ((u32int)newStackStart-size);
	   i -= 0x1000) {
	// General-purpose stack is in user-mode.
	allocFrame( getPage(i, 1, currentDir), 0 /* User mode */, 1 /* Is writable */ );
  }
  
  // Flush the TLB by reading and writing the page directory address again.
  u32int pdAddr;
  __asm__ __volatile__("mov %%cr3, %0" : "=r" (pdAddr));
  __asm__ __volatile__("mov %0, %%cr3" : : "r" (pdAddr));

  // Old ESP and EBP, read from registers.
  u32int oldStackPointer; __asm__ __volatile__("mov %%esp, %0" : "=r" (oldStackPointer));
  u32int oldBasePointer;  __asm__ __volatile__("mov %%ebp, %0" : "=r" (oldBasePointer));

  // Offset to add to old stack addresses to get a new stack address.
  u32int offset = (u32int)newStackStart - initEsp;

  // New ESP and EBP.
  u32int newStackPointer = oldStackPointer + offset;
  u32int newBasePointer  = oldBasePointer  + offset;

  // Copy the stack.
  memcpy((void*)newStackPointer, (void*)oldStackPointer, initEsp-oldStackPointer);

  // Backtrace through the original stack, copying new values into
  // the new stack.  
  for(u32int i = (u32int)newStackStart; i > (u32int)newStackStart-size; i -= 4) {
	u32int tmp = *(u32int*)i;
	// If the value of tmp is inside the range of the old stack, assume it is a base pointer
	// and remap it. This will unfortunately remap ANY value in this range, whether they are
	// base pointers or not.
	if (( oldStackPointer < tmp) && (tmp < initEsp)) {
	  tmp = tmp + offset;
	  u32int *tmp2 = (u32int*)i;
	  *tmp2 = tmp;
	}
  }

  // Change stacks.
  __asm__ __volatile__("mov %0, %%esp" : : "r" (newStackPointer));
  __asm__ __volatile__("mov %0, %%ebp" : : "r" (newBasePointer));
}

u32int switchTask(u32int oldEsp) {
/*	// If we haven't initialised tasking yet, just return.
	if (!timerLockFree) {
		newDirAddr = currentTask->pageDir->physicalAddr;
	  return oldEsp;
	}*/
	
	if (!currentTask) {
		return oldEsp;
	}
	currentTask->esp = oldEsp;

	//Now switch what task we're on
	currentTask = currentTask->next;
	// If we fell off the end of the linked list start again at the beginning.
	if (!currentTask) {
		currentTask = readeQueue;
	}
	
	currentDir = currentTask->pageDir;
	newDirAddr = currentTask->pageDir->physicalAddr;

	return currentTask->esp; //Return new stack pointer to ASM
}

int createTask(void (*thread)()) {
	
	__asm__ __volatile__("cli");

	// Clone the address space.
	pageDirT* directory = cloneDir(currentDir);

	u32int currentEsp;
	u32int currentEbp;
	__asm__ __volatile__ ("mov %%esp, %0" : "=r"(currentEsp));
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r"(currentEbp));

	__asm__ __volatile__("mov %%cr3, %%ebx; \
	mov %0, %%cr3; \
	mov %%ebp, %%esp; \
	push $0x0202; \
	push $0x08; \
	push %2; \
	push $0; \
	push $32; \
	push $0; \
	push $0; \
	push $0; \
	push $0; \
	push $0; \
	push $0; \
	push $0; \
	push $0; \
	push $0x10; \
	mov %%ebx, %%cr3; \
	mov %1, %%esp; \
	" : : "r"(directory->physicalAddr), "r"(currentEsp), "r"((u32int)thread) : "ebx");
	
	int stackDepth = 14 * 4;
	
	// Create a new process.
	taskT* newTask = (taskT*)kNew(sizeof(taskT));

	newTask->esp = currentEbp - stackDepth;
	newTask->id = nextPid++;
	newTask->pageDir = directory;
	newTask->next = 0;
	
	taskT* tempTask = (taskT*)readeQueue;
	while (tempTask->next)
		tempTask = tempTask->next;
	tempTask->next = newTask;

	__asm__ __volatile__("sti");

	return newTask->id;
}

int getPid() {
	return currentTask->id;
}

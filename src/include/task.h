// task.h - Defines the structures and prototypes needed to multitask.

#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "paging.h"

// This structure defines a 'task' - a process.
typedef struct task {
    int id;	// Process ID.
    u32int esp;	// Stack and base pointers.
    pageDirT* pageDir; // Page directory.
    struct task* next; // The next task in a linked list.
} taskT;

// Initialises the tasking system.
void initTasking();

// Called by the timer irq asm code. Asm passes current esp, expects new esp in eax.
u32int switchTask(u32int);

// Forks the current process, spawning a new one with a different
// memory space.
//int fork();

// creates task at function thread. Returns pid
int createTask(void (*thread)());

// Causes the current process' stack to be forcibly moved to a new location.
void moveStack(void*, u32int);

// Returns the pid of the current process.
int getPid();

#endif

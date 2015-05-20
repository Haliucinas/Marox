#include "syscall.h"

#include "print.h"
#include "idt.h"
#include "x86.h"
#include "mem.h"
#include "thread.h"
#include "kb.h"

static void print(const char *msg) {
    kprintf("%s", msg);
}

DEFN_SYSCALL1(print, 0, const char*)
DEFN_SYSCALL1(sleep, 1, unsigned int)
DEFN_SYSCALL1(malloc, 2, size_t)
DEFN_SYSCALL1(free, 3, void*)
DEFN_SYSCALL1(exit, 4, int)
DEFN_SYSCALL0(waitForKey, 5)
DEFN_SYSCALL1(getLine, 6, char*)
DEFN_SYSCALL0(shmGet, 7)
DEFN_SYSCALL1(shmRelease, 8, int)
DEFN_SYSCALL2(shmWrite, 9, int, char*)
DEFN_SYSCALL2(shmRead, 10, int, char*)

static void *syscalls[] = {
    &print,
    &sleep,
    &malloc,
    &free,
    &exit,
    &waitForKey,
    &getLine,
    &shmGet,
    &shmRelease,
    &shmWrite,
    &shmRead
};
size_t num_syscalls = sizeof(syscalls) / sizeof(*syscalls);

#include "thread.h"
static void syscallHandler(struct regs *regs) {
    if (regs->eax >= num_syscalls) {
        kprintf("%s\n", "Invalid syscall number");
        return;
    }

    void *location = syscalls[regs->eax];

    int ret;
    __asm__ volatile (" \
      push %1; \
      push %2; \
      push %3; \
      push %4; \
      push %5; \
      call *%6; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
    " : "=a" (ret) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (location));
    regs->eax = ret;
}

void syscallsInit(void) {
    installIntHandler(128, syscallHandler);
}

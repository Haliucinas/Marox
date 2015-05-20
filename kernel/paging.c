#include "mem.h"
#include "x86.h"
#include "paging.h"
#include "idt.h"

uintptr_t physToVirt(uintptr_t phys) {
    return phys + KERNEL_VBASE;
}

uintptr_t virtToPhys(uintptr_t virt) {
    return virt - KERNEL_VBASE;
}

void pageFaultHandler(struct regs *regs) {
    uintptr_t faultAddr;
    __asm__ volatile("mov %%cr2, %0" : "=r" (faultAddr));

    int present = !(regs->err_code & 0x1);
    int rw = regs->err_code & 0x2;
    int us = regs->err_code & 0x4;
    int reserved = regs->err_code & 0x8;
    /* int id = regs->err_code & 0x10; */

    kprintf("Page fault! ( ");
    if (present) { kprintf("present "); }
    if (rw) { kprintf("read-only "); }
    if (us) { kprintf("user-mode "); }
    if (reserved) { kprintf("reserved "); }
    kprintf(") at 0x%X\n", faultAddr);
    khalt();
}

void pagingInit(void) {
    /* find first 4KB aligned address after the end of the kernel */
    /* create page directory for 4GB of RAM at this address */
    uintptr_t pageDirectory = (uintptr_t)allocPage();
    KASSERT(pageDirectory);
    DEBUGF("page directory: 0x%x\n", virtToPhys(pageDirectory));

    unsigned int pde = 0;
    /* set bits for all page directory entries */
    for (pde = 0; pde < 1024; pde++) {
        /* ((uintptr_t*)pageDirectory)[pde] = 0 | 2; /1* supervisor level, read/write, not present *1/ */
        ((uintptr_t*)pageDirectory)[pde] = 0 | 6;  /* usermode level, read/write, not present */
    }

    /* map first 16MB in 4 page tables */
    uintptr_t address = 0x0;
    unsigned int pidx = 0;
    for (pidx = KERNEL_VBASE >> 22; pidx < (KERNEL_VBASE >> 22) + 4; pidx++) {
        uintptr_t pageTable = (uintptr_t)allocPage();
        KASSERT(pageTable);

        unsigned int pte;
        for (pte = 0; pte < 1024; pte++) {
            /* ((uint32_t*)pageTable)[pte] = address | 3;  /1* supervisor level, read/write, present *1/ */
            ((uint32_t*)pageTable)[pte] = address | 7; /* usermode level, read/write, present */
            address += 0x1000;  /* next page address */
        }

        /* set PHYSICAL addresses of page table(s) in page directory */
        /* ((uintptr_t*)pageDirectory)[pidx] = virtToPhys(pageTable) | 3; /1* supervisor level, read/write, present *1/ */
        ((uintptr_t*)pageDirectory)[pidx] = virtToPhys(pageTable) | 7; /* usermode level, read/write, present */
        DEBUGF("page table %u: 0x%x\n", pidx, virtToPhys(pageTable));
    }

    installIntHandler(14, pageFaultHandler);

    /* move PHYSICAL page directory address into cr3 */
    __asm__ volatile("mov %0, %%cr3":: "r" (virtToPhys(pageDirectory)));

    /* clear 4MB page bit since we're switching to 4KB pages */
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0": "=r" (cr4));
    cr4 &= ~(0x00000010);
    __asm__ volatile("mov %0, %%cr4":: "r" (cr4));

    /* read cr0, set paging bit, write it back */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=r" (cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0":: "r" (cr0));

    /*
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0": "=r" (cr3));

    kprintf("Kernel end: 0x%x\n", end);
    kprintf("page directory phyical: 0x%x\n", cr3);
    kprintf("page directory entry 0: 0x%x\n", ((uint32_t*)cr3)[0]);
    uint32_t fpt = ((uint32_t*)cr3)[0] & 0xFFFFF000;
    kprintf("first page table physical: 0x%x\n", fpt);
    for (i = 0; i < 4; ++i) {
        kprintf("first page table entry %d: 0x%x\n", i, ((uint32_t*)fpt)[i]);
    }
    for (i = 1023; i > 1023-4; i--) {
        kprintf("first page table entry %d: 0x%x\n", i, ((uint32_t*)fpt)[i]);
    }
    */
}

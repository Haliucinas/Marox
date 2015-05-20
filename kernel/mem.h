#ifndef MAROX_MEM_H
#define MAROX_MEM_H

#include "marox.h"
#include "multiboot.h"

enum {
    HDWARE_RAM_START = 0x9F000,     /* 0x9FC00 page aligned down */
    HDWARE_RAM_END = 0x100000       /* end of video memory */
};

enum {
    KERNEL_HEAP_SIZE = 0x200000    /* 1M heap */
};

enum {
    PAGE_POWER = 12,
    PAGE_SIZE = (unsigned)(1 << PAGE_POWER),
    PAGE_MASK = (~(PAGE_SIZE - 1))
};

enum {
    PAGE_AVAIL  = 0x1,  /* page on freelist */
    PAGE_KERN   = 0x2,  /* page used by kernel */
    PAGE_HDWARE = 0x4,  /* page used by hardware (ISA hole) */
    PAGE_ALLOC  = 0x8,  /* page allocated */
    PAGE_UNUSED = 0x10, /* page unused */
    PAGE_HEAP   = 0x20  /* page in kernel heap */
};

struct page {
    uint32_t flags;
    struct page *next;
};
typedef struct page page_t;

struct shm {
    uintptr_t buffer;
    unsigned int owner;
};
typedef struct shm shm_t;

uintptr_t memInit(struct multiboot_info *mbInfo, uintptr_t kernstart, uintptr_t kernend);
void bssInit(void);

void shmInit(void);
int shmGet();
int shmRelease(int);
int shmWrite(int, char*);
int shmRead(int, char*);

uintptr_t pageAlignUp(uintptr_t addr);
uintptr_t pageAlignDown(uintptr_t addr);

void* allocPage(void);
void freePage(void* pageAddress);

void* malloc(size_t size);
void free(void *buffer);

void dumpmem(uintptr_t start, size_t bytes);

#endif /* MAROX_MEM_H */

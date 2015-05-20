#include "int.h"
#include "bget.h"
#include "string.h"
#include "mem.h"
#include "thread.h"

static page_t* g_pageArray = NULL;

static page_t* g_freePageHead = NULL;
static page_t* g_freePageTail = NULL;
static unsigned int g_freePageCount;
static shm_t sharedMemory[100];
static mutex_t shmLock;

/*
 * Determine if given address is a multiple of the page size.
 */
static inline bool isPageAligned(uintptr_t addr) {
    return addr == (addr & PAGE_MASK);
}

uintptr_t pageAlignUp(uintptr_t addr) {
    /* only align forward a page if not already aligned */
    /* so for 4K pages, least significant 3 bytes are not zero */
    if ((addr & (~PAGE_MASK)) != 0) {
        addr &= PAGE_MASK;
        addr += PAGE_SIZE;
    }
    return addr;
}

uintptr_t pageAlignDown(uintptr_t addr) {
    return addr & PAGE_MASK;
}

static unsigned int pageIndex(uintptr_t addr) {
    return (addr - KERNEL_VBASE) >> PAGE_POWER;
}

static page_t* pageFromAddress(uintptr_t addr) {
    return &g_pageArray[pageIndex(addr)];
}

static uintptr_t addressFromPage(page_t *page) {
    unsigned int index = page - g_pageArray;
    return (index << PAGE_POWER) + KERNEL_VBASE;
}

static page_t* getFreelistPage() {
    KASSERT(g_freePageCount > 0);
    KASSERT(g_freePageHead != NULL);

    page_t* page = g_freePageHead;

    page->flags = PAGE_ALLOC;

    // if we just emptied the freelist, NULL the head/tail
    if (g_freePageHead == g_freePageTail) {
        g_freePageHead = NULL;
        g_freePageTail = NULL;
    }

    // move freelist head forward a page
    g_freePageHead = page->next;
    g_freePageCount--;

    return page;
}

static void freelistPageAdd(page_t* page) {
    page->flags = PAGE_AVAIL;

    if (!g_freePageHead) {
        g_freePageHead = page;
    }

    if (g_freePageTail != NULL) {
        g_freePageTail->next = page;
    }

    g_freePageTail = page;

    g_freePageCount++;
}

static void markPageRange(uintptr_t start, uintptr_t end, uint32_t flags) {
    char *flagname;
    switch (flags) {
        case PAGE_AVAIL:
            flagname = "AVAIL";
            break;
        case PAGE_KERN:
            flagname = "KERN";
            break;
        case PAGE_HDWARE:
            flagname = "HDWARE";
            break;
        case PAGE_ALLOC:
            flagname = "ALLOC";
            break;
        case PAGE_UNUSED:
            flagname = "USED";
            break;
        case PAGE_HEAP:
            flagname = "HEAP";
            break;
        default:
            flagname = "BAD FLAG";
    }
    DEBUGF("Add range: 0x%x - 0x%x - %s\n", start, end, flagname);
    KASSERT(isPageAligned(start));
    KASSERT(isPageAligned(end));
    KASSERT(start < end);

    uintptr_t addr;
    for (addr = start; addr < end; addr += PAGE_SIZE) {
        page_t* page = pageFromAddress(addr);
        page->flags = flags;

        if (flags & PAGE_AVAIL) {
            freelistPageAdd(page);
        } else {
            page->next = NULL;
        }
    }
}


uintptr_t memInit(struct multiboot_info *mbInfo, uintptr_t kernstart, uintptr_t kernend) {
    /* require valid memory limits in multiboot info */
    KASSERT(mbInfo->flags & multiboot_info_MEMORY);
    uint32_t mem_upper = mbInfo->mem_upper * 1024;   /* mem_upper is in KB */
    DEBUGF("Mem low: 0x%x, Mem high: 0x%x\n", mbInfo->mem_lower * 1024, mem_upper);

    uint32_t numPages = mem_upper / PAGE_SIZE;
    DEBUGF("Number of pages: %u\n", numPages);

    /* align kernel_start down a page because technically the multiboot
     * header sits in front of the kernel's entry point */
    kernstart = pageAlignDown(kernstart);

    /* account for the size of the struct page array */
    uint32_t pageArrayB = numPages * sizeof(page_t);
    g_pageArray = (page_t*)(kernend);     /* make room for page list */

    /* move kernel end past the page_t array */
    kernend = pageAlignUp(kernend + pageArrayB);

    uintptr_t memStart = KERNEL_VBASE;
    uintptr_t firstPage = PAGE_SIZE + KERNEL_VBASE;
    uintptr_t hdwareStart = HDWARE_RAM_START + KERNEL_VBASE;
    uintptr_t heapStart = kernend;
    uintptr_t heapEnd = kernend + KERNEL_HEAP_SIZE;
    uintptr_t endOfMemory = numPages * PAGE_SIZE + KERNEL_VBASE;

    /* unused first page */
    markPageRange(memStart, firstPage, PAGE_UNUSED);
    /* extra RAM */
    markPageRange(firstPage, hdwareStart, PAGE_KERN);
    /* Extended BIOS and Video RAM */
    markPageRange(hdwareStart, kernstart, PAGE_HDWARE);
    markPageRange(kernstart, kernend, PAGE_KERN);         /* kernel pages */
    markPageRange(kernend, heapEnd, PAGE_HEAP);          /* heap pages */
    markPageRange(heapEnd, endOfMemory, PAGE_AVAIL);   /* available RAM */

/*
    if (mbInfo->flags & multiboot_info_MEM_MAP) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)mbInfo->mmap_addr;
        KASSERT(mbInfo->mmap_length > 0);   // assume memory map has > 1 entry

        uintptr_t range_start = 0, range_end = PAGE_SIZE;
        while ((uintptr_t)mmap < mbInfo->mmap_addr + mbInfo->mmap_length) {
            kprintf("Base Addr: 0x%x, Length: 0x%x, %s\n", (uint32_t)mmap->addr, (uint32_t)mmap->len,
                    (mmap->type == 1 ? "free" : "used"));

            uint32_t type = mmap->type;
            if (type == MULTIBOOT_MEMORY_AVAILABLE) {
                range_start = pageAlignUp(mmap->addr);
                range_end = pageAlignDown(mmap->addr + mmap->len);
                markPageRange(range_start, range_end, PAGE_AVAIL);
            } else {
                range_start = pageAlignDown(mmap->addr);
                range_end = pageAlignUp(mmap->addr + mmap->len);
                markPageRange(range_start, range_end, PAGE_HDWARE);
            }

            mmap = (multiboot_memory_map_t*) ((uintptr_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }
*/
    /* initialize the kernel's heap */
    DEBUGF("Creating kernel heap: start=0x%x, size=0x%x\n", heapStart, KERNEL_HEAP_SIZE);
    bpool((void*) heapStart, KERNEL_HEAP_SIZE);

    shmInit();

    return heapEnd;
}

void* allocPage(void) {
    void* addr = NULL;

    bool iFlag = begIntAtomic();

    if (g_freePageCount > 0) {
        page_t* page = getFreelistPage();
        KASSERT(page->flags & PAGE_ALLOC);
        addr = (void*)addressFromPage(page);
    }

    endIntAtomic(iFlag);

    return addr;
}

void freePage(void* pageAddress) {
    uintptr_t addr = (uintptr_t)pageAddress;
    KASSERT(isPageAligned(addr));

    bool iFlag = begIntAtomic();

    page_t* page = pageFromAddress(addr);
    KASSERT(page->flags & PAGE_ALLOC);
    freelistPageAdd(page);

    endIntAtomic(iFlag);
}

void bssInit(void) {
    extern char g_bss, g_end;

    uintptr_t bssStart = (uintptr_t)&g_bss;
    uintptr_t bssEnd = (uintptr_t)&g_end;

    /* zero BSS section */
    DEBUGF("BSS: 0x%x: %u\n", bssStart, bssEnd - bssStart);
    memset((void*)bssStart, 0, bssEnd - bssStart);
}

void shmInit(void) {
    int i;
    for (i = 0; i < 100; ++i) {
        sharedMemory[i].owner = 0;
        sharedMemory[i].buffer = bgetz(0x1000);
    }
}

int shmGet() {
    unsigned int i = 0;
    while (1) {
        if (sharedMemory[i].owner == 0) {
            sharedMemory[i].owner = getCurrentThread()->id;
            return i;
        }

    	mutexLock(&shmLock);
        i++;
    	mutex_unlock(&shmLock);
    }
}

int shmRelease(int id) {
    if (sharedMemory[id].owner == getCurrentThread()->id) {
        sharedMemory[id].owner = 0;
        memset(sharedMemory[id].buffer, 0, 0x1000);
    } else {
        return -1;
    }

    return 0;
}

int shmWrite(int desc, char* buffer) {
    if (sharedMemory[desc].owner == getCurrentThread()->id) {
        strcpy(sharedMemory[desc].buffer, buffer);
    } else {
        return -1;
    }

    return 0;
}

int shmRead(int desc, char* buffer) {
    if (desc >= 0 && desc < 10) {
        strcpy(buffer, sharedMemory[desc].buffer);
    } else {
        return -1;
    }
    
    return 0;
}

void* malloc(size_t size) {
    void *buffer = NULL;
    bool iFlag;

    KASSERT(size > 0);

    iFlag = begIntAtomic();
    buffer = bget(size);
    endIntAtomic(iFlag);

    return buffer;
}

void free(void *buffer) {
    bool iFlag;

    iFlag = begIntAtomic();
    brel(buffer);
    endIntAtomic(iFlag);
}

void dumpmem(uintptr_t start, size_t bytes) {
    if (bytes > 256) bytes = 256;
    DEBUGF("Dump mem: 0x%x (%u bytes)\n", start, bytes);
    char *ptr = (char*)start;
    unsigned int i;
    for (i = 0; i < bytes / 16; i++) {
        DEBUGF("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
                *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF,
                *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF,
                *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF,
                *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF, *ptr++ & 0xFF);
    }
}

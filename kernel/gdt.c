#include "string.h"
#include "x86.h"

enum {
    GDT_NULL_DESCR,
    GDT_CODE_DESCR,
    GDT_DATA_DESCR,
    GDT_USER_CODE_DESCR,
    GDT_USER_DATA_DESCR,
    GDT_TSS_DESCR,
    GDT_NUM_ENTRIES
};

/*
 * Segment Descriptor
 *
 * Notes on Direction/Conforming (DC) bit:
 * Code segment:
 *   1: code in this segment can be executed in lower privilege levels
 *   0: code in this segment can only be accessed by Ring # in descriptor
 * Data segment:
 *   1: segment grows down
 *   0: segment grows up
 */
struct segmentDescriptor {
    uint16_t limitLow;

    uint16_t baseLow;
    uint8_t baseMiddle;

    /* access byte - starting with type nibble */
    unsigned accessed:1;    /* toggled by CPU */
    unsigned rw: 1;         /* code: readable? data: writable? tss: busy? */
    unsigned dc: 1;         /* direction/conforming */
    unsigned exec: 1;       /* executable segment? */

    unsigned system: 1;     /* 1 = code/data segment, 0 = system */

    unsigned dpl: 2;        /* descriptor privilege level (Ring 0-3) */
    unsigned present: 1;    /* present? 1=Yes */

    unsigned limitHigh: 4;

    /* flags nibble */
    unsigned avail: 1;      /* populated? 0=no, 1=yes */
    unsigned reserved: 1;   /* always 0 */
    unsigned opsize: 1;     /* 0=16-bit, 1=32-bit */
    unsigned granularity: 1;/* 0=1 byte, 1=4KB */

    uint8_t baseHigh;
};

/* special GDT pointer */
struct gdt_ptr {
    uint16_t limit;     /* max bytes used by GDT */
    uint32_t base;      /* base address of GDT */
} __attribute__((packed));


struct tss {
    uint32_t link;

    /* stack pointers/selectors */
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;

    /* page directory reg */
    uint32_t cr3;

    /* general purpose regs */
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    /* segment selectors */
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;

    /* GDT selector for the LDT descriptor */
    uint32_t ldt;

    uint16_t trap;

    /* pointer to IO port permissions map for this task */
    uint16_t iomap;
};


/* GDT and special global GDT pointer */
static struct segmentDescriptor g_gdt[GDT_NUM_ENTRIES];
struct gdt_ptr g_gdt_ptr;
static struct tss g_tss;


static uint16_t gdtSelector(struct segmentDescriptor* sd) {
    /*
     * (# of segment descriptors from GDT start)
     *                    *
     *   (sizeof segment descriptor in bytes)
     */
    return (sd - &g_gdt[0]) * sizeof(struct segmentDescriptor);
}

static uint8_t segmentDescriptorType(struct segmentDescriptor* sd) {
    uint8_t type = *((uint8_t*)sd + 5);
    return type;
}

static uint8_t segmentDescriptorAccess(struct segmentDescriptor* sd) {
    uint8_t access = *((uint8_t*)sd + 6);
    return access;
}

static void initCsDescriptor(struct segmentDescriptor *descr, uintptr_t base, uintptr_t limit, unsigned dpl) {
    descr->baseLow = (base & 0xFFFF);
    descr->baseMiddle = (base >> 16) & 0xFF;
    descr->baseHigh = (base >> 24) & 0xFF;

    descr->limitLow = (limit & 0xFFFF);
    descr->limitHigh = (limit >> 16) & 0xF;

    descr->accessed = 0;
    descr->rw = 1;          /* readable */
    descr->dc = 0;          /* non-conforming (only executable in Ring 0) */
    descr->exec = 1;        /* executable */

    descr->system = 1;      /* code seg, not TSS */

    descr->dpl = dpl;
    descr->present =  1;

    descr->avail = 0;       /* no longer available */
    descr->reserved = 0;
    descr->opsize = 1;      /* 32-bit operands */
    descr->granularity = 1; /* 4KB */
}

static void initDsDescriptor(struct segmentDescriptor *descr, uintptr_t base, uintptr_t limit, unsigned dpl) {
    descr->baseLow = (base & 0xFFFF);
    descr->baseMiddle = (base >> 16) & 0xFF;
    descr->baseHigh = (base >> 24) & 0xFF;

    descr->limitLow = (limit & 0xFFFF);
    descr->limitHigh = (limit >> 16) & 0xF;

    descr->accessed = 0;
    descr->rw = 1;          /* writable */
    descr->dc = 0;          /* segment grows UP */
    descr->exec = 0;        /* NOT executable */

    descr->system = 1;      /* data seg, not TSS */

    descr->dpl = dpl;
    descr->present =  1;

    descr->avail = 0;       /* no longer available */
    descr->reserved = 0;
    descr->opsize = 1;      /* 32-bit operands */
    descr->granularity = 1; /* 4KB granularity */
}

// 0x9A CF = 01011000 10101111  |    dpl=0
// 0xFA CF = 11111000 10101111
// 0101101

static void initTssDescriptor(struct segmentDescriptor* descr, struct tss* tss) {
    uintptr_t base = (uintptr_t)tss;
    uint32_t limit = sizeof(*tss);

    descr->baseLow = (base & 0xFFFF);
    descr->baseMiddle = (base >> 16) & 0xFF;
    descr->baseHigh = (base >> 24) & 0xFF;

    descr->limitLow = (limit & 0xFFFF);
    descr->limitHigh = (limit >> 16) & 0xF;

    /* the type nibble is 0x9 (32-bit available TSS) */
    descr->accessed = 1;
    descr->rw = 0;          /* busy bit - NOT BUSY! on init */
    descr->dc = 0;
    descr->exec = 1;

    descr->system = 0;      /* tss seg, not code/data */

    /* descr->dpl = KERNEL_DPL;    /1* Ring 0 *1/ */
    descr->dpl = USERMODE_DPL;      /* Ring 3 */
    descr->present = 1;

    descr->avail = 0;        /* no longer available */
    descr->reserved = 0;
    descr->opsize = 0;      /* 0 for TSS (Intel docs) */
    descr->granularity = 0; /* require TSS to be > 0x67 bytes in size */
}

static void initTss(struct tss* tss) {
    memset(tss, 0, sizeof(*tss));
    tss->ss0 = DATA_SEG_SELECTOR;
    tss->esp0 = 0;
    tss->cs = CODE_SEG_SELECTOR | USERMODE_DPL;
    tss->ss = DATA_SEG_SELECTOR | USERMODE_DPL;
    tss->ds = tss->es = tss->fs = tss->gs = DATA_SEG_SELECTOR | USERMODE_DPL;
    tss->iomap = sizeof(*tss);
    KASSERT(tss->iomap == 104);
}

void setKernelStack(uintptr_t sp) {
    /* DEBUGF("Updating ESP0 in TSS to: %X\n", sp); */
    g_tss.esp0 = sp;
}

/* defined in 'start.__asm__' */
extern void gdtFlush(void*);
extern void tssFlush();

void gdtInit() {
    KASSERT(sizeof(struct segmentDescriptor) == 8);

    /* Set up GDT pointer and limit */
    g_gdt_ptr.limit = sizeof(struct segmentDescriptor) * GDT_NUM_ENTRIES;
    g_gdt_ptr.base = (uint32_t)&g_gdt;

    /* NULL descriptor */
    struct segmentDescriptor* nullDescr = &g_gdt[GDT_NULL_DESCR];
    KASSERT(gdtSelector(nullDescr) == NULL_SEG_SELECTOR);
    memset(nullDescr, 0, sizeof(*nullDescr));

    /* code segment, Base addr: 0, Limit: 2^20 - 1 4KB pages */
    struct segmentDescriptor* csDescr = &g_gdt[GDT_CODE_DESCR];
    KASSERT(gdtSelector(csDescr) == CODE_SEG_SELECTOR);
    initCsDescriptor(csDescr, 0, 0xFFFFF, KERNEL_DPL);

    /* data segment, Base addr: 0, Limit: 2^20 - 1 4KB pages */
    struct segmentDescriptor* dsDescr = &g_gdt[GDT_DATA_DESCR];
    KASSERT(gdtSelector(dsDescr) == DATA_SEG_SELECTOR);
    initDsDescriptor(dsDescr, 0, 0xFFFFF, KERNEL_DPL);

    /* user mode code segment, Base addr: 0, Limit: 2^20 - 1 4KB pages */
    struct segmentDescriptor* userCsDescr = &g_gdt[GDT_USER_CODE_DESCR];
    KASSERT(gdtSelector(userCsDescr) == USER_CODE_SEG_SELECTOR);
    initCsDescriptor(userCsDescr, 0, 0xFFFFF, USERMODE_DPL);

    /* user mode data segment, Base addr: 0, Limit: 2^20 - 1 4KB pages */
    struct segmentDescriptor* userDsDescr = &g_gdt[GDT_USER_DATA_DESCR];
    KASSERT(gdtSelector(userDsDescr) == USER_DATA_SEG_SELECTOR);
    initDsDescriptor(userDsDescr, 0, 0xFFFFF, USERMODE_DPL);

    struct segmentDescriptor* tssDescr = &g_gdt[GDT_TSS_DESCR];
    initTssDescriptor(tssDescr, &g_tss);
    initTss(&g_tss);

    uint16_t tssSel = gdtSelector(tssDescr);
    KASSERT(tssSel == TSS_SELECTOR);

    KASSERT(segmentDescriptorType(&g_gdt[GDT_CODE_DESCR]) == 0x9A);
    KASSERT(segmentDescriptorAccess(&g_gdt[GDT_CODE_DESCR]) == 0xCF);
    KASSERT(segmentDescriptorType(&g_gdt[GDT_DATA_DESCR]) == 0x92);
    KASSERT(segmentDescriptorAccess(&g_gdt[GDT_DATA_DESCR]) == 0xCF);
    KASSERT(segmentDescriptorType(&g_gdt[GDT_USER_CODE_DESCR]) == 0xFA);
    KASSERT(segmentDescriptorAccess(&g_gdt[GDT_USER_CODE_DESCR]) == 0xCF);
    KASSERT(segmentDescriptorType(&g_gdt[GDT_USER_DATA_DESCR]) == 0xF2);
    KASSERT(segmentDescriptorAccess(&g_gdt[GDT_USER_DATA_DESCR]) == 0xCF);
    /* KASSERT(segmentDescriptorType(tssDescr) == 0x89); */
    KASSERT(segmentDescriptorType(tssDescr) == 0xE9);
    KASSERT(segmentDescriptorAccess(tssDescr) == 0x00);

    /* Flush out the old GDT and install new */
    gdtFlush(&g_gdt_ptr);

    /* load the TSS selector */
    /* tssFlush(); */
    __asm__ volatile("ltr %0" : : "a" (tssSel));
}

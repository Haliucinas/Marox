#include "util.h"
#include "screen.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "mem.h"
#include "paging.h"
#include "syscall.h"
#include "thread.h"
#include "kb.h"
#include "rtc.h"
#include "timer.h"

extern uintptr_t g_start, g_code, g_data, g_bss, g_end;

static void testShmSend(uint32_t);
static void testShmRead(uint32_t);
static void printDate(uint32_t);
static void echoInput(uint32_t);
static void hogCPU(uint32_t);
static void testUsermode(uint32_t);
static void echoRegisterValues(uint32_t);
static void isPrime(uint32_t);
static void godThread(uint32_t);

struct modInfo {
    uintptr_t start;
    uintptr_t end;
};

uintptr_t loadMods(struct multiboot_info *mbInfo, struct modInfo *initRdInfo) {
    uintptr_t start = 0, end = 0;
    if (mbInfo->modCount > 0) {
        multibootModule *mod = (multibootModule*)physToVirt(mbInfo->modAddr);
        initRdInfo->start = physToVirt(mod->modStart);
        initRdInfo->end = physToVirt(mod->modEnd);

        for (unsigned int i = 0; i < mbInfo->modCount; ++i) {
            start = physToVirt(mod->modStart);
            end = physToVirt(mod->modEnd);
            DEBUGF("Mod start: 0x%x, Mod end: 0x%x, Cmd line: %s\n", start, end, (char*)physToVirt(mod->cmdLine));
            ++mod;
        }
    }
    return end;
}

void kMain(struct multiboot_info *mbInfo, multiboot_uint32_t mbootMagic) {
    /* interrupts are disabled */

    kClearScreen();
    kprintf("Welcome to MaroxOS...\n\n");

    if (mbootMagic == MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("Multiboot Successful\n");
    } else {
        kprintf("Bad multiboot\n");
        return;
    }

    bssInit();     /* zero all static data */
    gdtInit();
    kprintf("GDT installed\n");
    idtInit();
    kprintf("IDT and ISRs installed\n");
    irqInit();
    kprintf("IRQ handlers installed\n");

    struct modInfo initRdInfo;
    uintptr_t modSend = loadMods(mbInfo, &initRdInfo);
    uintptr_t kEnd = (uintptr_t)(&g_end), kStart = (uintptr_t)(&g_start);
    kEnd = (modSend > kEnd) ? modSend : kEnd;
    kEnd = memInit(mbInfo, kStart, kEnd);
    kprintf("Memory manager and Heap initialized\n");

    sti();

    timerInit();
    keyboardInit();
    rtcInit();
    syscallsInit();

    //khalt();

    schedulerInit();
    kprintf("Scheduler initialized\n");

    pagingInit();
    kprintf("Paging enabled\n");

    DEBUGF("ESP: %X\n", getESP());

    // Run GRUB module (which just returns the value of register ESP
    unsigned int len = initRdInfo.end - initRdInfo.start;
    dumpmem(initRdInfo.start, len);
    typedef void (*call_module_t)(uint32_t);
    void *cp = malloc(len);
    memcpy(cp, (const void*)initRdInfo.start, len);
    call_module_t mod0 = (call_module_t)cp;

    // Test interrupt handler
    /* __asm__ volatile ("int $0x03");

    //Test ISR handler with a page fault
    /*uintptr_t* page_fault = (uintptr_t*)0xFFFFF0000;
    kprintf("page fault? 0x%x\n", *page_fault);
    kprintf("page fault? %u\n", *page_fault); */

    // test threads - loop infinitely without yielding CPU */
    // thread_t *infinite0 = spawnThread(hogCPU, 0, PRIORITY_NORMAL, false, false);
    // thread_t *infinite1 = spawnThread(hogCPU, 0, PRIORITY_NORMAL, false, false);
    thread_t *infinite2 = spawnThread(echoRegisterValues, 0, PRIORITY_NORMAL, false, false);

    // start thread to print date/time on screen
    thread_t* datePrinter = spawnThread(printDate, 0, PRIORITY_NORMAL, false, false);
    thread_t* tst = spawnThread(mod0, 0, PRIORITY_NORMAL, false, false);
    // thread_t* test = spawnThread(testUsermode, 5, PRIORITY_NORMAL, false, true);

    thread_t* shm_send = spawnThread(testShmSend, 0, PRIORITY_NORMAL, false, false);
    thread_t* shm_recv = spawnThread(testShmRead, 0, PRIORITY_NORMAL, false, false);

    // wait for some thread to finish (forever)
    join(datePrinter);

    // kprintf("%u\n", 1 / 0);

    // delay(500);
    // kreboot();

    // kClearScreen();
    kprintf("Goodbye!");
}

static void echoRegisterValues(uint32_t arg) {
    (void)arg;
    static volatile uint32_t eaxValue, ebxValue, ecxValue, edxValue;

    while (true) {
        __asm__ ("mov %%EAX, %0": "=r" (eaxValue));
        __asm__ ("mov %%EBX, %0": "=r" (ebxValue));
        __asm__ ("mov %%ECX, %0": "=r" (ecxValue));
        __asm__ ("mov %%EDX, %0": "=r" (edxValue));

        DEBUGF("[EAX=%x; EBX=%x; ECX=%x; EDX=%x]\n", eaxValue, ebxValue, ecxValue, edxValue);

        sleep(2000);
    }
}

static void printDate(uint32_t arg) {
    (void)arg;
    unsigned int row, col;
    struct tm dt;

    while (true) {
        dateTime(&dt);
        kGetChar(&row, &col);
        kSetCursor(arg, 58);
        kprintf("%02u:%02u:%02u %s %02u, %04u", dt.hour, dt.min, dt.sec, monthName(dt.month), dt.mday, dt.year);
        kSetCursor(row, col);
        sleep(200);
    }
}

void echoInput(uint32_t arg) {
    (void)arg;

    for (unsigned int i = 0; i < arg; ++i) {
        keycode_t kc = waitForKey();

        if (kc == 'q') {
            break;
        }

        kPutChar(kc);
    }

    kprintf("\nYou've typed enough!\n");
}

static void hogCPU(uint32_t arg) {
    (void)arg;

    while (true) ;
}

static void testUsermode(uint32_t arg) {
    uprintf("testUsermode looping %u times...\n", arg);
    
    char *dst = (char*)syscall_malloc(42);
    
    syscall_getLine(dst);
    syscall_print(dst);

    syscall_free(dst);
    syscall_sleep(2000);

    char buff[128];

    syscall_shmRead(0, buff);

    syscall_print(buff);
    for (int i = 0; i < arg; ++i) {
        char *dst = (char*)syscall_malloc(42);
        strcpy(dst, "hello");

        syscall_free(dst);
        syscall_print("Hello from usermode!\n");
        syscall_sleep(1000);
    }
}

static void testShmSend(uint32_t arg) {
    char* num = (char*)malloc(11);
    int shmDesc = shmGet();
    int len;

    while (true) {
        kprintf("Enter range (1-x): ");
        len = getLine(num);
        kprintf("\n");
        num[len] = '\0';

        shmWrite(shmDesc, num);

        kprintf("Thread sent %s to shared mem.\n", num);
        sleep(atoi(num)*500);
    }

    shmRelease(shmDesc);
    free(num);
}

static void testShmRead(uint32_t arg) {
    char buff[11];
    int num;

    while (true) {
        shmRead(arg, buff);
        num = atoi(buff);
        kprintf("Recv thread got: %d\nStarting prime check:\n", num);
        
        thread_t* list[num];

        for (int i = 1; i <= num; ++i) {
            list[i] = spawnThread(isPrime, i, PRIORITY_NORMAL, false, false);
            sleep(500);
        }
    }
}

static void isPrime(uint32_t arg) {
    for (int i = 2; i < arg; ++i) {
        if (arg % i == 0 && i != arg) {
            kprintf("[Thread %d] %d is not a prime.\n", getCurrentThread()->id, arg);
            return;
        }
    }

    kprintf("[Thread %d] %d is a prime.\n", getCurrentThread()->id, arg);
    exit(0);
}
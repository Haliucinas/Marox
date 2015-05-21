#include "irq.h"
#include "io.h"
#include "thread.h"
#include "kb.h"
#include "print.h"

static thread_queue_t keycodeWaitQueue;

enum { KEYCODE_QUEUE_MASK = 255, KEYCODE_QUEUE_SIZE = 256 };
//#define KEYCODE_QUEUE_NEXT(idx)     (((idx) + 1) & KEYCODE_QUEUE_MASK)
static keycode_t keycodeQueue[KEYCODE_QUEUE_SIZE];
static uint8_t keycodeQueueHead, keycodeQueueTail;

/* US Keyboard Layout lookup table */
uint8_t kdbus[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',     /* 9 */
    '9', '0', '-', '=', '\b',   /* Backspace */
    '\t',                       /* Tab */
    'q', 'w', 'e', 'r', /* 19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',       /* Enter key */
    0,                  /* 29   - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',   /* 39 */
    '\'', '`',   0,             /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',                 /* 49 */
    'm', ',', '.', '/',   0,                            /* Right shift */
    '*',
    0,  /* Alt */
    ' ', /* Space bar */
    1,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
    '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
    '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

static void enqueueKeycode(keycode_t kc) {
    /* only enqueue key code if the queue is not full */
    if ((keycodeQueueTail + 1) != keycodeQueueHead) {
        keycodeQueue[keycodeQueueTail++] = kc;
    }
}

static keycode_t dequeueKeycode(void) {
    /* assert that the queue is not empty */
    KASSERT(keycodeQueueHead != keycodeQueueTail);
    return keycodeQueue[keycodeQueueHead++];
}

static void toggleLight(unsigned int lightCode) {
    while (inPortB(KBD_STATUS_REG) & KBD_STATUS_BUSY);

    outPortB(KBD_DATA_REG, 0xED);
    outPortB(KBD_DATA_REG, lightCode & 0xF);
}

/*
static void toggle_scroll_lock_light(void) {
    toggleLight(0x0);
}
static void toggle_num_lock_light(void) {
    toggleLight(0x1);
}
*/

static void toggleCapsLockLight(void) {
    toggleLight(0x2);
}


void keyboardHandler(struct regs *r) {
    (void)r;

    // read from keyboard data register
    uint8_t data = inPortB(KBD_DATA_REG);
    uint8_t scancode = data & 0x7F;

    if (data & 0x80) {
        // key released
        if (scancode == 1) {
            toggleCapsLockLight();
        }
    } else {
        /* key pressed, add it to keycode queue and wake up
         * all threads waiting on the keycode thread queue */
        uint16_t keycode = kdbus[scancode];
        enqueueKeycode(keycode);

        wakeAll(&keycodeWaitQueue);
    }
}

/* installs keyboardHandler into IRQ1 */
void keyboardInit() {
    initIrqHandler(IRQ_KEYBOARD, keyboardHandler);
    enableIrq(IRQ_KEYBOARD);
}

keycode_t waitForKey(void) {
    keycode_t kc;
    bool iFlag = begIntAtomic();
    bool empty = true;

    do {
        empty = (keycodeQueueHead == keycodeQueueTail);

        if (empty) {
            wait(&keycodeWaitQueue);
        } else {
            kc = dequeueKeycode();
        }
    } while (empty);

    endIntAtomic(iFlag);

    return kc;
}

static unsigned int atoi(char* str) {
    int res = 0;
 
    for (int i = 0; str[i] != '\0'; ++i) {
        res = res*10 + str[i] - '0';
    }
 
    return res;
}

int getLine(char* buff) {
    keycode_t kc;
    bool iFlag = begIntAtomic();
    bool empty = true;

    int i = 0;
    do {
        empty = (keycodeQueueHead == keycodeQueueTail);
        
        if (empty) {
            wait(&keycodeWaitQueue);
        } else {
            bool shouldPrintChar = false;

            kc = dequeueKeycode();

            if (kc != 0) {
                if ((kc == '\b') && (i > 0)) {
                    shouldPrintChar = true;

                    i--;
                } else {
                    buff[i] = kc;
                    shouldPrintChar = true;

                    i++;
                }

                if ((kc != '\n') && shouldPrintChar)
                    kprintf("%c", kc);
            }
        }
    } while (kc != '\n');

    endIntAtomic(iFlag);

    return i-1;
}
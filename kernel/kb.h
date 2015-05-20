#ifndef MAROX_KB_H
#define MAROX_KB_H

enum {
    KBD_STATUS_BUSY = 0x02,
    KBD_DATA_REG = 0x60,
    KBD_CMD_REG = 0x64,
    KBD_STATUS_REG = 0x64,
    KBD_CPU_RESET_PIN = 0xFE
};

void keyboardInit();

typedef uint16_t keycode_t;
keycode_t waitForKey(void);
int getLine(char*);

#endif /* MAROX_KB_H */

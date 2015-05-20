#include "irq.h"
#include "io.h"
#include "rtc.h"


static char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char* days[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static struct tm g_dateTime;
static bool CMOS_BCD_VALUES = 0;


uint8_t readCmos(uint8_t addr) {
    outPortB(CMOS_ADDR_REG, addr | NMI_DISABLE);
    return inPortB(CMOS_DATA_REG);
}

void writeCmos(uint8_t addr, uint8_t val) {
    outPortB(CMOS_ADDR_REG, addr | NMI_DISABLE);
    outPortB(CMOS_DATA_REG, val);
}

char* dayName(uint8_t wday) {
    if (wday > 6) {
        return NULL;
    }
    return days[wday];
}

char* monthName(uint8_t mon) {
    if (mon < 1 || mon > 12) {
        return "?";
    }
    return months[mon - 1];
}

void dateTime(struct tm *time) {
    time->sec = g_dateTime.sec;
    time->min = g_dateTime.min;
    time->hour = g_dateTime.hour;
    time->wday = g_dateTime.wday;
    time->mday = g_dateTime.mday;
    time->month = g_dateTime.month;
    time->year = g_dateTime.year;
    time->yday = 0;
    time->isDst = 0;
}

void rtcHandler(struct regs *r) {
    (void)r;    /* prevent 'unused parameter' warning */
    static unsigned int ticks = 0;

    /* only read CMOS registers once a second (RTC @ 1024Hz)
     * in testing using QEMU, the RTC was very finnicky regarding
     * CMOS reads immediately after the RTC periodic interrupt is fired.
     * There's no reason to read the CMOS values at 1024Hz anyway, but I like
     * using an IRQ handler to populate a static date-time struct regularly
     * rather than performing the CMOS reads on demand.
     */
    ticks++;
    if (ticks / 1024 == 1) {
        ticks = 0;

        uint8_t sec = readCmos(0x0);
        uint8_t min = readCmos(0x02);
        uint8_t hour = readCmos(0x04);
        uint8_t wday = readCmos(0x06);
        uint8_t mday = readCmos(0x07);
        uint8_t month = readCmos(0x08);
        uint16_t year = (uint8_t)readCmos(0x09);
        /* There is an "RTC century register" at offset 108 in ACPI's
        * "Fixed ACPI Description Table". If this byte is 0, then the RTC
        * does not have a century register, otherwise, it is the number
        * of the RTC register to use for the century...
        *
        * if it existed:
        * uint8_t century = readCmos(CMOS_CENTURY_REG);
        *
        * For now, I'm going to 'deduce' the century based on the year
        */

        /* convert values if they're packed BCD */
        if (CMOS_BCD_VALUES) {
            sec = BCD2BIN(sec);
            min = BCD2BIN(min);
            hour = BCD2BIN(hour);
            wday = BCD2BIN(wday);
            mday = BCD2BIN(mday);
            month = BCD2BIN(month);
            year = BCD2BIN(year);
        }

        /* convert hour from 12-hour to 24-hour if necessary */
        uint8_t regB = readCmos(CMOS_STATUS_REGB);
        if (!(regB & 0x02) && (hour & 0x80)) {
            hour = ((hour & 0x7F) + 12) % 24;
        }

        /* fix year depending on century */
        year += (CURRENT_YEAR / 100) * 100;
        if (year < CURRENT_YEAR) {
            year += 100;
        }

        g_dateTime.sec = sec;
        g_dateTime.min = min;
        g_dateTime.hour = hour;
        g_dateTime.wday = wday;
        g_dateTime.mday = mday;
        g_dateTime.month = month;
        g_dateTime.year = year;
        g_dateTime.yday = 0;
        g_dateTime.isDst = 0;
    }

    /* register C must be read for another interrupt to occur
     * select register C, read, throw it away */
    outPortB(CMOS_ADDR_REG, CMOS_STATUS_REGC);
    inPortB(CMOS_DATA_REG);
}

void rtcInit(void) {
    initIrqHandler(IRQ_RTC, rtcHandler);

    /* determine if values are packed BCD or Binary */
    uint8_t regB = readCmos(CMOS_STATUS_REGB);
    if (!(regB & 0x04)) {
        CMOS_BCD_VALUES = true;
    }

    /* enable IRQ8 (RTC) - interrupts must be disabled! */
    /* read CMOS register B */
    uint8_t prev = readCmos(CMOS_STATUS_REGB);
    /* write previous value with bit 6 turned on */
    writeCmos(CMOS_STATUS_REGB, prev | 0x40);

    enableIrq(IRQ_RTC);
}

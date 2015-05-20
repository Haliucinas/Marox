#ifndef MAROX_RTC_H
#define MAROX_RTC_H

#include "marox.h"

#define BCD2BIN(bcd)    (((bcd) & 0xF) + ((bcd) >> 4) * 10)

enum {
    CMOS_STATUS_REGA = 0x0A,
    CMOS_STATUS_REGB = 0x0B,
    CMOS_STATUS_REGC = 0x0C,
    NMI_ENABLE = 0x0F,
    CMOS_ADDR_REG = 0x70,
    CMOS_DATA_REG = 0x71,
    NMI_DISABLE = 0x80,
    CURRENT_YEAR = 2013
};

struct tm {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t mday;
    uint8_t month;
    uint8_t wday;   /* days since Monday */
    uint16_t yday;  /* days since January 1 */
    uint16_t year;  /* year since 0 AD */
    uint16_t isDst; /* is daylight savings flag */
};

char* dayName(uint8_t wday);
char* monthName(uint8_t mon);
void dateTime(struct tm *tm_out);
void rtcInit(void);


#endif /* MAROX_RTC_H */

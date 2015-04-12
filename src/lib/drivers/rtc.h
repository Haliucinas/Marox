#ifndef RTC_H
#define RTC_H

#include "../../include/common.h"

#define BUFF_SIZE 19

typedef struct {
	u8int seconds;
	u8int minutes;
	u8int hours;
	u8int weekday;
	u8int dayOfMonth;
	u8int month;
	u8int year;
	u8int century;
} clockT;

void initClock();
u8int timeBuf[BUFF_SIZE];

#endif //RTC_H
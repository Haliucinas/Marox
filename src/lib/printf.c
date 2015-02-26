#include "printf.h"

static u8int* writeHex(u8int* str, u32int n) {
	u32int digit;
	u32int nDigits = 2 * sizeof(u32int);

	*str++ = '0';
	*str++ = 'x';

	for (int i = nDigits - 1; i >= 0; --i) {
		digit = ((n >> 4 * i) & 0xF);
		*str++ = digit < 10 ? digit + 0x30 : digit + 0x57;
	}

	return str;
}

static u8int* writeDec(s8int *str, s32int n) {
	s8int buffer[12];
	memset(buffer, 0, sizeof(buffer));

	if (n < 0) {
		buffer[0] = '-';
		n = -n;
	}

	buffer[11] = '0';

	for (int i = 11; n; --i, n /= 10) {
		buffer[i] = (n % 10) + 0x30;
	}

	for (int i = 0; i < 12; ++i) {
		if (buffer[i]) *str++ = buffer[i];
	}

	return str;
}

static s8int* handleFormatChar(s8int* str, s8int fmt, va_list* args) {
	switch (fmt) {
		case '%': // in case we want to print %
			*str++ = '%';
		case 's': { // string handler
			const s8int* s = va_arg(*args, const s8int*);
			while (*s) {
				*str = *s;
				++str, ++s;
			}
			break;
		}
		case 'c': // char handler
			*str++ = (u8int)va_arg(*args, s32int);
			break;
		case 'x': // hex handler
			str = writeHex(str, va_arg(*args, u32int));
			break;
		case 'd': // decimal handler
			str = writeDec(str, va_arg(*args, s32int));
			break;
		default: // just print format
			*str++ = '%';
			*str++ = fmt;
	}
	return str;
}

int vsprintf(s8int* buf, const s8int* fmt, va_list args) {
	while (*fmt) {
		switch (*fmt) {
			case '%':
				buf = handleFormatChar(buf, *(++fmt), &args);
				++fmt;
				break;
			default:
				*buf = *fmt;
				++buf, ++fmt;
		}
	}
	*buf = 0;
	return 0;
}

int sprintf(s8int* str, const s8int* fmt, ...) {
	va_list args;
	s32int ret;

	va_start(args, fmt);
	ret = vsprintf(str, fmt, args);
	va_end(args);

	consoleWrite(str);
	return ret;
};

int printf(const s8int *fmt, ...) {
	s8int buffer[128];
	va_list args;
	s32int ret;

	va_start(args, fmt);
	ret = vsprintf(buffer, fmt, args);
	va_end(args);

	consoleWrite(buffer);
	return ret;
};

int printfAt(const u8int x, const u8int y, const s8int* fmt, ...) {
	s8int buffer[128];
	va_list args;
	s32int ret;

	va_start(args, fmt);
	ret = vsprintf(buffer, fmt, args);
	va_end(args);

	consoleWriteAt(buffer, x, y);
	return ret;
};
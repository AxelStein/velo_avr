/*
 * utils.c
 *
 * Created: 09.07.2020 22:24:26
 *  Author: alex
 */ 

/**
* C++ version 0.4 char* style "itoa":
* Written by Luk?s Chmela
* Released under GPLv3.
*/
#include "utils.h"


char* itoa(int value, char* result, int base) {
	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

long abs_val(long v) {
	return v < 0 ? -v : v;
}

char *ftoa(char *a, float f, int precision) {
	long p[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
	char *ret = a;
	long number = (long) f;
	itoa(number, a, 10);
	while (*a != '\0') a++;
	*a++ = '.';
	long decimal = abs_val((long)((f - number) * p[precision]));
	itoa(decimal, a, 10);
	return ret;
}

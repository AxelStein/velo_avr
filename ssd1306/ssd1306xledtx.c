/**
 * SSD1306xLED - Library for the SSD1306 based OLED/PLED 128x64 displays
 *
 * @author Neven Boyanov
 *
 * This is part of the Tinusaur/SSD1306xLED project.
 *
 * Copyright (c) 2018 Neven Boyanov, The Tinusaur Team. All Rights Reserved.
 * Distributed as open source software under MIT License, see LICENSE.txt file.
 * Retain in your source code the link http://tinusaur.org to the Tinusaur project.
 *
 * Source code available at: https://bitbucket.org/tinusaur/ssd1306xled
 *
 */

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ssd1306xled.h"
#include "ssd1306xledtx.h"

extern void ssd1306_start_data();	// Initiate transmission of data
extern void ssd1306_data_byte(uint8_t);	// Transmission 1 byte of data
extern void ssd1306_stop();			// Finish transmission
char* ftoa(float f, int precision, char* a);
char* itoa(int value, char* result, int base);

const uint8_t *ssd1306tx_font_src;
char str_buf[10];

void ssd1306tx_init(const uint8_t *font) {
	ssd1306tx_font_src = font;
}

void ssd1306tx_float(float n, int precision) {
	ssd1306tx_string(ftoa(n, precision, str_buf));
}

void ssd1306tx_int(int n) {
	ssd1306tx_string(itoa(n, str_buf, 10));
}

void ssd1306tx_char(char ch) {
	uint16_t j = (ch << 2) + (ch << 1) - 192; // Equiv.: j=(ch-32)*6 <== Convert ASCII code to font data index.
	ssd1306_start_data();
	for (uint8_t i = 0; i < 6; i++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306tx_font_src[j + i]));
	}
	ssd1306_stop();
}

void ssd1306tx_string(char* s) {
	while (*s) {
		ssd1306tx_char(*s++);
	}
}

/**
* C++ version 0.4 char* style "itoa":
* Written by Lukas Chmela
* Released under GPLv3.
*/
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

char* ftoa(float f, int precision, char* a) {
	long p[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
	char* ret = a;
	long number = (long) f;
	itoa(number, a, 10);
	while (*a != '\0') a++;
	*a++ = '.';
	long decimal = abs_val((long)((f - number) * p[precision]));
	itoa(decimal, a, 10);
	return ret;
}

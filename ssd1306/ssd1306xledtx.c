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
void ftoa(float f, int precision, char *buf);
void itoa(int n, char *buf);
int abs_val(int v);

const uint8_t *ssd1306tx_font_src;

void ssd1306tx_init(const uint8_t *font) {
	ssd1306tx_font_src = font;
}

void ssd1306tx_float(float f, int precision) {
	if (f < 0 || f >= 1000) {
		return;
	}
	
	char buf[10];
	ftoa(f, precision, buf);
	
	for (uint8_t i = 0; buf[i] != '\0'; i++) {
		ssd1306tx_char(buf[i]);
	}
}

void ssd1306tx_int(int n) {
	if (n < 0 || n >= 1000) {
		return;
	}
	
	char buf[5];
	itoa(n, buf);
	
	for (uint8_t i = 0; buf[i] != '\0'; i++) {
		ssd1306tx_char(buf[i]);
	}
}

void ssd1306tx_char(char ch) {
	uint16_t j = (ch << 2) + (ch << 1) - 192; // Equiv.: j=(ch-32)*6 <== Convert ASCII code to font data index.
	ssd1306_start_data();
	for (uint8_t i = 0; i < 6; i++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306tx_font_src[j + i]));
	}
	ssd1306_stop();
}

void ssd1306tx_string(char *s) {
	while (*s) {
		ssd1306tx_char(*s++);
	}
}

void itoa(int n, char *buf) {
	int i = 0, j = 0;
	do {
		buf[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);
	buf[i] = '\0';

	// reverse
	char tmp;
	for (j = i-1, i = 0; i < j; i++, j--) {
		tmp = buf[i];
		buf[i] = buf[j];
		buf[j] = tmp;
	}
}

int abs_val(int v) {
	return v < 0 ? -v : v;
}

void ftoa(float f, int precision, char *buf) {
	int number = (int) f;
	itoa(number, buf);
	while (*buf != '\0') buf++;
	*buf++ = '.';
	
	if (precision > 4) {
		precision = 4;
	}
	int p[] = {0, 10, 100, 1000, 10000};
	int decimal = abs_val((int)((f - number) * p[precision]));
	itoa(decimal, buf);
}
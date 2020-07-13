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
#include "font6x8.h"
#include "font8x16.h"

extern void ssd1306_start_data();	// Initiate transmission of data
extern void ssd1306_data_byte(uint8_t);	// Transmission 1 byte of data
extern void ssd1306_stop();			// Finish transmission

void ssd1306tx_char_6x8(char ch);
void ssd1306tx_char_8x16(char ch, uint8_t x, uint8_t y);
void ftoa(float f, int precision, char *buf);
void itoa(int n, char *buf);
int abs_val(int v);

uint8_t font_size = 1;

void ssd1306tx_font_size(uint8_t size) {
	font_size = size;
}

void ssd1306tx_string(char *s) {
	switch (font_size) {
		case 1:
			while (*s) ssd1306tx_char_6x8(*s++);
			break;
		/*
		case 2: {
			uint8_t x = pos_x;
			uint8_t y = pos_y;
			
			while (*s) {
				ssd1306tx_char_8x16(*s++, x, y);
				x += 8;
			}
			
			ssd1306_set_pos(x, y);
			break;
		}
		*/
	}
}

void ssd1306tx_char_6x8(char ch) {
	uint16_t row = (ch << 2) + (ch << 1) - 192;
	
	ssd1306_start_data();
	for (uint8_t col = 0; col < 6; col++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306xled_font6x8data[row + col]));
	}
	ssd1306_stop();
}

void ssd1306tx_char_8x16(char ch, uint8_t x, uint8_t y) {
	uint16_t row = (ch - 32) * 16;
	
	ssd1306_set_pos(x, y);
	ssd1306_start_data();
	for (uint8_t col = 0; col < 8; col++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306xled_font8x16data[row + col]));
	}
	ssd1306_stop();
	
	ssd1306_set_pos(x, y + 1);
	ssd1306_start_data();
	for (uint8_t col = 8; col < 16; col++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306xled_font8x16data[row + col]));
	}
	ssd1306_stop();
}

void ssd1306tx_float(float f, int precision) {
	if (f < 0 || f >= 1000) {
		return;
	}
	
	char buf[10];
	ftoa(f, precision, buf);
	ssd1306tx_string(buf);
}

void ssd1306tx_int(int n) {
	if (n < 0 || n >= 1000) {
		return;
	}
	
	char buf[5];
	itoa(n, buf);
	ssd1306tx_string(buf);
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
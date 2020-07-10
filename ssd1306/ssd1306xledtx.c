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

uint8_t* ssd1306tx_font_src;

void ssd1306tx_set_font(uint8_t* font) {
	ssd1306tx_font_src = font;
}

/* itoa:  конвертируем n в символы в s */
void itoa(int n, char s[]) {
	int i, j, sign;

	if ((sign = n) < 0)  /* записываем знак */
		n = -n;          /* делаем n положительным числом */
	i = 0;
	do {       /* генерируем цифры в обратном порядке */
		s[i++] = n % 10 + '0';   /* берем следующую цифру */
	} while ((n /= 10) > 0);     /* удаляем */

	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';

	char c;
	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

void ssd1306tx_num(int num) {
	bool negative = num < 0;
	num *= -1;

	char buf[15];
	uint8_t i = 0;
	do {
		buf[i++] = num % 10 + '0';
	} while ((num /= 10) > 0);
	if (negative) {
		buf[i++] = '-';
	}
	buf[i] = 's';
	printf("%s", buf);
	for (uint8_t start = 0, i = 14; i < 255; i++) {
		if (start) {
			printf("%c", buf[i]);
		} else if (buf[i] == 's') {
			start = 1;
		}
	}
	/*
	for (i = 11; i < 255; i--) {
		printf("%c", buf[i]);
	}
	*/
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

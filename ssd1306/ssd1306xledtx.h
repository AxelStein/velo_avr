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

#ifndef SSD1306XLEDTX_H
#define SSD1306XLEDTX_H

extern uint8_t *ssd1306xled_font6x8;
extern uint8_t *ssd1306xled_font8x16;
// ----------------------------------------------------------------------------

void ssd1306tx_init(const uint8_t *font);
void ssd1306tx_float(float n, int precision);
void ssd1306tx_int(int n);
void ssd1306tx_char(char ch);
void ssd1306tx_string(char *s);

// ============================================================================

#endif

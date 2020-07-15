/*
 * Velo.c
 *
 * Created: 08.07.2020 1:39:18
 * Author : alex
 */
#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "ssd1306/ssd1306xled.h"
#include "ssd1306/ssd1306xledtx.h"

#define WHEEL_PIN PB2
#define BTN_PIN PB3
#define LED_PIN PB4
#define LONG_PRESS_TIME 500
#define WHEEL_RPM_MAX 600
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_TIME 2
#define MENU_LED 3

uint8_t EEMEM EEPROM_WHEEL_DIAMETER;

volatile uint32_t ms;

bool display_turned;
uint8_t display_menu;
uint32_t display_timer;

bool led_turned;
uint32_t led_timer;

bool btn_pressed;
bool btn_long_pressed;
uint32_t btn_timer; // ms

float wheel_length; // km
float distance; // km
float speed; // km/h
float max_speed; // km/h
float avg_speed; // km/h
float speed_arr[8];
uint8_t speed_arr_index;

volatile bool wheel_rotation_started;
uint8_t wheel_rotation_counter;
uint32_t wheel_rotation_last_time; // ms
uint32_t wheel_rotation_start_time; // ms
uint16_t wheel_rpm;

void start_millis_timer();
uint32_t millis();
void calc_wheel_length();
void display_update();
void calc_wheel_length();
void set_wheel_diameter(uint8_t diameter);
void turn_display(bool on);
void switch_display_menu();
void display_update();
void calc_avg_speed(float speed);
void turn_led(bool on);
void handle_btn_click(uint8_t pin_state, uint32_t timer_now);
void calc_speed(uint32_t timer_now);

int main(void) {
	// set 8 MHz frequency
	CLKPR = 1 << CLKPCE;
	CLKPR = 0;
	
	DDRB = 0;
	DDRB |= _BV(LED_PIN); // led pin as output
	
	PORTB |= _BV(BTN_PIN); // turn on btn pin input pullup
	
	start_millis_timer();
	
	calc_wheel_length();
	display_update();
	
	_delay_ms(100);
	ssd1306_init();
	ssd1306_clear();
	display_update();
	
	display_turned = true;
	
	uint32_t timer_now;
	bool wheel_pin_state;
    
    while (1) {
		timer_now = millis();
		
		wheel_pin_state = PINB & _BV(WHEEL_PIN);
		// detect rotation start
		if (!wheel_pin_state && !wheel_rotation_started) {
			wheel_rotation_started = true;
		}
		// detect when magnet passes by the hall sensor
		if (wheel_pin_state && wheel_rotation_started) {
			wheel_rotation_started = false;
			
			wheel_rotation_last_time = timer_now;
			if (wheel_rotation_start_time == 0) {
				wheel_rotation_start_time = timer_now;
			}

			wheel_rotation_counter++;
			distance += wheel_length;
		}
		
		calc_speed(timer_now);
		
		handle_btn_click(PINB & _BV(BTN_PIN), timer_now);
		
		// toggle led every 250 ms
		if (led_turned && (timer_now - led_timer) >= 250) {
			PORTB ^= _BV(LED_PIN);
			led_timer = timer_now;
		}
		
		// idle
		if (speed != 0 && timer_now - wheel_rotation_last_time >= 4000) {
			speed = 0;
			wheel_rpm = 0;
			wheel_rotation_counter = 0;
			wheel_rotation_start_time = 0;
			if (display_menu == MENU_MAIN) {
				display_update();
			}
		}
		
		bool upd_time = display_menu == MENU_TIME && timer_now - display_timer >= 1000;
		bool upd_speed = display_menu == MENU_MAIN && speed > 0 && timer_now - display_timer >= 4000;
		if (upd_time || upd_speed) {
			display_update();
			display_timer = timer_now;
		}
    }
}

void start_millis_timer() {
	cli();
	
	TCCR0A |= _BV(WGM01);            // set timer0 CTC mode
	OCR0A = 130;                     // set timer0 compare value
	TCCR0B |= _BV(CS00) | _BV(CS01); // set timer0 prescaler 64
	TIMSK |= _BV(OCIE0A);            // enable interrupt
	
	sei();
}

ISR(TIMER0_COMPA_vect) {
	ms++;
}

uint32_t millis() {
	uint32_t copy;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		copy = ms;
	}
	
	return copy;
}

void calc_wheel_length() {
	uint8_t diameter = eeprom_read_byte(&EEPROM_WHEEL_DIAMETER); // cm
	if (diameter == 0xFF) {
		diameter = 64; // default
		set_wheel_diameter(diameter);
	}
	wheel_length = (diameter * 3.14) / 100000.0;
}

void set_wheel_diameter(uint8_t diameter) {
	if (diameter > 0 && diameter < 0xFF) {
		eeprom_write_byte(&EEPROM_WHEEL_DIAMETER, diameter);
	}
}

void turn_display(bool on) {
	display_turned = on;
	ssd1306_turn_display(on);
}

void switch_display_menu() {
	ssd1306_clear();
	display_menu++;
	if (display_menu == 4) {
		display_menu = 0;
	}
}

void display_update() {
	switch(display_menu) {
		case MENU_MAIN:
			ssd1306_set_pos(0, 0);
			ssd1306tx_string("s: ");
			ssd1306tx_float(speed, 1);
			ssd1306tx_string(" km/h   ");

			ssd1306_set_pos(0, 2);
			ssd1306tx_string("d: ");
			ssd1306tx_float(distance, 2);
			ssd1306tx_string(" km   ");
			
			ssd1306_set_pos(0, 4);
			ssd1306tx_string("rpm: ");
			ssd1306tx_int(wheel_rpm);
			ssd1306tx_string("   ");
			break;
		
		case MENU_SPEED:
			ssd1306_set_pos(0, 0);
			ssd1306tx_string("ms: ");
			ssd1306tx_float(max_speed, 1);
			ssd1306tx_string(" km/h ");

			ssd1306_set_pos(0, 2);
			ssd1306tx_string("as: ");
			ssd1306tx_float(avg_speed, 1);
			ssd1306tx_string(" km/h ");
			break;

		case MENU_TIME: {
			uint32_t now = millis();
			uint32_t sec = now / 1000ul;
			int hours = (sec / 3600ul);
			int minutes = (sec % 3600ul) / 60ul;
			int seconds = (sec % 3600ul) % 60ul;
			
			ssd1306_set_pos(0, 0);
			ssd1306tx_string("time:");
			
			ssd1306_set_pos(0, 2);
			ssd1306tx_int_p(hours, 2);
			ssd1306tx_string(":");
			
			ssd1306tx_int_p(minutes, 2);
			ssd1306tx_string(":");
			
			ssd1306tx_int_p(seconds, 2);
			break;
		}
		
		case MENU_LED:
			ssd1306_set_pos(0, 0);
			ssd1306tx_string("led:");

			ssd1306_set_pos(0, 2);
			ssd1306tx_string(led_turned ? "on " : "off");
			break;
	}
}

void turn_led(bool on) {
	led_turned = on;
	PORTB &= ~_BV(LED_PIN); // turn off led
}

void handle_btn_click(uint8_t pin_state, uint32_t timer_now) {
	// click start
	if (!btn_pressed && !pin_state) {
		btn_pressed = true;
		btn_timer = timer_now;
	}
	
	// handle single button click
	if (btn_pressed && pin_state) {
		btn_pressed = false;
		if (!btn_long_pressed && display_turned && timer_now - btn_timer >= 50) { // single press
			switch_display_menu();
			display_update();
		}
		btn_long_pressed = false;
	}
	
	// handle long button click
	if (btn_pressed && !btn_long_pressed && ((timer_now - btn_timer) >= LONG_PRESS_TIME)) {
		btn_long_pressed = true;
		switch (display_menu) {
			case MENU_LED:
				turn_led(!led_turned);
				display_update();
				break;
			default:
				turn_display(!display_turned);
				break;
		}
	}
}

void calc_speed(uint32_t timer_now) {
	if (wheel_rotation_counter == 5) {
		uint32_t interval = timer_now - wheel_rotation_start_time;
		uint16_t avg_interval = interval / 4;

		uint16_t rpm = 60000 / avg_interval;
		if (rpm < WHEEL_RPM_MAX) {
			wheel_rpm = rpm;
			speed = wheel_rpm * 60 * wheel_length;
			if (speed >= max_speed) {
				max_speed = speed;
			}
			calc_avg_speed(speed);
		}

		wheel_rotation_counter = 0;
		wheel_rotation_start_time = 0;
	}
}

void calc_avg_speed(float speed) {
	if (speed == 0) {
		return;
	}
	
	speed_arr[speed_arr_index++] = speed;
	
	if (speed_arr_index == 8) {
		speed_arr_index = 0;
		
		float sum = 0;
		for (uint8_t i = 0; i < 8; i++) {
			sum += speed_arr[i];
		}
		
		sum /= 8;
		if (avg_speed == 0) {
			avg_speed = sum;
		} else {
			avg_speed = (avg_speed + sum) / 2;
		}
	}
}
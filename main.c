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
#include "ssd1306/font6x8.h"

#define WHEEL_PIN PB2
#define BTN_PIN PB3
#define LED_PIN PB4
#define LONG_PRESS_TIME 500
#define WHEEL_ROTATION_MAX 5
#define WHEEL_RPM_MAX 600
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_RPM 2
#define MENU_POWER 3
#define MENU_LED 4

uint8_t EEMEM EEPROM_WHEEL_DIAMETER;
uint8_t EEMEM EEPROM_PWR_SAVE_MODE;
uint8_t EEMEM EEPROM_LED_AUTO;

volatile uint32_t ms;
bool display_turned;
bool pwr_save_mode;
uint8_t display_menu;
bool led_turned;
bool led_auto;

bool btn_pressed;
bool btn_long_pressed;
uint32_t btn_timer; // ms

float wheel_length; // km
float distance; // km

volatile bool wheel_rotated;
bool wheel_pin_enabled;
uint8_t wheel_rotation_counter;
uint32_t wheel_rotation_last_time; // ms
uint32_t wheel_rotation_start_time; // ms
uint16_t wheel_rpm;
float speed; // km/h
float max_speed; // km/h
float avg_speed; // km/h
float speed_arr[5];
uint8_t speed_arr_index;

void start_millis_timer();
uint32_t millis();
void attach_wheel_interrupt();
void calc_wheel_length();
void display_data();
void calc_wheel_length();
void set_wheel_diameter(uint8_t diameter);
void turn_display(bool on);
void switch_display_menu();
void display_data();
void enable_pwr_save_mode(bool enable);
void enable_sleep_mode();
void calc_avg_speed(float speed);
void turn_led(bool on);
void handle_btn_click(uint8_t pin_state, uint32_t timer_now);
void calc_speed(uint32_t timer_now);

int main(void) {
	CLKPR = 1 << CLKPCE;
	CLKPR = 0;
	
	DDRB = 0;
	// led pin as output
	DDRB |= _BV(LED_PIN);
	
	// turn on btn pin input pullup
	PORTB |= _BV(BTN_PIN);
	
	attach_wheel_interrupt();
	start_millis_timer();
	
	display_turned = true;

	pwr_save_mode = eeprom_read_byte(&EEPROM_PWR_SAVE_MODE);
	led_auto = eeprom_read_byte(&EEPROM_LED_AUTO);
	if (led_auto) {
		// todo
		// get current time
		// turn led
	}
	
	calc_wheel_length();
	display_data();
	
	unsigned long timer_now;
	
	_delay_ms(100);
	ssd1306_init();
	ssd1306tx_init(ssd1306xled_font6x8data);
	ssd1306_clear();
	// ssd1306_fill(0xFF);
	_delay_ms(1000);
    
    while (1) {
		timer_now = millis();
		
		if (wheel_rotated) {
			wheel_rotated = false;
			
			wheel_rotation_last_time = timer_now;
			if (wheel_rotation_start_time == 0) {
				wheel_rotation_start_time = timer_now;
			}

			wheel_rotation_counter++;
			distance += wheel_length;
		}
		
		handle_btn_click(PINB & _BV(BTN_PIN), timer_now);
		// handle_btn_click(digitalRead(BTN_PIN), timer_now);

		calc_speed(timer_now);
		
		// idle
		if (timer_now - wheel_rotation_last_time >= 3000) {
			if (speed != 0 || (0 < wheel_rotation_counter && wheel_rotation_counter < WHEEL_ROTATION_MAX)) {
				speed = 0;
				wheel_rpm = 0;
				wheel_rotation_counter = 0;
				wheel_rotation_start_time = 0;
				display_data();
			}
		}
		
		// sleep
		if (timer_now - wheel_rotation_last_time >= 20000) {
			wheel_rotation_last_time = 0;
			if (pwr_save_mode) {
				enable_sleep_mode();
			}
		}

		_delay_ms(1);
    }
}

void attach_wheel_interrupt() {
	cli();

	// the rising edge of INT0 generates an interrupt
	MCUCR |= _BV(ISC00) | _BV(ISC01);
	
	// enable external interrupt
	GIMSK |= _BV(INT0);

	sei();
}

ISR(INT0_vect) {
	wheel_rotated = true;
	// TODO remove
	PORTB ^= _BV(LED_PIN);
}

void start_millis_timer() {
	cli();
	
	// set timer0 CTC mode
	TCCR0A |= _BV(WGM01);
	
	// set timer0 compare value
	OCR0A = 125;
	
	// set timer0 prescaler 64
	TCCR0B |= _BV(CS00) | _BV(CS01);
	
	// enable interrupt
	TIMSK |= _BV(OCIE0A);
	
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
		diameter = 65; // default
		set_wheel_diameter(diameter);
	}
	wheel_length = (diameter * 3.14) / 100000.0;
}

void set_wheel_diameter(uint8_t diameter) {
	if (diameter > 0 && diameter < 255) {
		eeprom_write_byte(&EEPROM_WHEEL_DIAMETER, diameter);
	}
}

void turn_display(bool on) {
	display_turned = on;
	ssd1306_turn_display(on);
}

void switch_display_menu() {
	display_menu++;
	if (display_menu == 5) {
		display_menu = 0;
	}
}

// NOTE: Screen width - 128, that is 21 symbols per row.
void display_data() {
	ssd1306_clear();

	switch(display_menu) {
		case MENU_MAIN:
			ssd1306_setpos(0, 0);
			ssd1306tx_string("s: ");
			ssd1306tx_float(speed, 1);
			ssd1306tx_string(" km/h");

			ssd1306_setpos(0, 10);
			ssd1306tx_string("d: ");
			ssd1306tx_float(distance, 2);
			ssd1306tx_string(" km");
			break;
		
		case MENU_SPEED:
			ssd1306_setpos(0, 0);
			ssd1306tx_string("ms: ");
			ssd1306tx_float(max_speed, 1);
			ssd1306tx_string(" km/h");

			ssd1306_setpos(0, 10);
			ssd1306tx_string("as: ");
			ssd1306tx_float(avg_speed, 1);
			ssd1306tx_string(" km/h");
			break;

		case MENU_RPM:
			ssd1306_setpos(0, 0);
			ssd1306tx_int(wheel_rpm);
			ssd1306tx_string(" rpm");
			break;
		
		case MENU_POWER:
			ssd1306_setpos(0, 0);
			ssd1306tx_string("pwr save:");

			ssd1306_setpos(0, 10);
			ssd1306tx_string(pwr_save_mode ? "on" : "off");
			break;

		case MENU_LED:
			ssd1306_setpos(0, 0);
			ssd1306tx_string("led:");

			ssd1306_setpos(0, 10);
			ssd1306tx_string(led_turned ? "on" : "off");
			break;
	}
}

void enable_pwr_save_mode(bool enable) {
	pwr_save_mode = !pwr_save_mode;
	eeprom_write_byte(&EEPROM_PWR_SAVE_MODE, pwr_save_mode);
}

void enable_sleep_mode() {
	turn_display(false);
	
	// turn off led
	PORTB &= ~_BV(LED_PIN);
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enable();
	sleep_bod_disable();
	sei();
	sleep_cpu();

	sleep_disable();
	sei();
}

void turn_led(bool on) {
	led_turned = on;
	
	TCCR1 = 0;
	OCR1C = 0;
	TIMSK = 0;
	
	if (on) {
		cli();
		
		// set timer1 CTC mode
		TCCR1 |= _BV(CTC1);
		
		// set timer1 compare value
		OCR1C = 244;
		
		// set timer1 prescaler 8192
		TCCR1 |= _BV(CS11) | _BV(CS12) | _BV(CS13);
		
		// enable interrupt
		TIMSK |= _BV(OCIE1A);

		sei();
	}
}

// toggle led every 250 ms
ISR(TIMER1_COMPA_vect) {
	PORTB ^= _BV(LED_PIN);
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
		if (!btn_long_pressed && display_turned) { // single press
			switch_display_menu();
			display_data();
		}
		btn_long_pressed = false;
	}
	
	// handle long button click
	if (btn_pressed && !btn_long_pressed && ((timer_now - btn_timer) >= LONG_PRESS_TIME)) {
		btn_long_pressed = true;
		switch (display_menu) {
			case MENU_POWER:
				enable_pwr_save_mode(!pwr_save_mode);
				display_data();
				if (pwr_save_mode) {
					wheel_rotation_last_time = timer_now; // fixme
				}
				break;
			case MENU_LED:
				turn_led(!led_turned);
				display_data();
				break;
			default:
				turn_display(!display_turned);
				break;
		}
	}
}

void calc_speed(uint32_t timer_now) {
	if (wheel_rotation_counter == WHEEL_ROTATION_MAX) {
		uint32_t interval = timer_now - wheel_rotation_start_time;
		float avg_interval = interval / WHEEL_ROTATION_MAX;

		uint16_t rpm = 60000 / avg_interval;
		if (rpm < WHEEL_RPM_MAX) {
			wheel_rpm = rpm;
			speed = wheel_rpm * 60 * wheel_length;
			if (speed >= max_speed) {
				max_speed = speed;
			}

			calc_avg_speed(speed);
			display_data();
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
	
	if (speed_arr_index == 5) {
		speed_arr_index = 0;
		float sum = 0;
		for (uint8_t i = 0; i < 5; i++) {
			sum += speed_arr[i];
		}
		
		sum /= 5;
		if (avg_speed == 0) {
			avg_speed = sum;
			} else {
			avg_speed = (avg_speed + sum) / 2;
		}
	}
}
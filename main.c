/*
 * Velo.c
 *
 * Created: 08.07.2020 1:39:18
 * Author : alex
 */
#define F_CPU 1000000L

#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdbool.h>
#include "main.h"

#define WHEEL_PIN 2
#define BTN_PIN 4
#define TEMP_PIN A1
#define LED_PIN 5
#define LONG_PRESS_TIME 750
#define WHEEL_ROTATION_MAX 5
#define WHEEL_RPM_MAX 600
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_RPM 2
#define MENU_POWER 3
#define MENU_VOLTAGE 4
#define MENU_TEMP 5
#define MENU_LED 6

uint8_t EEMEM EEPROM_WHEEL_DIAMETER;
uint8_t EEMEM EEPROM_PWR_SAVE_MODE;
uint8_t EEMEM EEPROM_LED_AUTO;

bool display_turned;
bool pwr_save_mode;
uint8_t display_menu;
bool sleep_mode;
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

int main(void) {
	// turn on btn pin input pullup
	PORTB |= _BV(BTN_PIN);
	
	// led pin as output
	DDRB |= _BV(LED_PIN);
	
	attach_wheel_interrupt();
	
	display_turned = true;

	pwr_save_mode = eeprom_read_byte(&EEPROM_PWR_SAVE_MODE);
	led_auto = eeprom_read_byte(&EEPROM_LED_AUTO);
	if (led_auto) {
		// get current time
		// turn led
	}
	
	calc_wheel_length();
	display_data();
	
	unsigned long timer_now;
    
    while (1) {
		if (sleep_mode) {
			sleep_mode = false;
			turn_display(true);
		}

		// timer_now = millis();
		timer_now = 0;
		
		handle_btn_click(PINB & _BV(BTN_PIN), timer_now);
		// handle_btn_click(digitalRead(BTN_PIN), timer_now);

		if (wheel_rotated) {
			wheel_rotated = false;
			wheel_rotation_last_time = timer_now;
			if (wheel_rotation_start_time == 0) {
				wheel_rotation_start_time = timer_now;
			}

			wheel_rotation_counter++;
			distance += wheel_length;
		}

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

		_delay_ms(2);
    }
}

void attach_wheel_interrupt() {
	cli();
	
	// the rising edge of INT0 generates an interrupt
	MCUCR |= _BV(ISC00) | _BV(ISC01);
	
	// enable INT0
	GIMSK |= _BV(INT0);
	
	sei();
}

ISR(INT0_vect) {
	wheel_rotated = true;
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
	//display.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void switch_display_menu() {
	display_menu++;
	if (display_menu == 5) {
		display_menu = 0;
	}
}

void display_data() {
	/*
	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);

	switch(display_menu) {
		case MENU_MAIN:
			// speed
			display.setCursor(0, 0);
			dtostrf(speed, 4, 1, str_tmp);
			snprintf(buf, BUF_SIZE, "%s km/h", str_tmp);
			display.print(buf);

			// distance
			display.setCursor(0, 16);
			dtostrf(distance, 4, 2, str_tmp);
			snprintf(buf, BUF_SIZE, "%s km", str_tmp);
			display.print(buf);
			break;
		
		case MENU_SPEED:
			// max speed
			display.setCursor(0, 0);
			dtostrf(max_speed, 4, 1, str_tmp);
			snprintf(buf, BUF_SIZE, "%s km/h", str_tmp);
			display.print(buf);
		
			// avg speed
			display.setCursor(0, 16);
			dtostrf(avg_speed, 4, 1, str_tmp);
			snprintf(buf, BUF_SIZE, "%s km/h", str_tmp);
			display.print(buf);
			break;

		case MENU_RPM:
			// rpm
			display.setCursor(0, 0);
			snprintf(buf, BUF_SIZE, "%d rpm", wheel_rpm);
			display.print(buf);
			break;
		
		case MENU_POWER: // power save mode
			display.setCursor(0, 0);
			display.print("pwr save:");

			display.setCursor(0, 16);
			display.print(pwr_save_mode ? "on" : "off");
			break;

		case MENU_VOLTAGE: // input voltage
			display.setCursor(0, 0);
			display.print("voltage:");

			dtostrf(read_voltage(), 4, 1, str_tmp);
			snprintf(buf, BUF_SIZE, "%s v", str_tmp);

			display.setCursor(0, 16);
			display.print(buf);
			break;

		case MENU_TEMP:
			display.setCursor(0, 0);
			display.print("temp:");

			snprintf(buf, BUF_SIZE, "%d v", read_temp());
			display.print(buf);
			break;

		case MENU_LED:
			display.setCursor(0, 0);
			display.print("led:");

			display.setCursor(0, 16);
			display.print(led_turned ? "on" : "off");
			break;
	}

	display.display();
	*/
}

int8_t read_temp() {
	ADMUX = 0;
	// int reading = analogRead(TEMP_PIN);
	int reading = 0;

	// преобразовываем полученные данные в напряжение. Если используем Arduino 3.3 В, то меняем константу на 3.3
	float voltage = reading * 5.0;
	voltage /= 1024.0;

	//конвертируем 10 мВ на градус с учетом отступа 500 мВ
	return (voltage - 0.5) * 100;
}

float read_voltage() {
	// Read 1.1V reference against AVcc
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	_delay_ms(2); // Wait for Vref to settle
	
	ADCSRA |= _BV(ADSC); // Convert
	loop_until_bit_is_clear(ADCSRA, ADSC);
	
	uint32_t result = ADCL;
	result |= ADCH << 8;
	result = 1126400L / result; // Back-calculate AVcc in mV
	return result / 1000.0;
}

void enable_pwr_save_mode(bool enable) {
	pwr_save_mode = !pwr_save_mode;
	eeprom_write_byte(&EEPROM_PWR_SAVE_MODE, pwr_save_mode);
}

void enable_sleep_mode() {
	turn_display(false);
	turn_led(false);
	sleep_mode = true;
	// LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
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

void turn_led(bool on) {
	led_turned = on;
	/*
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	if (on) {
		// disable interrupts
		cli();

		// set ctc mode
		TCCR1B |= (1<<WGM12);

		// set counter max value for 250 ms
		OCR1A = 3906; // 1953
		
		// set 1024 prescaler
		TCCR1B |= (1<<CS12) | (1<<CS10);

		// enable timer compare interrupt
		TIMSK1 |= (1<<OCIE1A);

		// allow interrupts
		sei();
	}
	*/
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

void detect_rotation() {
	wheel_rotated = true;
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

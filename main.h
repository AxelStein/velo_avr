
void attach_wheel_interrupt();

void calc_wheel_length();

void display_data();

void calc_wheel_length();

void set_wheel_diameter(uint8_t diameter);

void turn_display(bool on);

void switch_display_menu();

void display_data();

int8_t read_temp();

float read_voltage();

void enable_pwr_save_mode(bool enable);

void enable_sleep_mode();

void calc_avg_speed(float speed);

void turn_led(bool on);

void handle_btn_click(uint8_t pin_state, uint32_t timer_now);

void detect_rotation();

void calc_speed(uint32_t timer_now);
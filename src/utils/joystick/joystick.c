#include "joystick.h"

// implementações das funções
void joystick_init() {
    adc_init();
    adc_gpio_init(ANALOG_X);
    adc_gpio_init(ANALOG_Y);
}

uint16_t joystick_read_y() {
    adc_select_input(0);
    return adc_read();
}

JoystickState joystick_get_state() {
    uint16_t value = joystick_read_y();
    if (value > JOY_UP_THRESHOLD) return JOY_UP;
    if (value < JOY_DOWN_THRESHOLD) return JOY_DOWN;
    return JOY_IDLE;
}
#include "leds.h"

void leds_init() {

    // inicia o led verde
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);

    // inicia o led vermelho
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);
}

void leds_turn_on(uint pin) {
    gpio_put(pin, 1);
}

void leds_turn_off(uint pin) {
    gpio_put(pin, 0);
}
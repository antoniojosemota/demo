#ifndef LEDS_H
#define LEDS_H

// inclusão de bibliotecas
#include "pico/stdlib.h"

// definição das pinagens
#define LED_GREEN_PIN 11
#define LED_RED_PIN 13

// definição de funções
void leds_init();
void leds_turn_on(uint pin);
void leds_turn_off(uint pin);


#endif
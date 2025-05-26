#ifndef JOYSTICK_H
#define JOYSTICK_H

// inclusão de bibliotecas
#include "pico/stdlib.h"
#include "hardware/adc.h"

// definição da pinagem
#define ANALOG_X 26
#define ANALOG_Y 27

// Definições para obter o estado do joystick
#define JOY_DEADZONE 400
#define JOY_CENTER 1915
#define JOY_UP_THRESHOLD    (JOY_CENTER + JOY_DEADZONE)
#define JOY_DOWN_THRESHOLD  (JOY_CENTER - JOY_DEADZONE)

typedef enum {
    JOY_IDLE,       // parado
    JOY_UP,         // moveu para cima
    JOY_DOWN        // moveu para baixo
} JoystickState;

// definição das funções
void joystick_init();
uint16_t joystick_read_y();
JoystickState joystick_get_state();

#endif
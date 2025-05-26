#ifndef DISPLAY_H
#define DISPLAY_H

// inclusão de drivers e bibliotecas
#include <stdio.h>
#include "hardware/i2c.h"
#include "ssd1306.h"

// definição da pinagem do display
#define I2C_SDA 14
#define I2C_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display

extern ssd1306_t display;

// definição de funções
void display_init();
void display_write(const char *msg, uint x, uint y, uint size);
void display_show();
void display_clear();
void txt_display(int64_t sec, char *task_name);

#endif

#include "display.h"
#include "string.h"

ssd1306_t display;

// implementação das funções
void display_init() {

    // inicializando o canal i2c
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // iniciando o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) { 
        printf("Falha ao inicializar o display SSD1306\n");
    }
}

void display_write(const char *msg, uint x, uint y, uint size) {
    ssd1306_draw_string(&display, x, y, size, msg);
}

void display_show() {
    ssd1306_show(&display);
}

void display_clear() {
    ssd1306_clear(&display);
}

void txt_display(int64_t sec, char *task_name){
    display_clear(&display);

    // exibindo a task atual
    display_write(task_name, 1, 0, 1);

    char timer[32];
    snprintf(timer, sizeof(timer), "%.2f", sec / 1e6);
    int x = (128 - (strlen(timer) * 6)) / 2;
    display_write(timer, x, 30, 1);
    display_show();
}
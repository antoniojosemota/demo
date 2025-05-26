#include "buttons.h"

// implementação das funções
void buttons_init() {

    // iniciando botão A
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A); // Habilita o pull-up interno para o botão

    // iniciando botão B
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
}
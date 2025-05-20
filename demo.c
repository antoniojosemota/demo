#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define SSID_WIFI ""
#define PASS_WIFI ""

#define BTN_A 5
#define BTN_B 6
#define ANALOG_X 26
#define ANALOG_Y 27
#define LED_GREEN_PIN 11
#define LED_RED_PIN 13
#define I2C_SDA 14
#define I2C_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display

ssd1306_t display;


void setup(){
    stdio_init_all();

    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Configuração dos LEDs como saída
    adc_init();
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A); // Habilita o pull-up interno para o botão

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);

    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);

    adc_gpio_init(ANALOG_X);
    adc_gpio_init(ANALOG_Y);

    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) { 
        printf("Falha ao inicializar o display SSD1306\n");
    }

    while(cyw43_arch_init()){
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return;
    }

    cyw43_arch_enable_sta_mode();

    if(cyw43_arch_wifi_connect_timeout_ms(SSID_WIFI, PASS_WIFI, CYW43_AUTH_WPA2_MIXED_PSK, 20000)){
        printf("Falha ao conectar com a rede\n");
        return;
    }

    printf("Wi-Fi Conectado\n");

    if(netif_default){
        printf("IP do Dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }


}


int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

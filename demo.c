#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define SSID_WIFI "20c"
#define PASS_WIFI "12345678"

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
volatile bool running = false;
volatile bool running2 = false;
volatile bool task_completed = false;
volatile bool service_completed = false;
absolute_time_t start_time;
absolute_time_t start_time2;
int64_t elapsed_us = 0;
int64_t elapsed_us2 = 0;
int num_task = 0;
int qntTasks = 5;

typedef struct
{
    char state_button[50];
    int id;
    double time; 
}datab;

datab* state = NULL;


void txt_display(int64_t sec){
    char timer[32];
    snprintf(timer, sizeof(timer), "%.2f", sec / 1e6);
    int x = (128 - (strlen(timer) * 6)) / 2;
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, x, 30, 1, timer);
    ssd1306_show(&display);
}

void gpio_callback(uint gpio, uint32_t events) {
    
    if (gpio == BTN_A && (events & GPIO_IRQ_EDGE_FALL)) {
        
        if (!running) {
            running = true;
            running2 = true;
            start_time = get_absolute_time();
            start_time2 = get_absolute_time();
            gpio_put(LED_RED_PIN, 1);  // Liga LED vermelho
            gpio_put(LED_GREEN_PIN, 0);  // Liga LED vermelho
            strcpy(state->state_button, "Ativado");
        }
    }

    if (gpio == BTN_B && (events & GPIO_IRQ_EDGE_FALL)) {
        
        if (running && running2) {
            
            elapsed_us2 = absolute_time_diff_us(start_time2, get_absolute_time());
            printf("Tempo da tarefa %d: %.2f segundos\n", num_task + 1,  elapsed_us2 / 1e6);
            state[num_task].id = num_task + 1;
            state[num_task].time = elapsed_us2 / 1e6;
            start_time2 = get_absolute_time();
            num_task += 1;

            if (num_task >= qntTasks){
                num_task = 0;
                service_completed = true;
            }

            if(service_completed){
                elapsed_us = absolute_time_diff_us(start_time, get_absolute_time()); // <-- Adicione isso
                running = false;
                running2 = false; 
                service_completed = false;
                gpio_put(LED_RED_PIN, 0);  // Desliga LED vermelho
                gpio_put(LED_GREEN_PIN, 1);  // Liga LED verde
                for(int i = 0; i <= qntTasks; i++){
                    printf("Tarefa %d: %.2f segundos\n", state[i].id, state[i].time);
                }
                printf("Tempo: %.2f segundos\n", elapsed_us / 1e6);
                strcpy(state->state_button, "Desligado");
        }
    }
}
}


static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    tcp_recved(tpcb,p->len);

    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);
    
    absolute_time_t last_update_html = get_absolute_time();

    last_update_html = get_absolute_time();

    if (strstr(request, "GET /data") != NULL)
    {
        char json_body[128];
        snprintf(json_body, sizeof(json_body), "{\"sec\": %.2f, \"button\": \"%s\"}", elapsed_us / 1e6, state->state_button);

        char json[256];
        snprintf(json, sizeof(json),
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n" 
            "Content-Type: application/json\r\n"
            "Cache-Control: no-cache\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            (int)strlen(json_body), json_body);

        tcp_write(tpcb, json, strlen(json), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);

        tcp_close(tpcb);                             
        tcp_recv(tpcb, NULL);

        free(request);
        pbuf_free(p);
        return ERR_OK;
    }

    // Página HTML principal
    char html[2048];
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Connection: close\r\n" 
             "Content-Type: text/html\r\n"
             "Cache-Control: no-cache\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<title>Joystick Monitor</title>\n"
             "<style>\n"
             "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 48px; }\n"
             ".data { font-size: 36px; margin-top: 20px; }\n"
             "</style>\n"
             "<script>\n"
             "function updateData() {\n"
             "  fetch('/data').then(r => r.json()).then(data => {\n"
             "    document.getElementById('sec').textContent = data.sec;\n"
             "    document.getElementById('button').textContent = data.button;\n"
             "  });\n"
             "}\n"
             "setInterval(updateData, 1);\n"
             "window.onload = updateData;\n"
             "</script>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Monitoramento</h1>\n"
             "<div class=\"data\">Segundos: <span id=\"sec\">-</span></div>\n"
             "<div class=\"data\">Estado do botão: <span id=\"button\">-</span></div>\n"
             "</body>\n"
             "</html>\n");

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    tcp_close(tpcb);
    tcp_recv(tpcb, NULL);
    
    free(request);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err){
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}


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

/*     while(cyw43_arch_init()){
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return;
    }

    cyw43_arch_enable_sta_mode();

    if(cyw43_arch_wifi_connect_timeout_ms(SSID_WIFI, PASS_WIFI, CYW43_AUTH_WPA2_MIXED_PSK, 20000)){
        printf("Falha ao conectar com a rede\n");
        return;
    }

    cyw43_arch_gpio_put(LED_GREEN_PIN, 1);
    printf("Wi-Fi Conectado\n");
    sleep_ms(1000);
    cyw43_arch_gpio_put(LED_GREEN_PIN, 0);


    if(netif_default){
        printf("IP do Dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }
     */

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true);

}

void config_server(){
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");
}

int main()
{
    setup();

    //config_server();

    state = malloc((qntTasks *2) * sizeof(int));

    absolute_time_t last_update = get_absolute_time();


    while (true) {
        if (running && running2) {
            elapsed_us = absolute_time_diff_us(start_time, get_absolute_time());

        }

        if (absolute_time_diff_us(last_update, get_absolute_time()) > 100000) {
            last_update = get_absolute_time();
            txt_display(elapsed_us);
    }
}
}   

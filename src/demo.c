// bibliotecas padrões
#include <stdio.h>
#include "string.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "lwip/netif.h"

// bibliotecas customizadas
#include "display.h"
#include "leds.h"
#include "buttons.h"
#include "joystick.h"
#include "wifi.h"
#include "server.h"
#include "service_data.h"

// variáveis de controle para a escolha do serviço
int selected_service_id = 0;        // 0 -> nenhum foi selecionado
int current_option = 0;

// timer para tratar o debounc
struct repeating_timer button_debouncing;
bool button_is_active = false;

// variáveis de controle de execução das tarefas
volatile bool running = false;
volatile bool running2 = false;
volatile bool task_completed = false;
volatile bool service_completed = false;
int num_task = 0;
int qntTasks = 0;

// variáveis para manipular os timers
absolute_time_t start_time;
absolute_time_t start_time2;
int64_t elapsed_us = 0;
int64_t elapsed_us2 = 0;

// function to init wifi connection and show feedback on display
int init_wifi_connection() {

    // displaying the name of the wifi network
    display_clear();
    display_write("Conectando em:", 23, 20, 1);
    display_write(WIFI_SSID, 23, 32, 1);
    display_show();

    // trying to connect to the network and retrieving connection status
    int state_connection = wifi_connect();

    // show state of connection on display
    display_clear();
    if (state_connection == 0) {
        display_write("CONEXAO ESTABECIDA", 10, 30, 1);
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    } else {
        display_write("FALHA NA CONEXAO", 10, 30, 1);
    }
    display_show();
    sleep_ms(3000);

    return state_connection;
}

// Função para reativar os botões
bool reaneble_button_callback() {
    button_is_active = true;
    return false;
}

// Função para tratar o efeito bounce nos botões
void debouncing() {
    button_is_active = false;
    cancel_repeating_timer(&button_debouncing);
    add_repeating_timer_ms(300, reaneble_button_callback, NULL, &button_debouncing);
}

// Função responsável por permitir o usuário selecionar um dos seviços disponíveis
void select_service(Service *services, int qtd_services) {
    JoystickState state;
    draw_services_on_display(services, qtd_services, current_option);
    // enquanto não selecionar um serviço
    while(selected_service_id == 0) {
        state = joystick_get_state();
        // checando o estado do joystick para navegar entre os serviços
        if (state == JOY_UP && current_option > 0) {
            --current_option;
        } else if (state == JOY_DOWN && current_option < qtd_services-1) {
            ++current_option;
        }
        draw_services_on_display(services, qtd_services, current_option);
        sleep_ms(200);
    }
}

// Função responsável por carregar os serviços e levar o usuário a escolha
void choose_service() {

    // busca pelos serviços no servidor
    server_get_services();
    sleep_ms(2000);

    // reseta o sected_id
    selected_service_id = 0;

    Service *services = get_services();
    int quant_services = get_total_services();

    // escolha do serviço
    select_service(services, quant_services);
    selected_service_id = services[current_option].id;
    printf("Servico escolhido id: %d - %s\n", selected_service_id, services[current_option].name);

    // chama o endpoint que puxa os dados do serviço pelo seu id
    server_get_service_by_id(selected_service_id);
    sleep_ms(1000);

    // Obtendo os dados do serviço selecionado
    SelectedService *selected_service = get_service_details();

    // atualizando a variável que armazena a quantidade de tarefas (Tasks)
    qntTasks = selected_service->quantity_tasks;
}

// Função para exibir resultado final
void show_final_duration(float total) {
    leds_turn_off(LED_GREEN_PIN);
    display_clear();
    char msg[40];
    snprintf(msg, sizeof(msg), "Tempo Servico: %.2f", total);
    display_write(msg, 0, 32, 1);
    display_show();

    // await 5s
    sleep_ms(5000);
    choose_service();
}

// Callback para as interrupções dos botões A e B
void gpio_callback(uint gpio, uint32_t events) {
    
    if (!button_is_active) return;

    if (gpio == BTN_A && (events & GPIO_IRQ_EDGE_FALL)) {
        debouncing();
        if (!running) {
            running = true;
            running2 = true;
            start_time = get_absolute_time();
            start_time2 = get_absolute_time();
            leds_turn_on(LED_RED_PIN);          // Liga LED vermelho
            leds_turn_off(LED_GREEN_PIN);       // Desliga o LED verde
        }
        return;
    }

    // para confirmar a opção selecionada
    if (selected_service_id == 0 && gpio == BTN_B) {
        debouncing();
        ++selected_service_id;
        return;
    }

    if (gpio == BTN_B && (events & GPIO_IRQ_EDGE_FALL)) {

        debouncing();
        
        if (running && running2) {
            
            elapsed_us2 = absolute_time_diff_us(start_time2, get_absolute_time());

            // atualizando o valor de duração da task finalizada
            get_service_details()->tasks[num_task].duration_in_seconds = elapsed_us2 / 1e6;
            start_time2 = get_absolute_time();
            num_task += 1;

            if (num_task >= qntTasks){
                num_task = 0;
                service_completed = true;
            }
        }
        return;
    }
}

// Função para inicializar os dispositivos
void setup(){
    stdio_init_all();

    // iniciando o display
    display_init();

    // iniciando leds
    leds_init();

    // iniciando botões A e B
    buttons_init();
    button_is_active = true;

    // iniciando o joystick
    joystick_init();

    // configurando interrupção para os botões A e B
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true);

}

int main()
{

    // fazendo a configuração inicial dos dispositios
    setup();

    // tentando se conectar a rede
    if (init_wifi_connection() == 0) {
        // busca pelos serviços e faz com que o usuário escolha um
        choose_service();
    }
    
    absolute_time_t last_update = get_absolute_time();

    // obtendo a lista de tarefas (tasks) do serviço escolhido
    Task *tasks = malloc(sizeof(Task) * qntTasks);
    for (int i = 0; i < qntTasks; i++) {
        tasks[i] = get_service_details()->tasks[i];
        printf("Task id: %d - name: %s\n", tasks[i].id, tasks[i].name);
    }

    while (true) {

        if (running && running2) {
            elapsed_us = absolute_time_diff_us(start_time, get_absolute_time());
        }

        if (absolute_time_diff_us(last_update, get_absolute_time()) > 100000) {
            last_update = get_absolute_time();
            txt_display(elapsed_us, tasks[num_task].name);
        }

        // caso o serviço esteja terminado
        if(service_completed){
            elapsed_us = absolute_time_diff_us(start_time, get_absolute_time()); 
            running = false;
            running2 = false; 
            service_completed = false;
            leds_turn_off(LED_RED_PIN);     // Desliga LED vermelho
            leds_turn_on(LED_GREEN_PIN);    // Liga LED verde

            // Obtendo informações do serviço para exibir o tempo que cada tarefa foi executada
            SelectedService *s = get_service_details();

            // enviando duração das tarefas para o servidor
            server_send_duration_tasks(*s);

            // exibindo tempo de cada tarefa e o tempo total do serviço
            double total = 0.0;
            for (int i = 0; i < qntTasks; i++) {
                printf("\nid: %d - name %s - time: %.2f s", s->tasks[i].id, s->tasks[i].name, s->tasks[i].duration_in_seconds);
                total += s->tasks[i].duration_in_seconds;
            }
            printf("\nTempo total Servico: %.2f\n", total);

            show_final_duration(total);
        }
    }
}   

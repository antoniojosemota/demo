#ifndef SERVICE_DATA_H
#define SERVICE_DATA_H

#include <stdint.h>

// Estrutura para armazenar dados de cada serviço recebido da API
typedef struct {
    int id;
    char name[50];
} Service;

// Estrutura para armazenar dados de cada Task associado ao serviço selecionado
typedef struct {
    int id;
    char name[50];
    float duration_in_seconds;
} Task;

// Estrutura para armazenar o serviço selecionado e tempos de tarefa
typedef struct {
    int id;
    char name[50];
    int quantity_tasks;
    Task* tasks;          // array dinâmico de tasks
    double total_time;
} SelectedService;


// definição das funções
void save_services(char *json_response);
void save_service_details(char *json_response);
Service* get_services();
int get_total_services();
SelectedService* get_service_details();
void draw_services_on_display(Service * service, int qtd_serv, int current_option);

#endif

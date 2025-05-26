#include "service_data.h"
#include "parson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"

// Máximo de serviços que podem vir da API
#define MAX_SERVICES 20

// Variáveis globais
Service services[MAX_SERVICES];
int total_services = 0;
SelectedService selected_service;

// Armazena os serviços vindos da API (simulado por enquanto)
void save_services(char *json_response) {

    JSON_Value *root_value;
    JSON_Array *array;

    // Faz o parse da string JSON
    root_value = json_parse_string(json_response);
    if (root_value == NULL) {
        printf("Falha ao fazer parse do JSON.\n");
        return;
    }

    // Acessa o array que está na raiz
    array = json_value_get_array(root_value);

    // Itera sobre os objetos no array
    for (int i = 0; i < json_array_get_count(array); i++) {
        JSON_Object *obj = json_array_get_object(array, i);
        const char *name = json_object_get_string(obj, "name");
        int id = (int)json_object_get_number(obj, "id");
        
        // salva os dados dos serviços na estrutura
        services[i].id = id;
        strcpy(services[i].name, name);

    }

    total_services = json_array_get_count(array);

    // Libera memória
    json_value_free(root_value);

}

// Armazena os dados do serviço selecionado, incluindo suas Tarefas (Tasks)
void save_service_details(char *json_response) {
    JSON_Value *root_value;
    JSON_Object *root_object;
    JSON_Array *tasks_array;

    // Parse do JSON recebido
    root_value = json_parse_string(json_response);
    if (root_value == NULL) {
        printf("Falha ao fazer parse do JSON.\n");
        return;
    }

    root_object = json_value_get_object(root_value);
    if (root_object == NULL) {
        printf("Erro ao obter objeto raiz do JSON.\n");
        json_value_free(root_value);
        return;
    }

    // Pega os dados do serviço
    selected_service.id = (int)json_object_get_number(root_object, "id");
    const char *name = json_object_get_string(root_object, "name");
    strcpy(selected_service.name, name);

    selected_service.quantity_tasks = (int)json_object_get_number(root_object, "totalTasks");

    // Aloca memória para tasks
    if (selected_service.quantity_tasks > 0) {
        selected_service.tasks = malloc(sizeof(Task) * selected_service.quantity_tasks);
    } else {
        selected_service.tasks = NULL;
    }

    // Pega o array de tasks
    tasks_array = json_object_get_array(root_object, "tasks");

    // Itera sobre tasks e preenche a estrutura
    for (int i = 0; i < selected_service.quantity_tasks; i++) {
        JSON_Object *task_obj = json_array_get_object(tasks_array, i);
        selected_service.tasks[i].id = (int)json_object_get_number(task_obj, "id");
        const char *task_name = json_object_get_string(task_obj, "taskName");
        strcpy(selected_service.tasks[i].name, task_name);

        // Inicializa tempo para zero
        selected_service.tasks[i].duration_in_seconds = 0.0;
    }

    selected_service.total_time = 0.0;

    json_value_free(root_value);
}

// Funções de acesso
Service* get_services() {
    return services;
}

int get_total_services() {
    return total_services;
}

SelectedService* get_service_details() {
    return &selected_service;
}

// Função que desenha no display os serviços e o que está selecionado
void draw_services_on_display(Service * service, int qtd_serv, int current_option) {
    display_clear();

    display_write("Servicos", 30, 0, 1);

    for (int i = 0; i < qtd_serv; i++) {
        int _x = 7;
        int _y = (i+1) * 12;

        if (i == current_option) {
            display_write(">", _x, _y, 1);
            display_write(service[i].name, _x+9, _y, 1);
        } else {
            display_write(service[i].name, _x, _y, 1);
        }
    }

    display_show();
}
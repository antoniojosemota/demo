// inclusions libraries
#include "server.h"
#include "parson.h"
#include "service_data.h"

// tcp_pcb struct to stablish connection
struct tcp_pcb *pcb = NULL;

// variables and definition to control connetion attemps
bool have_connection = false;
#define MAX_RETRIES 5
int retries = 0;

// implementation functions

char *get_json_response(struct pbuf *p) {
    // get_and_show_json_response(p);
    char *response = (char*)p->payload;
    char *body = strstr(response, "\r\n\r\n");
    if (body != NULL) {
        body += 4; // pula os 4 caracteres da quebra de linha

        // pula a linha com o tamanho di json
        char *json_start = strchr(body, '\n');
        return json_start;
    }
    return NULL;
}

void handle_durations_response(struct pbuf *p) {
    // printf("Response: %s\n", (char *)p->payload);
    server_close_tcp_connection();
}

void handle_services_response(struct pbuf *p) {
    // get_and_show_json_response(p);
    char *json_response = get_json_response(p);
    if (json_response != NULL) {
        json_response += 1; // avança para depois da quebra de linha
        save_services(json_response);
    }
    server_close_tcp_connection();
}

void handle_service_details_response(struct pbuf *p) {

    char *json_response = get_json_response(p);

    if (json_response != NULL) {
        json_response += 1;
        
        save_service_details(json_response);
    }
    server_close_tcp_connection();
}


// Callback for server response
err_t server_tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        printf("Connection closed by the server.\n");
        tcp_close(tpcb);
        have_connection = false;
        pcb = NULL;
        return ERR_OK;
    }

    tcp_client_context_t *context = (tcp_client_context_t*) arg;
    if (context && context->response_callback) {
        context->response_callback(p);  // chama o callback associado
    } else {
        printf("No response callback set.\n");
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

// Callback para erro na conexão TCP
void server_tcp_client_error(void *arg, err_t err) {
    printf("TCP connection error (%d). Trying to reconnect..\n", err);
    pcb = NULL;
    have_connection = false;

    tcp_client_context_t *context = (tcp_client_context_t*) arg;
    if (context) {
        if (retries < MAX_RETRIES) {
            server_create_tcp_connection(context->response_callback);
        } else {
            printf("Maximum number of attempts reached. Aborting.\n");
        }
        free(context);  // libera o antigo contexto
    } else {
        printf("Error: missing context to reconnect.\n");
    }
}

// Close tcp connection
void server_close_tcp_connection() {
    if (pcb != NULL) {
        tcp_close(pcb);
        pcb = NULL;
        have_connection = false;
        printf("TCP connection closed.\n");
    }
}

err_t server_tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Falha ao conectar: %d\n", err);
        tcp_abort(tpcb);
        pcb = NULL;
        return err;
    }

    printf("Conexão estabelecida com sucesso!\n");
    have_connection = true;

    // Define callback de recv aqui
    tcp_recv(tpcb, server_tcp_client_recv);

    return ERR_OK;
}

void server_create_tcp_connection(tcp_response_callback_t callback) {
    // Se já existe um pcb ativo, encerra ele antes
    if (pcb != NULL) {
        tcp_abort(pcb);
        pcb = NULL;
    }

    retries++;

    // server config
    ip_addr_t server_ip;
    server_ip.addr = ipaddr_addr(SERVER_IP);

    // err_t resolved_dns = dns_gethostbyname(SERVER_IP, &server_ip, NULL, NULL);

    // if (resolved_dns != ERR_OK) {
    //     printf("Failed to resolve dns\n");
    //     return;
    // }

    pcb = tcp_new();
    if (!pcb) {
        printf("Falha ao criar pcb\n");
        return;
    }

    tcp_err(pcb, server_tcp_client_error);

    // Aloca contexto e valida
    tcp_client_context_t *context = malloc(sizeof(tcp_client_context_t));
    if (!context) {
        printf("Falha ao alocar contexto\n");
        tcp_abort(pcb);
        pcb = NULL;
        return;
    }
    context->response_callback = callback;
    tcp_arg(pcb, context);

    // Agora sim — passando o callback de conexão certo
    err_t connect_err = tcp_connect(pcb, &server_ip, SERVER_PORT, server_tcp_connected);
    if (connect_err != ERR_OK) {
        printf("Falha na tentativa de conexão (%d). Tentativa %d/%d\n", connect_err, retries, MAX_RETRIES);
        tcp_abort(pcb);
        pcb = NULL;
        free(context);
    }
}

void server_get_services() {
    while (pcb == NULL || !have_connection) {
        if (retries >= MAX_RETRIES) {
            printf("Error: Connection could not be re-established after %d attempts.\n", MAX_RETRIES);
            return;  // Aborts if the maximum number of reconnection attempts is reached
        }

        printf("Connection lost. Trying to reconnect... (%d/%d)\n", retries, MAX_RETRIES);
        server_create_tcp_connection(handle_services_response);
        sleep_ms(1000);
    }


    // Send a request to the server
    retries = 0;

    // preparing the request
    char request[256];
    snprintf(request, sizeof(request), 
    "GET /services HTTP/1.1\r\n"
    "Host: %s\r\n"
    "\r\n",
    SERVER_IP);

    // sending a request to the server
    if (tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY) != ERR_OK) {
        printf("Error sending data. Closing connection...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }

    if (tcp_output(pcb) != ERR_OK) {
        printf("Eerror when sending data (tcp_output). Closing connection...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }

}

void server_get_service_by_id(int id) {
    while (pcb == NULL || !have_connection) {
        if (retries >= MAX_RETRIES) {
            printf("Error: Connection could not be re-established after %d attempts.\n", MAX_RETRIES);
            return;  // Aborts if the maximum number of reconnection attempts is reached
        }

        printf("Connection lost. Trying to reconnect... (%d/%d)\n", retries, MAX_RETRIES);
        server_create_tcp_connection(handle_service_details_response);
        sleep_ms(1000);
    }

    // Send a request to the server
    retries = 0;

    // preparing the request
    char request[256];
    snprintf(request, sizeof(request), 
    "GET /services/%d HTTP/1.1\r\n"
    "Host: %s\r\n"
    "\r\n",
    id, SERVER_IP);


    // sending a request to the server
    if (tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY) != ERR_OK) {
        printf("Error sending data. Closing connection...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }

    if (tcp_output(pcb) != ERR_OK) {
        printf("Eerror when sending data (tcp_output). Closing connection...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }

}

// Função que monta o JSON da atualização de durações de tasks para um serviço
void buildServiceTaskUpdateJson(SelectedService service, char* output_buffer, size_t buffer_size) {
    char tasks_buffer[768] = "";  // aumentei um pouco aqui
    char task_json[128];

    for (int i = 0; i < service.quantity_tasks; i++) {
        snprintf(task_json, sizeof(task_json),
                 "{ \"taskId\": %d, \"durationInSeconds\": %.1f }%s",
                 service.tasks[i].id, service.tasks[i].duration_in_seconds,
                 (i < service.quantity_tasks - 1) ? "," : "");
        strcat(tasks_buffer, task_json);
    }

    snprintf(output_buffer, buffer_size,
             "{"
             "\"serviceId\": %d,"
             "\"tasks\": [ %s ]"
             "}",
             service.id, tasks_buffer);
}

void server_send_duration_tasks(SelectedService s) {
    while (pcb == NULL || !have_connection) {
        if (retries >= MAX_RETRIES) {
            printf("Error: Connection could not be re-established after %d attempts.\n", MAX_RETRIES);
            return;  // Aborts if the maximum number of reconnection attempts is reached
        }

        printf("Connection lost. Trying to reconnect... (%d/%d)\n", retries, MAX_RETRIES);
        server_create_tcp_connection(handle_durations_response);
        sleep_ms(1000);
    }


    // Send a request to the server
    retries = 0;

    char json_request[1024];
    buildServiceTaskUpdateJson(s, json_request, sizeof(json_request));

    char header[512];
    int content_length = strlen(json_request);
    snprintf(header, sizeof(header),
        "POST /services/update-task-durations HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        SERVER_IP, content_length);

    if (tcp_write(pcb, header, strlen(header), TCP_WRITE_FLAG_COPY) != ERR_OK) {
        printf("Erro enviando header. Fechando conexão...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }

    if (tcp_write(pcb, json_request, strlen(json_request), TCP_WRITE_FLAG_COPY) != ERR_OK) {
        printf("Erro enviando body. Fechando conexão...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }

    if (tcp_output(pcb) != ERR_OK) {
        printf("Erro finalizando envio (tcp_output). Fechando conexão...\n");
        tcp_abort(pcb);
        pcb = NULL;
        have_connection = false;
        return;
    }
}

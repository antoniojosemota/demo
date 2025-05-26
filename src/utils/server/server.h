#ifndef SERVER_H
#define SERVER_H

// inclusion of libraries
#include <string.h>
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "service_data.h"

// Tipos e definições
typedef void (*tcp_response_callback_t)(struct pbuf *p);

typedef struct {
    tcp_response_callback_t response_callback;
} tcp_client_context_t;

// server config
// #define SERVER_PROXY "mainline.proxy.rlwy.net"
#define SERVER_IP "35.212.89.16"
#define SERVER_PORT 53119

// definitions functions
void server_create_tcp_connection(tcp_response_callback_t callback);
void server_close_tcp_connection();
void server_tcp_client_error(void *arg, err_t err);
err_t server_tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

void server_get_services();
void server_get_service_by_id(int id);
void server_send_duration_tasks(SelectedService s);

#endif

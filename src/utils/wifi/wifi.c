// inclusion libraries
#include "wifi.h"
#include "pico/cyw43_arch.h"

// implementations functions
int wifi_connect() {

    int connected;
    // initialize of chip
    if (cyw43_arch_init()) {
        printf("Wi-fi init failed.\n");
        return 1;
    }

    // enable wifi client mode
    cyw43_arch_enable_sta_mode();

    // trying connection
    printf("Connecting Wifi...\n");
    connected = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000);
    
    // print log in terminal and return state of connection
    char *message_log = connected == 0? "Wifi conected!\n" : "Failed to connect.\n";
    printf(message_log);
    return connected;
}
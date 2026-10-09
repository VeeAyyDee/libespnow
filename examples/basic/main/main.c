/* SPDX-License-Identifier: MPL-2.0
 * Minimal ordinary esp_now.h application; radio startup remains SDK-owned. */
#include <stdio.h>
#include <string.h>
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static void received(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    (void)data;
    printf("ESP-NOW received %d bytes\n", len);
}

void app_main(void)
{
    /* For this example, NVS errors are reported rather than erasing user NVS. */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wifi = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(received));

    esp_now_peer_info_t broadcast = {0};
    memset(broadcast.peer_addr, 0xff, sizeof(broadcast.peer_addr));
    broadcast.channel = 6;
    broadcast.ifidx = WIFI_IF_STA;
    ESP_ERROR_CHECK(esp_now_add_peer(&broadcast));
    static const uint8_t message[] = "hello from libespnow";
    ESP_ERROR_CHECK(esp_now_send(broadcast.peer_addr, message, sizeof(message) - 1));
}

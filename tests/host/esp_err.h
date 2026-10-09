/* SPDX-License-Identifier: MPL-2.0
 * Minimal original host dependency shim; target builds use actual ESP-IDF. */
#ifndef HOST_ESP_ERR_H
#define HOST_ESP_ERR_H
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_WIFI_BASE 0x3000
#define ESP_ERR_WIFI_NOT_INIT 0x3001
#endif

/* SPDX-License-Identifier: MPL-2.0 */
#include "libespnow_internal.h"
#include <string.h>

/* Public Wi-Fi error value in RATE-OFFCHANNEL-01; avoids requiring an
 * unrelated public header to expose this symbol transitively. */
enum { ESPNOW_WIFI_NOT_INITIALIZED = 0x3001 };

/* These payload offsets are part of the private 32-bit wire/driver ABI even
 * in host tests. Pointer-containing request layout is asserted in abi_idf.c. */
static void store_u32(uint8_t *dest, uint32_t value)
{
    dest[0] = (uint8_t)value;
    dest[1] = (uint8_t)(value >> 8);
    dest[2] = (uint8_t)(value >> 16);
    dest[3] = (uint8_t)(value >> 24);
}

esp_err_t esp_now_switch_channel_tx(esp_now_switch_channel_t *config)
{
    if (!espnow_wifi_initialized()) { return ESPNOW_WIFI_NOT_INITIALIZED; }
    if (config == NULL) { return ESP_ERR_INVALID_ARG; }
    uint8_t *payload = espnow_alloc(48U + config->data_len);
    if (payload == NULL) { return ESP_ERR_NO_MEM; }
    memcpy(payload + 4, config->dest_mac, 6);
    store_u32(payload + 12, (uint32_t)config->type);
    payload[16] = config->channel;
    store_u32(payload + 20, (uint32_t)config->sec_channel);
    store_u32(payload + 24, config->wait_time_ms);
    store_u32(payload + 44, config->data_len);
    memcpy(payload + 48, config->data, config->data_len);
    /* Allocation failure must preserve op_id. Other driver results still
     * carry the driver's output operation id, including cancellation. */
    espnow_ioctl_t *request = espnow_alloc(sizeof(*request));
    if (request == NULL) {
        espnow_free(payload);
        return ESP_ERR_NO_MEM;
    }
    request->opcode = 0x55;
    request->handler = espnow_offchannel_handler(true);
    request->synchronous = 1;
    request->payload = payload;
    esp_err_t result = espnow_ioctl(request);
    config->op_id = payload[36];
    espnow_free(payload);
    return result;
}

esp_err_t esp_now_remain_on_channel(esp_now_remain_on_channel_t *config)
{
    if (!espnow_wifi_initialized()) { return ESPNOW_WIFI_NOT_INITIALIZED; }
    if (config == NULL) { return ESP_ERR_INVALID_ARG; }
    uint8_t payload[32] = {0};
    store_u32(payload + 4, (uint32_t)config->type);
    payload[8] = config->channel;
    store_u32(payload + 12, (uint32_t)config->sec_channel);
    store_u32(payload + 16, config->wait_time_ms);
    espnow_ioctl_t *request = espnow_alloc(sizeof(*request));
    if (request == NULL) { return ESP_ERR_NO_MEM; }
    request->opcode = 0x56;
    request->handler = espnow_offchannel_handler(false);
    request->synchronous = 1;
    request->payload = payload;
    esp_err_t result = espnow_ioctl(request);
    config->op_id = payload[24];
    return result;
}

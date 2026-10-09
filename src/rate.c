/* SPDX-License-Identifier: MPL-2.0 */
#include "libespnow_internal.h"
#include <string.h>

static esp_err_t validate_rate(const espnow_peer_t *peer,
                               const esp_now_rate_config_t *config)
{
    int mode = (int)config->phymode;
    int rate = (int)config->rate;
    if (mode < 0 || mode > 6 || rate < 0 || rate > 42) {
        return ESP_ERR_ESPNOW_ARG;
    }
    if (mode == 0) {
        return (rate == 41 || rate == 42) && espnow_lr_enabled(peer->interface)
            ? ESP_OK : ESP_ERR_ESPNOW_ARG;
    }
    uint8_t current_mode;
    if (!espnow_interface_mode(peer->interface, &current_mode)) {
        return ESP_ERR_ESPNOW_INTERNAL;
    }
    uint8_t channel = peer->channel != 0 ? peer->channel : espnow_current_channel();
    if (channel <= 14) {
        if ((mode >= 3 && current_mode < 3) || mode == 3
            || (mode < 3 && rate > 15) || (mode >= 3 && rate <= 15)) {
            return ESP_ERR_ESPNOW_ARG;
        }
    } else {
        if ((current_mode >= 5 && mode < 3)
            || (mode == 3 && (rate < 8 || rate > 15))
            || (mode >= 4 && rate <= 15)) {
            return ESP_ERR_ESPNOW_ARG;
        }
    }
    if (current_mode == 3 && mode >= 6) { return ESP_ERR_ESPNOW_ARG; }
    if (mode == 5 && (espnow_secondary_channel() == 0
                     || espnow_bandwidth(peer->interface, channel) != 2)) {
        return ESP_ERR_ESPNOW_ARG;
    }
    if (mode == 6) {
        if (config->ersu && ((rate >= 19 && rate <= 23) || rate > 26)) {
            return ESP_ERR_ESPNOW_ARG;
        }
        if (config->dcm && (rate == 18 || rate == 26 || rate < 16 || rate > 28
                           || (rate >= 21 && rate <= 23))) {
            return ESP_ERR_ESPNOW_ARG;
        }
    }
    return ESP_OK;
}

esp_err_t mt_set_peer_rate_config(const esp_now_peer_info_t *input,
                                  const esp_now_rate_config_t *config)
{
    if (input == NULL || config == NULL
        || (int)config->phymode < 0 || (int)config->phymode > 6
        || (int)config->rate < 0 || (int)config->rate > 42) {
        return ESP_ERR_ESPNOW_ARG;
    }
    espnow_peer_t *peer = mt_get_peer_info(input->peer_addr);
    if (peer == NULL) { return ESP_ERR_ESPNOW_NOT_FOUND; }
    esp_err_t result = validate_rate(peer, config);
    if (result != ESP_OK) { return result; }
    esp_now_rate_config_t *replacement = espnow_alloc(sizeof(*replacement));
    if (replacement == NULL) { return ESP_ERR_ESPNOW_NO_MEM; }
    /* Padding is deterministic, independent of caller's padding contents. */
    replacement->phymode = config->phymode;
    replacement->rate = config->rate;
    replacement->ersu = config->ersu;
    replacement->dcm = config->dcm;
    esp_now_rate_config_t *old = peer->rate;
    peer->rate = replacement;
    espnow_free(old);
    return ESP_OK;
}

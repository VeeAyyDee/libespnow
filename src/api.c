/* SPDX-License-Identifier: MPL-2.0 */
#include "libespnow_internal.h"
#include <string.h>

/* First use must be serialized by the integrator. The retained recursive mutex
 * then serializes public operations across ordinary deinit/reinit cycles. */
static esp_err_t enter(bool require_initialized)
{
    if (g_espnow_lock == NULL) {
        g_espnow_lock = espnow_mutex_create();
        if (g_espnow_lock == NULL) { return ESP_ERR_ESPNOW_NO_MEM; }
    }
    if (!espnow_mutex_lock(g_espnow_lock)) { return ESP_ERR_ESPNOW_INTERNAL; }
    if (require_initialized && !espnow_initialized()) {
        espnow_mutex_unlock(g_espnow_lock);
        return ESP_ERR_ESPNOW_NOT_INIT;
    }
    return ESP_OK;
}

/* The expression runs only after the appropriate gate and while locked. */
#define PUBLIC_CALL(initialized, expression) do { \
    esp_err_t result = enter(initialized); \
    if (result != ESP_OK) { return result; } \
    result = (expression); \
    espnow_mutex_unlock(g_espnow_lock); \
    return result; \
} while (0)

esp_err_t esp_now_init(void) { PUBLIC_CALL(false, mt_init()); }
esp_err_t esp_now_deinit(void) { PUBLIC_CALL(false, mt_deinit()); }
esp_err_t esp_now_get_version(uint32_t *version)
{ PUBLIC_CALL(false, mt_get_version(version)); }
esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb)
{ PUBLIC_CALL(true, mt_register_recv_cb(cb)); }
esp_err_t esp_now_unregister_recv_cb(void)
{ PUBLIC_CALL(true, mt_unregister_recv_cb()); }
esp_err_t esp_now_register_send_cb(esp_now_send_cb_t cb)
{ PUBLIC_CALL(true, mt_register_send_cb(cb)); }
esp_err_t esp_now_unregister_send_cb(void)
{ PUBLIC_CALL(true, mt_unregister_send_cb()); }
esp_err_t esp_now_send(const uint8_t *mac, const uint8_t *data, size_t len)
{ PUBLIC_CALL(true, mt_send(mac, data, len)); }
esp_err_t esp_now_add_peer(const esp_now_peer_info_t *peer)
{ PUBLIC_CALL(true, mt_add_peer(peer)); }
esp_err_t esp_now_del_peer(const uint8_t *mac)
{ PUBLIC_CALL(true, mt_del_peer(mac)); }
esp_err_t esp_now_mod_peer(const esp_now_peer_info_t *peer)
{ PUBLIC_CALL(true, mt_mod_peer(peer)); }
esp_err_t esp_now_get_peer(const uint8_t *mac, esp_now_peer_info_t *peer)
{ PUBLIC_CALL(true, mt_get_peer(mac, peer)); }
esp_err_t esp_now_fetch_peer(bool from_head, esp_now_peer_info_t *peer)
{ PUBLIC_CALL(true, mt_fetch_peer(from_head, peer)); }
esp_err_t esp_now_get_peer_num(esp_now_peer_num_t *num)
{ PUBLIC_CALL(true, mt_get_peer_num(num)); }
esp_err_t esp_now_set_pmk(const uint8_t *pmk)
{ PUBLIC_CALL(true, mt_set_pmk(pmk)); }
esp_err_t esp_now_set_wake_window(uint16_t window)
{ PUBLIC_CALL(true, mt_set_wake_window(window)); }
esp_err_t esp_now_set_user_oui(uint8_t *oui)
{ PUBLIC_CALL(false, mt_set_user_oui(oui)); }
esp_err_t esp_now_get_user_oui(uint8_t *oui)
{ PUBLIC_CALL(false, mt_get_user_oui(oui)); }

bool esp_now_is_peer_exist(const uint8_t *mac)
{
    if (enter(true) != ESP_OK) { return false; }
    bool exists = mt_is_peer_exist(mac);
    espnow_mutex_unlock(g_espnow_lock);
    return exists;
}

esp_err_t esp_now_set_peer_rate_config(const uint8_t *mac,
                                       esp_now_rate_config_t *config)
{
    esp_err_t result = enter(true);
    if (result != ESP_OK) { return result; }
    if (mac == NULL || config == NULL) {
        result = ESP_ERR_ESPNOW_ARG;
    } else {
        esp_now_peer_info_t peer;
        memset(&peer, 0, sizeof(peer));
        memcpy(peer.peer_addr, mac, sizeof(peer.peer_addr));
        result = mt_set_peer_rate_config(&peer, config);
    }
    espnow_mutex_unlock(g_espnow_lock);
    return result;
}

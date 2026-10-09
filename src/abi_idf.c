/* SPDX-License-Identifier: MPL-2.0
 * Only this file knows the pinned SDK's private globals and function table. */
#include "libespnow_internal.h"
#include <string.h>

#define OFFSET(type, field, offset) \
    _Static_assert(offsetof(type, field) == (offset), #type "." #field " ABI")
#define SIZE(type, size) _Static_assert(sizeof(type) == (size), #type " size ABI")
SIZE(void *, 4);
SIZE(esp_now_peer_info_t, 36);
OFFSET(esp_now_peer_info_t, peer_addr, 0);
OFFSET(esp_now_peer_info_t, lmk, 6);
OFFSET(esp_now_peer_info_t, channel, 22);
OFFSET(esp_now_peer_info_t, ifidx, 24);
OFFSET(esp_now_peer_info_t, encrypt, 28);
OFFSET(esp_now_peer_info_t, priv, 32);
SIZE(esp_now_rate_config_t, 12);
OFFSET(esp_now_rate_config_t, phymode, 0);
OFFSET(esp_now_rate_config_t, rate, 4);
OFFSET(esp_now_rate_config_t, ersu, 8);
OFFSET(esp_now_rate_config_t, dcm, 9);
SIZE(wifi_tx_info_t, 28);
OFFSET(wifi_tx_info_t, des_addr, 0);
OFFSET(wifi_tx_info_t, src_addr, 4);
OFFSET(wifi_tx_info_t, ifidx, 8);
OFFSET(wifi_tx_info_t, data, 12);
OFFSET(wifi_tx_info_t, data_len, 16);
OFFSET(wifi_tx_info_t, rate, 20);
OFFSET(wifi_tx_info_t, tx_status, 24);
SIZE(esp_now_recv_info_t, 12);
OFFSET(esp_now_recv_info_t, src_addr, 0);
OFFSET(esp_now_recv_info_t, des_addr, 4);
OFFSET(esp_now_recv_info_t, rx_ctrl, 8);
SIZE(espnow_state_t, 60);
OFFSET(espnow_state_t, pmk, 0);
OFFSET(espnow_state_t, reserved, 16);
OFFSET(espnow_state_t, head, 28);
OFFSET(espnow_state_t, tail, 32);
OFFSET(espnow_state_t, total, 36);
OFFSET(espnow_state_t, encrypted, 40);
OFFSET(espnow_state_t, cursor, 44);
OFFSET(espnow_state_t, recv_cb, 48);
OFFSET(espnow_state_t, send_cb, 52);
OFFSET(espnow_state_t, wake_window, 56);
SIZE(espnow_peer_t, 28);
OFFSET(espnow_peer_t, next, 0);
OFFSET(espnow_peer_t, previous, 4);
OFFSET(espnow_peer_t, channel, 8);
OFFSET(espnow_peer_t, interface, 9);
OFFSET(espnow_peer_t, mac, 10);
OFFSET(espnow_peer_t, priv, 16);
OFFSET(espnow_peer_t, key, 20);
OFFSET(espnow_peer_t, rate, 24);
SIZE(espnow_key_t, 192);
OFFSET(espnow_key_t, index, 0);
OFFSET(espnow_key_t, opaque, 2);
OFFSET(espnow_key_t, cipher, 160);
OFFSET(espnow_key_t, opaque_word, 164);
OFFSET(espnow_key_t, wrapped_lmk, 168);
OFFSET(espnow_key_t, length, 184);
OFFSET(espnow_key_t, cipher_id, 185);
OFFSET(espnow_key_t, hardware_index, 186);
OFFSET(espnow_key_t, reserved, 187);
SIZE(espnow_ioctl_t, 24);
OFFSET(espnow_ioctl_t, opcode, 0);
OFFSET(espnow_ioctl_t, handler, 4);
OFFSET(espnow_ioctl_t, synchronous, 8);
OFFSET(espnow_ioctl_t, reserved1, 10);
OFFSET(espnow_ioctl_t, reserved2, 12);
OFFSET(espnow_ioctl_t, reserved3, 16);
OFFSET(espnow_ioctl_t, payload, 20);
OFFSET(esp_now_switch_channel_t, type, 0);
OFFSET(esp_now_switch_channel_t, channel, 4);
OFFSET(esp_now_switch_channel_t, sec_channel, 8);
OFFSET(esp_now_switch_channel_t, wait_time_ms, 12);
OFFSET(esp_now_switch_channel_t, op_id, 16);
OFFSET(esp_now_switch_channel_t, dest_mac, 17);
OFFSET(esp_now_switch_channel_t, data_len, 24);
OFFSET(esp_now_switch_channel_t, data, 26);
OFFSET(esp_now_remain_on_channel_t, type, 0);
OFFSET(esp_now_remain_on_channel_t, channel, 4);
OFFSET(esp_now_remain_on_channel_t, sec_channel, 8);
OFFSET(esp_now_remain_on_channel_t, wait_time_ms, 12);
OFFSET(esp_now_remain_on_channel_t, op_id, 16);

typedef struct {
    uint8_t reserved0[0x4c];
    void *(*recursive_mutex_create)(void);
    void (*mutex_delete)(void *);
    int (*mutex_lock)(void *);
    int (*mutex_unlock)(void *);
    uint8_t reserved1[0xb0 - 0x5c];
    void (*free)(void *);
    uint8_t reserved2[0x168 - 0xb4];
    void *(*wifi_malloc)(size_t);
    void *(*wifi_realloc)(void *, size_t);
    void *(*wifi_calloc)(size_t, size_t);
    void *(*wifi_zalloc)(size_t);
} espnow_osi_t;
OFFSET(espnow_osi_t, recursive_mutex_create, 0x4c);
OFFSET(espnow_osi_t, mutex_delete, 0x50);
OFFSET(espnow_osi_t, mutex_lock, 0x54);
OFFSET(espnow_osi_t, mutex_unlock, 0x58);
OFFSET(espnow_osi_t, free, 0xb0);
OFFSET(espnow_osi_t, wifi_malloc, 0x168);
OFFSET(espnow_osi_t, wifi_realloc, 0x16c);
OFFSET(espnow_osi_t, wifi_calloc, 0x170);
OFFSET(espnow_osi_t, wifi_zalloc, 0x174);

extern const espnow_osi_t *g_osi_funcs_p;
extern uint8_t *g_chm;
extern const uint8_t g_wifi_menuconfig[];
extern const uint8_t g_ic[];
extern uint8_t *g_wifi_nvs;
extern const uint8_t ccmp[];
extern const uint8_t wpa_crypto_funcs[];
extern int ieee80211_espnow_get_init_flag(void);
extern void ieee80211_espnow_set_init_flag(int);
extern int pp_register_tx_cb(void (*handler)(void *), int, int);
extern void pp_unregister_tx_cb(int);
extern void register_ieee80211_action_vendor_spec_cb(esp_now_recv_cb_t);
extern void unregister_ieee80211_action_vendor_spec_cb(void);
extern void register_ieee80211_action_vendor_get_key_cb(espnow_key_t *(*)(const uint8_t *));
extern void unregister_ieee80211_action_vendor_get_key_cb(void);
extern int esp_wifi_internal_configure_wake_window(int, uint16_t *);
extern void ieee80211_get_tx_info_from_eb(void *, wifi_tx_info_t *);
extern int ieee80211_send_action_vendor_spec(int, const uint8_t *, const uint8_t *,
                                            size_t, int, const esp_now_rate_config_t *);
extern int ieee80211_regdomain_check_channel(uint8_t);
extern uint8_t *chm_get_current_channel(void);
extern int chm_get_band_from_chan(uint8_t);
extern int ieee80211_mt_key_is_mask(int, int);
extern void ieee80211_mt_key_set_mask(int, int);
extern void ieee80211_mt_key_clear_mask(int, int);
extern bool esp_wifi_sta_pmf_enabled(void);
extern bool wifi_ap_pmf_enabled(void);
extern void ic_set_key(int, int, int, const uint8_t *, int, const uint8_t *, int, int, int);
extern void ic_del_key(int);
extern bool wifi_init_completed(void);
extern int ieee80211_ioctl(void *);
extern void wifi_action_tx_process(void);
extern void wifi_roc_process(void);

void *espnow_alloc(size_t size)
{
    if (g_osi_funcs_p == NULL || g_osi_funcs_p->wifi_zalloc == NULL
        || g_osi_funcs_p->free == NULL) { return NULL; }
    return g_osi_funcs_p->wifi_zalloc(size);
}
void espnow_free(void *ptr)
{
    if (ptr != NULL) { g_osi_funcs_p->free(ptr); }
}
void *espnow_mutex_create(void)
{
    return g_osi_funcs_p != NULL && g_osi_funcs_p->recursive_mutex_create != NULL
        ? g_osi_funcs_p->recursive_mutex_create() : NULL;
}
bool espnow_mutex_lock(void *mutex)
{
    return g_osi_funcs_p != NULL && g_osi_funcs_p->mutex_lock != NULL
        && g_osi_funcs_p->mutex_unlock != NULL && g_osi_funcs_p->mutex_lock(mutex) == 1;
}
void espnow_mutex_unlock(void *mutex) { (void)g_osi_funcs_p->mutex_unlock(mutex); }
int espnow_initialized(void) { return ieee80211_espnow_get_init_flag(); }
void espnow_set_initialized(int value) { ieee80211_espnow_set_init_flag(value); }
int espnow_register_tx(void (*handler)(void *)) { return pp_register_tx_cb(handler, 7, 0); }
void espnow_unregister_tx(void) { pp_unregister_tx_cb(7); }
void espnow_register_rx(esp_now_recv_cb_t handler)
{ register_ieee80211_action_vendor_spec_cb(handler); }
void espnow_unregister_rx(void) { unregister_ieee80211_action_vendor_spec_cb(); }
void espnow_register_key(espnow_key_t *(*lookup)(const uint8_t *))
{ register_ieee80211_action_vendor_get_key_cb(lookup); }
void espnow_unregister_key(void) { unregister_ieee80211_action_vendor_get_key_cb(); }
int espnow_wake(int enabled, uint16_t *window)
{ return esp_wifi_internal_configure_wake_window(enabled, window); }
bool espnow_tx_info(void *buffer, wifi_tx_info_t *info)
{
    if (buffer == NULL) { return false; }
    void *descriptor;
    memcpy(&descriptor, (const uint8_t *)buffer + 44, sizeof(descriptor));
    if (descriptor == NULL) { return false; }
    ieee80211_get_tx_info_from_eb(buffer, info);
    return true;
}
int espnow_transmit(const espnow_peer_t *peer, const uint8_t *data, size_t len,
                   bool callback_requested)
{
    return ieee80211_send_action_vendor_spec(peer->interface, peer->mac, data,
                                             len, callback_requested, peer->rate);
}
int espnow_encrypted_limit(void)
{
    int32_t limit;
    memcpy(&limit, g_wifi_menuconfig + 80, sizeof(limit));
    return limit;
}
bool espnow_channel_allowed(uint8_t channel)
{ return ieee80211_regdomain_check_channel(channel) != 0; }
uint8_t espnow_home_channel(void) { return g_chm != NULL ? g_chm[80] : 0; }
uint8_t espnow_secondary_channel(void) { return g_chm != NULL ? g_chm[81] : 0; }
uint8_t espnow_current_channel(void)
{
    const uint8_t *channel = chm_get_current_channel();
    return channel != NULL ? channel[0] : 0;
}
bool espnow_interface_mode(uint8_t interface, uint8_t *mode)
{
    const uint8_t *object;
    if (interface > 1) { return false; }
    memcpy(&object, g_ic + 16 + 4U * interface, sizeof(object));
    if (object == NULL) { return false; }
    *mode = object[0x168];
    return true;
}
bool espnow_lr_enabled(uint8_t interface)
{
    return g_wifi_nvs != NULL && g_wifi_nvs[interface == 0 ? 0x475 : 0x511] != 0;
}
uint8_t espnow_bandwidth(uint8_t interface, uint8_t channel)
{
    if (g_wifi_nvs == NULL) { return 0; }
    if (chm_get_band_from_chan(channel) == 1) {
        return g_wifi_nvs[interface == 0 ? 0x9d : 0x3fb];
    }
    return g_wifi_nvs[interface == 0 ? 0x473 : 0x50a];
}
bool espnow_key_slot_used(uint8_t interface, uint8_t slot)
{ return ieee80211_mt_key_is_mask(interface, slot) != 0; }
void espnow_key_reserve(uint8_t interface, uint8_t slot)
{ ieee80211_mt_key_set_mask(interface, slot); }
void espnow_key_release(uint8_t interface, uint8_t slot)
{ ieee80211_mt_key_clear_mask(interface, slot); }
const void *espnow_key_cipher(void) { return ccmp; }
bool espnow_crypt(bool encrypt, const uint8_t *pmk, uint8_t *block)
{
    typedef int (*crypt_fn_t)(const uint8_t *, uint8_t *, uint8_t *, size_t);
    crypt_fn_t crypt;
    uint8_t iv[16] = {0};
    memcpy(&crypt, wpa_crypto_funcs + (encrypt ? 16 : 20), sizeof(crypt));
    bool success = crypt != NULL && crypt(pmk, iv, block, 16) == 0;
    espnow_clear(iv, sizeof(iv));
    return success;
}
void espnow_key_install(const espnow_peer_t *peer, espnow_key_t *key)
{
    bool pmf = peer->interface == 0 ? esp_wifi_sta_pmf_enabled() : wifi_ap_pmf_enabled();
    ic_set_key(peer->interface, 3, key->cipher_id, peer->mac, key->hardware_index,
               key->wrapped_lmk, key->length, !pmf, 0);
}
void espnow_key_delete(uint8_t slot) { ic_del_key(slot); }
bool espnow_wifi_initialized(void) { return wifi_init_completed(); }
int espnow_ioctl(espnow_ioctl_t *request) { return ieee80211_ioctl(request); }
void (*espnow_offchannel_handler(bool action))(void)
{ return action ? wifi_action_tx_process : wifi_roc_process; }

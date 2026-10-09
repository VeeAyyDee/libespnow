/* SPDX-License-Identifier: MPL-2.0 */
#include "libespnow_internal.h"
#include <string.h>

espnow_state_t g_mt;
uint32_t g_mt_version = 2;
uint8_t g_espnow_user_oui[3] = {0x18, 0xfe, 0x34};
void *g_espnow_lock;
const char *libespnow_reversion_git = "libespnow-original-0.1.0";

void espnow_clear(void *memory, size_t len)
{
    volatile uint8_t *bytes = memory;
    while (len-- != 0) {
        *bytes++ = 0;
    }
}

static bool is_multicast(const uint8_t *mac)
{
    return (mac[0] & 1U) != 0;
}

espnow_peer_t *mt_get_peer_info(const uint8_t *mac)
{
    if (mac != NULL) {
        for (espnow_peer_t *peer = g_mt.head; peer != NULL; peer = peer->next) {
            if (memcmp(mac, peer->mac, sizeof(peer->mac)) == 0) {
                return peer;
            }
        }
    }
    return NULL;
}

static espnow_key_t *lookup_key(const uint8_t *mac)
{
    espnow_peer_t *peer = mt_get_peer_info(mac);
    return peer != NULL ? peer->key : NULL;
}

static void received(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    esp_now_recv_cb_t callback = g_mt.recv_cb;
    if (callback != NULL) {
        callback(info, data, len);
    }
}

static void transmitted(void *buffer)
{
    wifi_tx_info_t info = {0};
    esp_now_send_cb_t callback = g_mt.send_cb;
    if (callback != NULL && espnow_tx_info(buffer, &info)) {
        callback(&info, (esp_now_send_status_t)info.tx_status);
    }
}

esp_err_t mt_init(void)
{
    if (espnow_initialized()) {
        return ESP_OK;
    }
    memset(&g_mt, 0, sizeof(g_mt));
    for (unsigned i = 0; i < sizeof(g_mt.pmk); ++i) {
        g_mt.pmk[i] = (uint8_t)(i * i);
    }
    g_mt_version = 2;
    if (espnow_register_tx(transmitted) != 0) {
        espnow_clear(&g_mt, sizeof(g_mt));
        return ESP_ERR_ESPNOW_INTERNAL;
    }
    espnow_register_rx(received);
    espnow_register_key(lookup_key);
    g_mt.wake_window = UINT16_MAX;
    (void)espnow_wake(1, &g_mt.wake_window);
    espnow_set_initialized(1);
    return ESP_OK;
}

static void release_key(espnow_peer_t *peer)
{
    if (peer->key != NULL) {
        espnow_key_delete(peer->key->hardware_index);
        espnow_key_release(peer->interface, peer->key->hardware_index);
        espnow_clear(peer->key, sizeof(*peer->key));
        espnow_free(peer->key);
        peer->key = NULL;
        --g_mt.encrypted;
    }
}

static void remove_peer(espnow_peer_t *peer)
{
    if (g_mt.cursor == peer) {
        g_mt.cursor = peer->next;
    }
    if (peer->previous != NULL) {
        peer->previous->next = peer->next;
    } else {
        g_mt.head = peer->next;
    }
    if (peer->next != NULL) {
        peer->next->previous = peer->previous;
    } else {
        g_mt.tail = peer->previous;
    }
    release_key(peer);
    espnow_free(peer->rate);
    espnow_clear(peer, sizeof(*peer));
    espnow_free(peer);
    --g_mt.total;
}

esp_err_t mt_deinit(void)
{
    if (!espnow_initialized()) {
        return ESP_OK;
    }
    espnow_unregister_tx();
    espnow_unregister_rx();
    espnow_unregister_key();
    while (g_mt.head != NULL) {
        remove_peer(g_mt.head);
    }
    (void)espnow_wake(0, &g_mt.wake_window);
    espnow_clear(&g_mt, sizeof(g_mt));
    espnow_set_initialized(0);
    return ESP_OK;
}

esp_err_t mt_register_recv_cb(esp_now_recv_cb_t cb)
{
    if (cb == NULL) { return ESP_ERR_ESPNOW_INTERNAL; }
    g_mt.recv_cb = cb;
    return ESP_OK;
}
esp_err_t mt_unregister_recv_cb(void)
{
    g_mt.recv_cb = NULL;
    return ESP_OK;
}
esp_err_t mt_register_send_cb(esp_now_send_cb_t cb)
{
    if (cb == NULL) { return ESP_ERR_ESPNOW_INTERNAL; }
    g_mt.send_cb = cb;
    return ESP_OK;
}
esp_err_t mt_unregister_send_cb(void)
{
    g_mt.send_cb = NULL;
    return ESP_OK;
}

static esp_err_t validate_peer(const esp_now_peer_info_t *peer,
                               const espnow_peer_t *existing)
{
    if (peer == NULL || (peer->ifidx != WIFI_IF_STA && peer->ifidx != WIFI_IF_AP)
        || (peer->channel != 0 && !espnow_channel_allowed(peer->channel))
        || (peer->encrypt && is_multicast(peer->peer_addr))) {
        return ESP_ERR_ESPNOW_ARG;
    }
    if (peer->encrypt && (existing == NULL || existing->key == NULL)
        && g_mt.encrypted >= espnow_encrypted_limit()) {
        return ESP_ERR_ESPNOW_FULL;
    }
    return ESP_OK;
}

/* No driver state changes occur until crypto, allocation and slot validation
 * have all succeeded. Updating in place preserves live driver opaque fields. */
static esp_err_t prepare_key(const esp_now_peer_info_t *input,
                             const espnow_peer_t *existing,
                             espnow_key_t **prepared, uint8_t wrapped[16])
{
    *prepared = NULL;
    memcpy(wrapped, input->lmk, 16);
    if (!espnow_crypt(true, g_mt.pmk, wrapped)) {
        espnow_clear(wrapped, 16);
        return ESP_ERR_ESPNOW_INTERNAL;
    }
    if (existing != NULL && existing->key != NULL
        && existing->interface == (uint8_t)input->ifidx) {
        *prepared = existing->key;
        return ESP_OK;
    }
    int slot;
    for (slot = 24; slot >= 0; --slot) {
        if (!espnow_key_slot_used(0, (uint8_t)slot)
            && !espnow_key_slot_used(1, (uint8_t)slot)) {
            break;
        }
    }
    if (slot < 0) { return ESP_ERR_ESPNOW_FULL; }
    espnow_key_t *key = espnow_alloc(sizeof(*key));
    if (key == NULL) { return ESP_ERR_ESPNOW_NO_MEM; }
    key->index = (uint16_t)slot;
    key->hardware_index = (uint8_t)slot;
    key->length = 16;
    key->cipher_id = 3;
    key->cipher = espnow_key_cipher();
    if (key->cipher == NULL) {
        espnow_clear(key, sizeof(*key));
        espnow_free(key);
        return ESP_ERR_ESPNOW_INTERNAL;
    }
    *prepared = key;
    return ESP_OK;
}

static void copy_peer_fields(espnow_peer_t *peer, const esp_now_peer_info_t *input)
{
    memcpy(peer->mac, input->peer_addr, sizeof(peer->mac));
    peer->channel = input->channel;
    peer->interface = (uint8_t)input->ifidx;
    peer->priv = input->priv;
}

esp_err_t mt_add_peer(const esp_now_peer_info_t *input)
{
    if (input == NULL) { return ESP_ERR_ESPNOW_ARG; }
    if (mt_get_peer_info(input->peer_addr) != NULL) { return ESP_ERR_ESPNOW_EXIST; }
    esp_err_t result = validate_peer(input, NULL);
    if (result != ESP_OK) { return result; }
    if (g_mt.total >= ESP_NOW_MAX_TOTAL_PEER_NUM) { return ESP_ERR_ESPNOW_FULL; }
    espnow_peer_t *peer = espnow_alloc(sizeof(*peer));
    if (peer == NULL) { return ESP_ERR_ESPNOW_NO_MEM; }
    copy_peer_fields(peer, input);
    if (input->encrypt) {
        uint8_t wrapped[16];
        result = prepare_key(input, NULL, &peer->key, wrapped);
        if (result == ESP_OK) {
            memcpy(peer->key->wrapped_lmk, wrapped, sizeof(wrapped));
            espnow_key_reserve(peer->interface, peer->key->hardware_index);
            espnow_key_install(peer, peer->key);
            ++g_mt.encrypted;
        }
        espnow_clear(wrapped, sizeof(wrapped));
        if (result != ESP_OK) {
            espnow_free(peer);
            return result;
        }
    }
    peer->previous = g_mt.tail;
    if (g_mt.tail != NULL) { g_mt.tail->next = peer; }
    else { g_mt.head = peer; }
    g_mt.tail = peer;
    ++g_mt.total;
    return ESP_OK;
}

esp_err_t mt_mod_peer(const esp_now_peer_info_t *input)
{
    if (input == NULL) { return ESP_ERR_ESPNOW_ARG; }
    espnow_peer_t *peer = mt_get_peer_info(input->peer_addr);
    if (peer == NULL) { return ESP_ERR_ESPNOW_NOT_FOUND; }
    esp_err_t result = validate_peer(input, peer);
    if (result != ESP_OK) { return result; }
    espnow_key_t *prepared = NULL;
    uint8_t wrapped[16] = {0};
    if (input->encrypt) {
        result = prepare_key(input, peer, &prepared, wrapped);
        if (result != ESP_OK) {
            espnow_clear(wrapped, sizeof(wrapped));
            return result;
        }
    }
    if (peer->key != prepared) {
        release_key(peer);
        peer->key = prepared;
        if (prepared != NULL) {
            espnow_key_reserve((uint8_t)input->ifidx, prepared->hardware_index);
            ++g_mt.encrypted;
        }
    }
    copy_peer_fields(peer, input);
    if (prepared != NULL) {
        memcpy(prepared->wrapped_lmk, wrapped, sizeof(wrapped));
        espnow_key_install(peer, prepared);
    }
    espnow_clear(wrapped, sizeof(wrapped));
    return ESP_OK;
}

esp_err_t mt_del_peer(const uint8_t *mac)
{
    if (mac == NULL) { return ESP_ERR_ESPNOW_ARG; }
    espnow_peer_t *peer = mt_get_peer_info(mac);
    if (peer == NULL) { return ESP_ERR_ESPNOW_NOT_FOUND; }
    remove_peer(peer);
    return ESP_OK;
}

static esp_err_t export_peer(const espnow_peer_t *peer, esp_now_peer_info_t *output)
{
    esp_now_peer_info_t value;
    memset(&value, 0, sizeof(value));
    memcpy(value.peer_addr, peer->mac, sizeof(value.peer_addr));
    value.channel = peer->channel;
    value.ifidx = (wifi_interface_t)peer->interface;
    value.priv = peer->priv;
    value.encrypt = peer->key != NULL;
    if (value.encrypt) {
        memcpy(value.lmk, peer->key->wrapped_lmk, sizeof(value.lmk));
        if (!espnow_crypt(false, g_mt.pmk, value.lmk)) {
            espnow_clear(&value, sizeof(value));
            return ESP_ERR_ESPNOW_INTERNAL;
        }
    }
    memcpy(output, &value, sizeof(value));
    espnow_clear(&value, sizeof(value));
    return ESP_OK;
}

esp_err_t mt_get_peer(const uint8_t *mac, esp_now_peer_info_t *output)
{
    if (mac == NULL || output == NULL) { return ESP_ERR_ESPNOW_ARG; }
    espnow_peer_t *peer = mt_get_peer_info(mac);
    return peer != NULL ? export_peer(peer, output) : ESP_ERR_ESPNOW_NOT_FOUND;
}

esp_err_t mt_fetch_peer(bool from_head, esp_now_peer_info_t *output)
{
    if (output == NULL) { return ESP_ERR_ESPNOW_ARG; }
    espnow_peer_t *peer = from_head ? g_mt.head : g_mt.cursor;
    while (peer != NULL && is_multicast(peer->mac)) { peer = peer->next; }
    g_mt.cursor = peer;
    if (peer == NULL) { return ESP_ERR_ESPNOW_NOT_FOUND; }
    esp_err_t result = export_peer(peer, output);
    if (result == ESP_OK) { g_mt.cursor = peer->next; }
    return result;
}

bool mt_is_peer_exist(const uint8_t *mac) { return mt_get_peer_info(mac) != NULL; }
esp_err_t mt_get_peer_num(esp_now_peer_num_t *num)
{
    if (num == NULL) { return ESP_ERR_ESPNOW_ARG; }
    num->total_num = g_mt.total;
    num->encrypt_num = g_mt.encrypted;
    return ESP_OK;
}
esp_err_t mt_get_version(uint32_t *version)
{
    if (version == NULL) { return ESP_ERR_ESPNOW_ARG; }
    *version = g_mt_version;
    return ESP_OK;
}
esp_err_t mt_set_pmk(const uint8_t *pmk)
{
    if (pmk == NULL || g_mt.encrypted != 0) { return ESP_ERR_ESPNOW_ARG; }
    memmove(g_mt.pmk, pmk, sizeof(g_mt.pmk));
    return ESP_OK;
}
esp_err_t mt_set_wake_window(uint16_t window)
{
    if (window != g_mt.wake_window) {
        (void)espnow_wake(0, &g_mt.wake_window);
        g_mt.wake_window = window;
        if (window != 0) { (void)espnow_wake(1, &g_mt.wake_window); }
    }
    return ESP_OK;
}
esp_err_t mt_set_user_oui(uint8_t *oui)
{
    static const uint8_t default_oui[3] = {0x18, 0xfe, 0x34};
    memmove(g_espnow_user_oui, oui != NULL ? oui : default_oui, 3);
    return ESP_OK;
}
esp_err_t mt_get_user_oui(uint8_t *oui)
{
    if (oui == NULL) { return ESP_ERR_ESPNOW_ARG; }
    memmove(oui, g_espnow_user_oui, 3);
    return ESP_OK;
}

static esp_err_t send_peer(const espnow_peer_t *peer, const uint8_t *data, size_t len)
{
    if (peer->channel != 0 && peer->channel != espnow_home_channel()) {
        return ESP_ERR_ESPNOW_CHAN;
    }
    int result = espnow_transmit(peer, data, len, g_mt.send_cb != NULL);
    switch (result) {
    case 0x101: return ESP_ERR_ESPNOW_NO_MEM;
    case 0x3004: return ESP_ERR_ESPNOW_IF;
    case -1: return ESP_ERR_ESPNOW_INTERNAL;
    default: return result;
    }
}

esp_err_t mt_send(const uint8_t *mac, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0 || len > 1470) { return ESP_ERR_ESPNOW_ARG; }
    if (mac != NULL) {
        espnow_peer_t *peer = mt_get_peer_info(mac);
        return peer != NULL ? send_peer(peer, data, len) : ESP_ERR_ESPNOW_NOT_FOUND;
    }
    bool sent = false;
    for (espnow_peer_t *peer = g_mt.head; peer != NULL; peer = peer->next) {
        if (!is_multicast(peer->mac)) {
            sent = true;
            if (send_peer(peer, data, len) != ESP_OK) { return ESP_ERR_ESPNOW_INTERNAL; }
        }
    }
    return sent ? ESP_OK : ESP_ERR_ESPNOW_NOT_FOUND;
}

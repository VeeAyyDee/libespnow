/* SPDX-License-Identifier: MPL-2.0
 * Private, pinned ESP32-S3 ESP-IDF 6.0.3 compatibility declarations. */
#ifndef LIBESPNOW_INTERNAL_H
#define LIBESPNOW_INTERNAL_H
#include <stddef.h>
#include <stdint.h>
#include "esp_now.h"

typedef struct espnow_key {
    uint16_t index;
    uint8_t opaque[158];
    const void *cipher;
    uint32_t opaque_word;
    uint8_t wrapped_lmk[16];
    uint8_t length;
    uint8_t cipher_id;
    uint8_t hardware_index;
    uint8_t reserved[5];
} espnow_key_t;

typedef struct espnow_peer {
    struct espnow_peer *next;
    struct espnow_peer *previous;
    uint8_t channel;
    uint8_t interface;
    uint8_t mac[6];
    void *priv;
    espnow_key_t *key;
    esp_now_rate_config_t *rate;
} espnow_peer_t;

typedef struct {
    uint8_t pmk[16];
    uint8_t reserved[12];
    espnow_peer_t *head;
    espnow_peer_t *tail;
    int32_t total;
    int32_t encrypted;
    espnow_peer_t *cursor;
    esp_now_recv_cb_t recv_cb;
    esp_now_send_cb_t send_cb;
    uint16_t wake_window;
} espnow_state_t;

extern espnow_state_t g_mt;
extern uint32_t g_mt_version;
extern uint8_t g_espnow_user_oui[3];
extern void *g_espnow_lock;
extern const char *libespnow_reversion_git;

esp_err_t mt_init(void);
esp_err_t mt_deinit(void);
esp_err_t mt_register_recv_cb(esp_now_recv_cb_t cb);
esp_err_t mt_unregister_recv_cb(void);
esp_err_t mt_register_send_cb(esp_now_send_cb_t cb);
esp_err_t mt_unregister_send_cb(void);
esp_err_t mt_send(const uint8_t *mac, const uint8_t *data, size_t len);
esp_err_t mt_add_peer(const esp_now_peer_info_t *peer);
esp_err_t mt_mod_peer(const esp_now_peer_info_t *peer);
esp_err_t mt_del_peer(const uint8_t *mac);
esp_err_t mt_get_peer(const uint8_t *mac, esp_now_peer_info_t *peer);
esp_err_t mt_fetch_peer(bool from_head, esp_now_peer_info_t *peer);
bool mt_is_peer_exist(const uint8_t *mac);
espnow_peer_t *mt_get_peer_info(const uint8_t *mac);
esp_err_t mt_get_peer_num(esp_now_peer_num_t *num);
esp_err_t mt_get_version(uint32_t *version);
esp_err_t mt_set_pmk(const uint8_t *pmk);
esp_err_t mt_set_wake_window(uint16_t window);
esp_err_t mt_set_user_oui(uint8_t *oui);
esp_err_t mt_get_user_oui(uint8_t *oui);
esp_err_t mt_set_peer_rate_config(const esp_now_peer_info_t *peer,
                                  const esp_now_rate_config_t *config);

/* Portable core / platform boundary; not an application-facing replacement API. */
void *espnow_alloc(size_t size);
void espnow_free(void *ptr);
void *espnow_mutex_create(void);
bool espnow_mutex_lock(void *mutex);
void espnow_mutex_unlock(void *mutex);
int espnow_initialized(void);
void espnow_set_initialized(int initialized);
int espnow_register_tx(void (*handler)(void *));
void espnow_unregister_tx(void);
void espnow_register_rx(esp_now_recv_cb_t handler);
void espnow_unregister_rx(void);
void espnow_register_key(espnow_key_t *(*lookup)(const uint8_t *));
void espnow_unregister_key(void);
int espnow_wake(int enabled, uint16_t *window);
bool espnow_tx_info(void *buffer, wifi_tx_info_t *info);
int espnow_transmit(const espnow_peer_t *peer, const uint8_t *data, size_t len,
                   bool callback_requested);
int espnow_encrypted_limit(void);
bool espnow_channel_allowed(uint8_t channel);
uint8_t espnow_home_channel(void);
uint8_t espnow_current_channel(void);
uint8_t espnow_secondary_channel(void);
bool espnow_interface_mode(uint8_t interface, uint8_t *mode);
bool espnow_lr_enabled(uint8_t interface);
uint8_t espnow_bandwidth(uint8_t interface, uint8_t channel);
bool espnow_key_slot_used(uint8_t interface, uint8_t slot);
void espnow_key_reserve(uint8_t interface, uint8_t slot);
void espnow_key_release(uint8_t interface, uint8_t slot);
const void *espnow_key_cipher(void);
bool espnow_crypt(bool encrypt, const uint8_t *pmk, uint8_t *block);
void espnow_key_install(const espnow_peer_t *peer, espnow_key_t *key);
void espnow_key_delete(uint8_t slot);
bool espnow_wifi_initialized(void);

/* Natural host layout is deliberately distinct from the 24-byte target ABI. */
typedef struct {
    uint8_t opcode;
    uint8_t reserved0[3];
    void (*handler)(void);
    uint16_t synchronous;
    uint8_t reserved1[2];
    uint32_t reserved2;
    uint32_t reserved3;
    void *payload;
} espnow_ioctl_t;
int espnow_ioctl(espnow_ioctl_t *request);
void (*espnow_offchannel_handler(bool action))(void);
void espnow_clear(void *memory, size_t len);
#endif

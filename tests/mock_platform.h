/* SPDX-License-Identifier: MPL-2.0 */
#ifndef MOCK_PLATFORM_H
#define MOCK_PLATFORM_H
#include "libespnow_internal.h"
typedef struct {
    int initialized;
    bool wifi_initialized;
    int allocation_fail_after;
    int allocations;
    int frees;
    int live_allocations;
    bool fail_mutex;
    bool fail_lock;
    int locks;
    int unlocks;
    int register_result;
    int register_calls;
    int unregister_calls;
    void (*tx_hook)(void *);
    esp_now_recv_cb_t rx_hook;
    espnow_key_t *(*key_hook)(const uint8_t *);
    int wake_calls;
    int wake_enabled[64];
    uint16_t wake_values[64];
    int wake_result;
    int transmit_result;
    int transmissions;
    bool callback_requested;
    uint8_t sent_data[1470];
    size_t sent_len;
    uint8_t sent_mac[6];
    const esp_now_rate_config_t *sent_rate;
    int encrypted_limit;
    uint8_t home_channel;
    uint8_t current_channel;
    uint8_t secondary_channel;
    bool interface_present[2];
    uint8_t mode[2];
    bool lr[2];
    uint8_t bandwidth[2];
    bool slots[2][25];
    bool fail_crypto;
    bool missing_crypto;
    bool missing_cipher;
    int installs;
    int deletes;
    uint8_t last_installed_slot;
    uint8_t last_installed_interface;
    uint8_t last_wrapped[16];
    int ioctl_result;
    int ioctl_calls;
    uint8_t ioctl_op_id;
    uint8_t ioctl_opcode;
    uint8_t ioctl_payload[70000];
    size_t ioctl_payload_size;
} mock_t;
extern mock_t mock;
void mock_reset(void);
#endif

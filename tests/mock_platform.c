/* SPDX-License-Identifier: MPL-2.0 */
#include "mock_platform.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
mock_t mock;
static int mutex_token;
static int cipher_token;
/* Keep allocation metadata separate so freeing a key can independently assert
 * compiler-resistant zeroing, without inspecting freed storage. */
static struct { void *pointer; size_t size; } allocated[128];
void mock_reset(void)
{
    assert(mock.live_allocations == 0);
    memset(&mock, 0, sizeof(mock));
    mock.allocation_fail_after = -1;
    mock.encrypted_limit = 17;
    mock.home_channel = 6;
    mock.current_channel = 6;
    mock.interface_present[0] = mock.interface_present[1] = true;
    mock.mode[0] = mock.mode[1] = 6;
    mock.bandwidth[0] = mock.bandwidth[1] = 2;
    mock.ioctl_op_id = 73;
}
void *espnow_alloc(size_t size)
{
    if (mock.allocation_fail_after == 0) { return NULL; }
    if (mock.allocation_fail_after > 0) { --mock.allocation_fail_after; }
    void *result = calloc(1, size);
    assert(result != NULL);
    bool recorded = false;
    for (unsigned i = 0; i < 128; ++i) {
        if (allocated[i].pointer == NULL) {
            allocated[i].pointer = result;
            allocated[i].size = size;
            recorded = true;
            break;
        }
    }
    assert(recorded);
    ++mock.allocations;
    ++mock.live_allocations;
    return result;
}
void espnow_free(void *ptr)
{
    if (ptr != NULL) {
        bool found = false;
        for (unsigned i = 0; i < 128; ++i) {
            if (allocated[i].pointer == ptr) {
                if (allocated[i].size == sizeof(espnow_key_t)) {
                    const uint8_t *bytes = ptr;
                    for (size_t j = 0; j < allocated[i].size; ++j) { assert(bytes[j] == 0); }
                }
                allocated[i].pointer = NULL;
                found = true;
                break;
            }
        }
        assert(found);
        ++mock.frees;
        --mock.live_allocations;
        assert(mock.live_allocations >= 0);
        free(ptr);
    }
}
void *espnow_mutex_create(void) { return mock.fail_mutex ? NULL : &mutex_token; }
bool espnow_mutex_lock(void *mutex)
{
    assert(mutex == &mutex_token);
    if (mock.fail_lock) { return false; }
    ++mock.locks;
    return true;
}
void espnow_mutex_unlock(void *mutex)
{ assert(mutex == &mutex_token); ++mock.unlocks; }
int espnow_initialized(void) { return mock.initialized; }
void espnow_set_initialized(int value) { mock.initialized = value; }
int espnow_register_tx(void (*handler)(void *))
{
    ++mock.register_calls;
    if (mock.register_result == 0) { mock.tx_hook = handler; }
    return mock.register_result;
}
void espnow_unregister_tx(void) { mock.tx_hook = NULL; ++mock.unregister_calls; }
void espnow_register_rx(esp_now_recv_cb_t handler) { mock.rx_hook = handler; }
void espnow_unregister_rx(void) { mock.rx_hook = NULL; }
void espnow_register_key(espnow_key_t *(*lookup)(const uint8_t *)) { mock.key_hook = lookup; }
void espnow_unregister_key(void) { mock.key_hook = NULL; }
int espnow_wake(int enabled, uint16_t *window)
{
    assert(mock.wake_calls < 64);
    mock.wake_enabled[mock.wake_calls] = enabled;
    mock.wake_values[mock.wake_calls] = *window;
    ++mock.wake_calls;
    return mock.wake_result;
}
bool espnow_tx_info(void *buffer, wifi_tx_info_t *info)
{
    if (buffer == NULL) { return false; }
    const wifi_tx_info_t *source;
    memcpy(&source, (uint8_t *)buffer + 44, sizeof(source));
    if (source == NULL) { return false; }
    *info = *source;
    return true;
}
int espnow_transmit(const espnow_peer_t *peer, const uint8_t *data, size_t len,
                   bool callback_requested)
{
    assert(len <= sizeof(mock.sent_data));
    ++mock.transmissions;
    mock.callback_requested = callback_requested;
    memcpy(mock.sent_data, data, len);
    mock.sent_len = len;
    memcpy(mock.sent_mac, peer->mac, 6);
    mock.sent_rate = peer->rate;
    return mock.transmit_result;
}
int espnow_encrypted_limit(void) { return mock.encrypted_limit; }
bool espnow_channel_allowed(uint8_t channel) { return channel <= 14 || channel == 36; }
uint8_t espnow_home_channel(void) { return mock.home_channel; }
uint8_t espnow_current_channel(void) { return mock.current_channel; }
uint8_t espnow_secondary_channel(void) { return mock.secondary_channel; }
bool espnow_interface_mode(uint8_t interface, uint8_t *mode)
{
    assert(interface < 2);
    *mode = mock.mode[interface];
    return mock.interface_present[interface];
}
bool espnow_lr_enabled(uint8_t interface) { assert(interface < 2); return mock.lr[interface]; }
uint8_t espnow_bandwidth(uint8_t interface, uint8_t channel)
{ (void)channel; assert(interface < 2); return mock.bandwidth[interface]; }
bool espnow_key_slot_used(uint8_t interface, uint8_t slot)
{ assert(interface < 2 && slot < 25); return mock.slots[interface][slot]; }
void espnow_key_reserve(uint8_t interface, uint8_t slot)
{ assert(!mock.slots[0][slot] && !mock.slots[1][slot]); mock.slots[interface][slot] = true; }
void espnow_key_release(uint8_t interface, uint8_t slot)
{ assert(mock.slots[interface][slot]); mock.slots[interface][slot] = false; }
const void *espnow_key_cipher(void) { return mock.missing_cipher ? NULL : &cipher_token; }
bool espnow_crypt(bool encrypt, const uint8_t *pmk, uint8_t *block)
{
    (void)encrypt;
    /* Reversible test double, explicitly NOT AES or radio validation. */
    if (mock.missing_crypto) { return false; }
    for (unsigned i = 0; i < 16; ++i) { block[i] ^= (uint8_t)(pmk[i] ^ 0xa5U); }
    return !mock.fail_crypto;
}
void espnow_key_install(const espnow_peer_t *peer, espnow_key_t *key)
{
    assert(key->length == 16 && key->cipher_id == 3 && key->index == key->hardware_index);
    assert(mock.slots[peer->interface][key->hardware_index]);
    ++mock.installs;
    mock.last_installed_slot = key->hardware_index;
    mock.last_installed_interface = peer->interface;
    memcpy(mock.last_wrapped, key->wrapped_lmk, 16);
}
void espnow_key_delete(uint8_t slot) { assert(slot < 25); ++mock.deletes; }
bool espnow_wifi_initialized(void) { return mock.wifi_initialized; }
static uint32_t read_u32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
int espnow_ioctl(espnow_ioctl_t *request)
{
    assert(request->synchronous == 1);
    assert(request->reserved0[0] == 0 && request->reserved0[1] == 0 && request->reserved0[2] == 0);
    assert(request->reserved1[0] == 0 && request->reserved1[1] == 0);
    assert(request->reserved2 == 0 && request->reserved3 == 0);
    assert(request->opcode == 0x55 || request->opcode == 0x56);
    assert(request->handler == espnow_offchannel_handler(request->opcode == 0x55));
    ++mock.ioctl_calls;
    mock.ioctl_opcode = request->opcode;
    uint8_t *payload = request->payload;
    mock.ioctl_payload_size = request->opcode == 0x55 ? 48U + read_u32(payload + 44) : 32;
    assert(mock.ioctl_payload_size <= sizeof(mock.ioctl_payload));
    memcpy(mock.ioctl_payload, payload, mock.ioctl_payload_size);
    payload[request->opcode == 0x55 ? 36 : 24] = mock.ioctl_op_id;
    espnow_free(request);
    return mock.ioctl_result;
}
static void action_handler(void) {}
static void roc_handler(void) {}
void (*espnow_offchannel_handler(bool action))(void)
{ return action ? action_handler : roc_handler; }

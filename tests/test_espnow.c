/* SPDX-License-Identifier: MPL-2.0 */
#include "mock_platform.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_peer_model(void);

static void start(void)
{
    assert(esp_now_deinit() == ESP_OK);
    assert(mock.locks == mock.unlocks);
    mock_reset();
    assert(esp_now_init() == ESP_OK);
}
static esp_now_peer_info_t peer(unsigned id, bool encrypt)
{
    esp_now_peer_info_t value;
    memset(&value, 0, sizeof(value));
    value.peer_addr[0] = 2;
    value.peer_addr[4] = (uint8_t)(id >> 8);
    value.peer_addr[5] = (uint8_t)id;
    value.ifidx = WIFI_IF_STA;
    value.encrypt = encrypt;
    for (unsigned i = 0; i < 16; ++i) { value.lmk[i] = (uint8_t)(id + i); }
    return value;
}
static void lifecycle(void)
{
    mock_reset();
    mock.fail_mutex = true;
    assert(esp_now_init() == ESP_ERR_ESPNOW_NO_MEM);
    assert(esp_now_get_version(NULL) == ESP_ERR_ESPNOW_NO_MEM);
    assert(!esp_now_is_peer_exist(NULL));
    mock.fail_mutex = false;
    mock.fail_lock = true;
    assert(esp_now_init() == ESP_ERR_ESPNOW_INTERNAL);
    mock.fail_lock = false;
    uint32_t version = 0;
    assert(esp_now_get_version(&version) == ESP_OK && version == 2);
    assert(esp_now_get_version(NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_get_user_oui(NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_add_peer(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_send(NULL, NULL, 0) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_mod_peer(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_del_peer(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_get_peer(NULL, NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_fetch_peer(true, NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_get_peer_num(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_register_recv_cb(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_unregister_recv_cb() == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_register_send_cb(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_unregister_send_cb() == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_set_pmk(NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_set_wake_window(0) == ESP_ERR_ESPNOW_NOT_INIT);
    assert(esp_now_set_peer_rate_config(NULL, NULL) == ESP_ERR_ESPNOW_NOT_INIT);
    mock.register_result = -3;
    assert(esp_now_init() == ESP_ERR_ESPNOW_INTERNAL);
    assert(mock.initialized == 0 && mock.rx_hook == NULL && mock.key_hook == NULL);
    mock.register_result = 0;
    assert(esp_now_init() == ESP_OK);
    assert(esp_now_init() == ESP_OK && mock.register_calls == 2);
    assert(mock.initialized == 1 && mock.rx_hook != NULL && mock.key_hook != NULL);
    for (unsigned i = 0; i < 16; ++i) { assert(g_mt.pmk[i] == (uint8_t)(i * i)); }
    assert(mock.wake_calls == 1 && mock.wake_enabled[0] == 1 && mock.wake_values[0] == 65535);
    assert(esp_now_register_recv_cb(NULL) == ESP_ERR_ESPNOW_INTERNAL);
    assert(esp_now_register_send_cb(NULL) == ESP_ERR_ESPNOW_INTERNAL);
    assert(esp_now_deinit() == ESP_OK);
    assert(esp_now_deinit() == ESP_OK && mock.unregister_calls == 1);
    assert(mock.rx_hook == NULL && mock.tx_hook == NULL && mock.key_hook == NULL);
    assert(g_mt.head == NULL && g_mt.tail == NULL && g_mt.cursor == NULL && g_mt.total == 0);
    assert(mock.live_allocations == 0);
}
static void plain_peers(void)
{
    start();
    assert(esp_now_add_peer(NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_mod_peer(NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_get_peer(NULL, NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_del_peer(NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_fetch_peer(true, NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_get_peer_num(NULL) == ESP_ERR_ESPNOW_ARG);
    esp_now_peer_info_t a = peer(1, false), output;
    a.priv = &a;
    a.ifidx = WIFI_IF_MAX;
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_ARG);
    a.ifidx = WIFI_IF_STA;
    a.channel = 99;
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_ARG);
    a.channel = 0;
    mock.allocation_fail_after = 0;
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_NO_MEM);
    mock.allocation_fail_after = -1;
    assert(esp_now_add_peer(&a) == ESP_OK);
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_EXIST);
    memset(&output, 0xff, sizeof(output));
    assert(esp_now_get_peer(a.peer_addr, &output) == ESP_OK);
    assert(output.priv == &a && output.ifidx == WIFI_IF_STA && !output.encrypt);
    for (unsigned i = 0; i < 16; ++i) { assert(output.lmk[i] == 0); }
    a.channel = 6; a.ifidx = WIFI_IF_AP;
    assert(esp_now_mod_peer(&a) == ESP_OK);
    assert(esp_now_get_peer(a.peer_addr, &output) == ESP_OK && output.channel == 6 && output.ifidx == WIFI_IF_AP);
    for (unsigned i = 2; i <= 20; ++i) {
        esp_now_peer_info_t value = peer(i, false);
        assert(esp_now_add_peer(&value) == ESP_OK);
    }
    esp_now_peer_info_t extra = peer(21, false);
    assert(esp_now_add_peer(&extra) == ESP_ERR_ESPNOW_FULL);
    esp_now_peer_num_t counts;
    assert(esp_now_get_peer_num(&counts) == ESP_OK && counts.total_num == 20 && counts.encrypt_num == 0);
    assert(esp_now_get_peer(extra.peer_addr, &output) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(esp_now_del_peer(extra.peer_addr) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(esp_now_mod_peer(&extra) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(esp_now_is_peer_exist(a.peer_addr));
    assert(!esp_now_is_peer_exist(extra.peer_addr) && !esp_now_is_peer_exist(NULL));
    assert(esp_now_deinit() == ESP_OK && mock.live_allocations == 0);
}
static void fetch_and_broadcast(void)
{
    start();
    esp_now_peer_info_t a = peer(1, false), b = peer(2, false), c = peer(3, false), out;
    b.peer_addr[0] = 0xff;
    assert(esp_now_add_peer(&a) == ESP_OK);
    assert(esp_now_add_peer(&b) == ESP_OK);
    assert(esp_now_add_peer(&c) == ESP_OK);
    assert(esp_now_fetch_peer(true, &out) == ESP_OK && out.peer_addr[5] == 1);
    assert(esp_now_del_peer(b.peer_addr) == ESP_OK); /* deletes cursor target */
    assert(esp_now_fetch_peer(false, &out) == ESP_OK && out.peer_addr[5] == 3);
    assert(esp_now_fetch_peer(false, &out) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(esp_now_fetch_peer(true, &out) == ESP_OK && out.peer_addr[5] == 1);
    assert(esp_now_del_peer(c.peer_addr) == ESP_OK);
    assert(esp_now_fetch_peer(false, &out) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(esp_now_add_peer(&b) == ESP_OK);
    uint8_t data = 8;
    assert(esp_now_send(NULL, &data, 1) == ESP_OK && mock.transmissions == 1);
    assert(esp_now_send(b.peer_addr, &data, 1) == ESP_OK && mock.transmissions == 2);
    assert(esp_now_del_peer(a.peer_addr) == ESP_OK);
    assert(esp_now_send(NULL, &data, 1) == ESP_ERR_ESPNOW_NOT_FOUND);
    b.encrypt = true;
    assert(esp_now_mod_peer(&b) == ESP_ERR_ESPNOW_ARG);
}
static void encrypted_peers(void)
{
    start();
    esp_now_peer_info_t a = peer(1, true), b = peer(2, true), out;
    mock.slots[1][24] = true;
    assert(esp_now_add_peer(&a) == ESP_OK);
    assert(mock.last_installed_slot == 23 && mock.slots[0][23]);
    assert(mock.key_hook(a.peer_addr) == mt_get_peer_info(a.peer_addr)->key);
    assert(mock.key_hook(b.peer_addr) == NULL);
    assert(esp_now_get_peer(a.peer_addr, &out) == ESP_OK && out.encrypt && memcmp(a.lmk, out.lmk, 16) == 0);
    assert(esp_now_set_pmk(a.lmk) == ESP_ERR_ESPNOW_ARG);
    espnow_key_t *old = mt_get_peer_info(a.peer_addr)->key;
    old->opaque[17] = 0xab; old->opaque_word = 0x12345678;
    a.lmk[3] ^= 0x77;
    assert(esp_now_mod_peer(&a) == ESP_OK && mt_get_peer_info(a.peer_addr)->key == old);
    assert(old->opaque[17] == 0xab && old->opaque_word == 0x12345678);
    assert(esp_now_get_peer(a.peer_addr, &out) == ESP_OK && memcmp(a.lmk, out.lmk, 16) == 0);
    mock.fail_crypto = true;
    a.channel = 6; a.ifidx = WIFI_IF_AP;
    assert(esp_now_mod_peer(&a) == ESP_ERR_ESPNOW_INTERNAL);
    assert(mt_get_peer_info(a.peer_addr)->channel == 0 && mt_get_peer_info(a.peer_addr)->interface == 0);
    memset(&out, 0x55, sizeof(out));
    assert(esp_now_get_peer(a.peer_addr, &out) == ESP_ERR_ESPNOW_INTERNAL && out.peer_addr[0] == 0x55);
    mock.fail_crypto = false;
    mock.allocation_fail_after = 0;
    assert(esp_now_mod_peer(&a) == ESP_ERR_ESPNOW_NO_MEM && mock.slots[0][23]);
    mock.allocation_fail_after = -1;
    assert(esp_now_mod_peer(&a) == ESP_OK && !mock.slots[0][23] && mock.slots[1][22]);
    assert(mock.last_installed_interface == 1);
    mock.encrypted_limit = 1;
    assert(esp_now_add_peer(&b) == ESP_ERR_ESPNOW_FULL);
    b.encrypt = false;
    assert(esp_now_add_peer(&b) == ESP_OK);
    b.encrypt = true;
    assert(esp_now_mod_peer(&b) == ESP_ERR_ESPNOW_FULL);
    a.encrypt = false;
    assert(esp_now_mod_peer(&a) == ESP_OK && !mock.slots[1][22]);
    assert(esp_now_set_pmk(a.lmk) == ESP_OK);
    assert(esp_now_set_pmk(NULL) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_get_peer(a.peer_addr, &out) == ESP_OK && !out.encrypt && out.lmk[3] == 0);
    for (unsigned i = 0; i < 25; ++i) { mock.slots[0][i] = true; }
    assert(esp_now_mod_peer(&b) == ESP_ERR_ESPNOW_FULL && !mt_get_peer_info(b.peer_addr)->key);
    memset(mock.slots, 0, sizeof(mock.slots));
    assert(esp_now_mod_peer(&b) == ESP_OK);
    assert(esp_now_del_peer(b.peer_addr) == ESP_OK && !mock.slots[0][24]);
}
static void failures(void)
{
    for (int fail = 0; fail <= 1; ++fail) {
        start();
        esp_now_peer_info_t a = peer(1, true);
        mock.allocation_fail_after = fail;
        assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_NO_MEM);
        assert(mock.live_allocations == 0 && g_mt.total == 0 && g_mt.encrypted == 0);
        assert(!mock.slots[0][24]);
    }
    start();
    esp_now_peer_info_t a = peer(1, true);
    mock.fail_crypto = true;
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_INTERNAL && mock.live_allocations == 0);
    mock.fail_crypto = false; mock.missing_crypto = true;
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_INTERNAL && mock.live_allocations == 0);
    mock.missing_crypto = false; mock.missing_cipher = true;
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_INTERNAL && mock.live_allocations == 0);
    mock.missing_cipher = false;
    for (unsigned i = 0; i < 17; ++i) {
        a = peer(i, true);
        assert(esp_now_add_peer(&a) == ESP_OK);
    }
    a = peer(17, true);
    assert(esp_now_add_peer(&a) == ESP_ERR_ESPNOW_FULL);
}
static int sends, receives;
static void on_send(const esp_now_send_info_t *info, esp_now_send_status_t status)
{ assert(info != NULL && status == ESP_NOW_SEND_FAIL && info->tx_status == WIFI_SEND_FAIL); ++sends; }
static void on_receive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{ assert(info != NULL && data[0] == 0x42 && len == 1); ++receives; }
static void sending(void)
{
    start();
    esp_now_peer_info_t a = peer(1, false);
    uint8_t data[1471]; memset(data, 0x42, sizeof(data));
    assert(esp_now_send(a.peer_addr, data, 1) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(esp_now_add_peer(&a) == ESP_OK);
    assert(esp_now_send(a.peer_addr, NULL, 1) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_send(a.peer_addr, data, 0) == ESP_ERR_ESPNOW_ARG);
    assert(esp_now_send(a.peer_addr, data, 1471) == ESP_ERR_ESPNOW_ARG);
    const size_t lengths[] = {1, 250, 251, 1470};
    for (size_t i = 0; i < sizeof(lengths)/sizeof(lengths[0]); ++i) {
        assert(esp_now_send(a.peer_addr, data, lengths[i]) == ESP_OK);
        assert(mock.sent_len == lengths[i] && !mock.callback_requested);
    }
    data[0] = 1; assert(mock.sent_data[0] == 0x42);
    assert(esp_now_register_send_cb(on_send) == ESP_OK);
    assert(esp_now_register_recv_cb(on_receive) == ESP_OK);
    assert(esp_now_send(a.peer_addr, data, 1) == ESP_OK && mock.callback_requested && sends == 0 && receives == 0);
    uint8_t buffer[44 + sizeof(void *)] = {0};
    mock.tx_hook(NULL); mock.tx_hook(buffer); assert(sends == 0);
    wifi_tx_info_t info = {0}; info.tx_status = WIFI_SEND_FAIL;
    wifi_tx_info_t *pointer = &info;
    memcpy(buffer + 44, &pointer, sizeof(pointer));
    mock.tx_hook(buffer); assert(sends == 1 && receives == 0);
    esp_now_recv_info_t rx = {0}; data[0] = 0x42;
    mock.rx_hook(&rx, data, 1); assert(receives == 1);
    assert(esp_now_unregister_send_cb() == ESP_OK && esp_now_unregister_recv_cb() == ESP_OK);
    mock.tx_hook(buffer); mock.rx_hook(&rx, data, 1); assert(sends == 1 && receives == 1);
    esp_now_peer_info_t later = peer(2, false);
    assert(esp_now_add_peer(&later) == ESP_OK);
    const int driver[] = {0x101, 0x3004, -1, 456};
    const int expected[] = {ESP_ERR_ESPNOW_NO_MEM, ESP_ERR_ESPNOW_IF, ESP_ERR_ESPNOW_INTERNAL, 456};
    for (unsigned i = 0; i < 4; ++i) {
        mock.transmit_result = driver[i];
        assert(esp_now_send(a.peer_addr, data, 1) == expected[i]);
        int submitted_before = mock.transmissions;
        assert(esp_now_send(NULL, data, 1) == ESP_ERR_ESPNOW_INTERNAL);
        assert(mock.transmissions == submitted_before + 1);
    }
    mock.transmit_result = 0;
    a.channel = 7;
    assert(esp_now_mod_peer(&a) == ESP_OK);
    assert(esp_now_send(a.peer_addr, data, 1) == ESP_ERR_ESPNOW_CHAN);
    assert(esp_now_send(NULL, data, 1) == ESP_ERR_ESPNOW_INTERNAL);
}
static void oui_wake(void)
{
    start();
    uint8_t oui[3] = {4, 5, 6}, out[3];
    assert(esp_now_set_user_oui(oui) == ESP_OK);
    assert(esp_now_deinit() == ESP_OK && esp_now_init() == ESP_OK);
    assert(esp_now_get_user_oui(out) == ESP_OK && memcmp(out, oui, 3) == 0);
    assert(esp_now_set_user_oui(g_espnow_user_oui) == ESP_OK);
    assert(esp_now_get_user_oui(g_espnow_user_oui) == ESP_OK);
    assert(esp_now_set_user_oui(NULL) == ESP_OK);
    assert(esp_now_get_user_oui(out) == ESP_OK && out[0] == 0x18 && out[1] == 0xfe && out[2] == 0x34);
    int before = mock.wake_calls;
    assert(esp_now_set_wake_window(65535) == ESP_OK && mock.wake_calls == before);
    assert(esp_now_set_wake_window(23) == ESP_OK && mock.wake_calls == before + 2);
    assert(mock.wake_enabled[before] == 0 && mock.wake_values[before] == 65535);
    assert(mock.wake_enabled[before+1] == 1 && mock.wake_values[before+1] == 23);
    assert(esp_now_set_wake_window(0) == ESP_OK && mock.wake_calls == before + 3);
    mock.wake_result = -1;
    assert(esp_now_set_wake_window(55) == ESP_OK && g_mt.wake_window == 55);
}
static esp_err_t rate_set(const uint8_t *mac, int mode, int rate, bool ersu, bool dcm)
{
    esp_now_rate_config_t config = {(wifi_phy_mode_t)mode, (wifi_phy_rate_t)rate, ersu, dcm};
    return esp_now_set_peer_rate_config(mac, &config);
}
static void rates(void)
{
    start();
    esp_now_peer_info_t a = peer(1, false);
    assert(esp_now_add_peer(&a) == ESP_OK);
    assert(esp_now_set_peer_rate_config(a.peer_addr, NULL) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(NULL, 1, 0, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, -1, 0, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 7, 0, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 1, 43, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 1, -1, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 0, 41, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.lr[1] = true;
    assert(rate_set(a.peer_addr, 0, 41, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.lr[0] = true;
    assert(rate_set(a.peer_addr, 0, 41, false, false) == ESP_OK);
    assert(rate_set(a.peer_addr, 0, 42, false, false) == ESP_OK);
    assert(rate_set(a.peer_addr, 0, 40, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.interface_present[0] = false;
    assert(rate_set(a.peer_addr, 1, 0, false, false) == ESP_ERR_ESPNOW_INTERNAL);
    mock.interface_present[0] = true; mock.mode[0] = 2;
    assert(rate_set(a.peer_addr, 4, 16, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.mode[0] = 6;
    assert(rate_set(a.peer_addr, 3, 8, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 1, 16, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 4, 15, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 1, 15, false, false) == ESP_OK);
    assert(rate_set(a.peer_addr, 4, 16, false, false) == ESP_OK);
    esp_now_rate_config_t *saved = mt_get_peer_info(a.peer_addr)->rate;
    const uint8_t data = 1;
    assert(esp_now_send(a.peer_addr, &data, 1) == ESP_OK && mock.sent_rate == saved);
    assert((int)mock.sent_rate->phymode == 4 && (int)mock.sent_rate->rate == 16);
    mock.allocation_fail_after = 0;
    assert(rate_set(a.peer_addr, 4, 17, false, false) == ESP_ERR_ESPNOW_NO_MEM);
    assert(mt_get_peer_info(a.peer_addr)->rate == saved && (int)saved->rate == 16);
    mock.allocation_fail_after = -1;
    a.channel = 6; assert(esp_now_mod_peer(&a) == ESP_OK && mt_get_peer_info(a.peer_addr)->rate == saved);
    assert(rate_set(a.peer_addr, 5, 16, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.secondary_channel = 1; mock.bandwidth[0] = 1;
    assert(rate_set(a.peer_addr, 5, 16, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.bandwidth[0] = 2;
    assert(rate_set(a.peer_addr, 5, 16, false, false) == ESP_OK);
    mock.mode[0] = 3;
    assert(rate_set(a.peer_addr, 6, 16, false, false) == ESP_ERR_ESPNOW_ARG);
    mock.mode[0] = 6;
    for (int rate = 16; rate <= 42; ++rate) {
        bool ersu_bad = (rate >= 19 && rate <= 23) || rate > 26;
        bool dcm_bad = rate == 18 || rate == 26 || rate > 28 || (rate >= 21 && rate <= 23);
        assert(rate_set(a.peer_addr, 6, rate, true, false) == (ersu_bad ? ESP_ERR_ESPNOW_ARG : ESP_OK));
        assert(rate_set(a.peer_addr, 6, rate, false, true) == (dcm_bad ? ESP_ERR_ESPNOW_ARG : ESP_OK));
        assert(rate_set(a.peer_addr, 6, rate, true, true) == ((ersu_bad || dcm_bad) ? ESP_ERR_ESPNOW_ARG : ESP_OK));
    }
    a.channel = 36; assert(esp_now_mod_peer(&a) == ESP_OK);
    assert(rate_set(a.peer_addr, 1, 8, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 3, 7, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 3, 8, false, false) == ESP_OK);
    assert(rate_set(a.peer_addr, 3, 15, false, false) == ESP_OK);
    assert(rate_set(a.peer_addr, 3, 16, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 4, 15, false, false) == ESP_ERR_ESPNOW_ARG);
    assert(rate_set(a.peer_addr, 4, 16, false, false) == ESP_OK);
    mock.mode[0] = 4;
    assert(rate_set(a.peer_addr, 1, 8, false, false) == ESP_OK);
    uint8_t missing[6] = {8};
    assert(rate_set(missing, 1, 8, false, false) == ESP_ERR_ESPNOW_NOT_FOUND);
    assert(rate_set(missing, 7, 8, false, false) == ESP_ERR_ESPNOW_ARG);
}
static uint32_t read_u32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void offchannel(void)
{
    start();
    assert(esp_now_deinit() == ESP_OK); /* unrelated to ESP-NOW init flag */
    assert(esp_now_switch_channel_tx(NULL) == ESP_ERR_WIFI_NOT_INIT);
    assert(esp_now_remain_on_channel(NULL) == ESP_ERR_WIFI_NOT_INIT);
    mock.wifi_initialized = true;
    assert(esp_now_switch_channel_tx(NULL) == ESP_ERR_INVALID_ARG);
    assert(esp_now_remain_on_channel(NULL) == ESP_ERR_INVALID_ARG);
    const uint16_t size = 2000;
    esp_now_switch_channel_t *config = calloc(1, sizeof(*config) + size);
    assert(config != NULL);
    config->type = WIFI_OFFCHAN_TX_REQ; config->channel = 11;
    config->sec_channel = WIFI_SECOND_CHAN_ABOVE; config->wait_time_ms = 123456;
    config->op_id = 91; config->data_len = size; memset(config->data, 0x6a, size);
    memset(config->dest_mac, 0xee, 6);
    for (int fail = 0; fail <= 1; ++fail) {
        mock.allocation_fail_after = fail;
        assert(esp_now_switch_channel_tx(config) == ESP_ERR_NO_MEM && config->op_id == 91);
        assert(mock.live_allocations == 0 && mock.ioctl_calls == 0);
    }
    mock.allocation_fail_after = -1;
    assert(esp_now_switch_channel_tx(config) == ESP_OK && config->op_id == 73);
    assert(mock.ioctl_opcode == 0x55 && mock.ioctl_payload_size == 2048);
    const uint8_t *p = mock.ioctl_payload;
    assert(memcmp(p + 4, config->dest_mac, 6) == 0 && read_u32(p + 12) == 1 && p[16] == 11);
    assert(read_u32(p + 20) == 1 && read_u32(p + 24) == 123456 && read_u32(p + 44) == size);
    assert(memcmp(p + 48, config->data, size) == 0);
    for (unsigned i = 0; i < 48; ++i) {
        bool field = (i >= 4 && i < 10) || (i >= 12 && i < 17) || (i >= 20 && i < 28) || i >= 44;
        if (!field) { assert(p[i] == 0); }
    }
    mock.ioctl_result = -19; config->type = WIFI_OFFCHAN_TX_CANCEL;
    assert(esp_now_switch_channel_tx(config) == -19 && read_u32(mock.ioctl_payload + 12) == 0);
    free(config);
    esp_now_remain_on_channel_t roc = {WIFI_ROC_REQ, 7, WIFI_SECOND_CHAN_BELOW, 98765, 91};
    mock.allocation_fail_after = 0;
    assert(esp_now_remain_on_channel(&roc) == ESP_ERR_NO_MEM && roc.op_id == 91);
    mock.allocation_fail_after = -1; mock.ioctl_result = 0;
    assert(esp_now_remain_on_channel(&roc) == ESP_OK && roc.op_id == 73);
    p = mock.ioctl_payload;
    assert(mock.ioctl_opcode == 0x56 && mock.ioctl_payload_size == 32);
    assert(read_u32(p + 4) == 1 && p[8] == 7 && read_u32(p + 12) == 2 && read_u32(p + 16) == 98765);
    assert(read_u32(p) == 0 && read_u32(p + 20) == 0 && read_u32(p + 24) == 0 && read_u32(p + 28) == 0);
    roc.type = WIFI_ROC_CANCEL; mock.ioctl_result = 0x101;
    assert(esp_now_remain_on_channel(&roc) == 0x101 && read_u32(mock.ioctl_payload + 4) == 0);
    assert(mock.live_allocations == 0);
}
int main(void)
{
    lifecycle(); plain_peers(); fetch_and_broadcast(); encrypted_peers();
    failures(); sending(); oui_wake(); rates(); offchannel(); test_peer_model();
    assert(esp_now_deinit() == ESP_OK);
    assert(mock.live_allocations == 0 && mock.locks == mock.unlocks);
    puts("libespnow host semantics: all assertions passed (mock crypto/radio only)");
    return 0;
}

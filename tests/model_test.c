/* SPDX-License-Identifier: MPL-2.0 */
#include "mock_platform.h"
#include <assert.h>
#include <string.h>

void test_peer_model(void);

typedef struct {
    bool present;
    esp_now_peer_info_t value;
    unsigned insertion;
} expected_peer_t;
static uint32_t random_state = UINT32_C(0x82564217);
static uint32_t random_value(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}
static void same_peer(const esp_now_peer_info_t *actual, const esp_now_peer_info_t *expected)
{
    assert(memcmp(actual->peer_addr, expected->peer_addr, 6) == 0);
    assert(actual->channel == expected->channel && actual->ifidx == expected->ifidx);
    assert(actual->priv == expected->priv && actual->encrypt == expected->encrypt);
    uint8_t zero_key[16] = {0};
    assert(memcmp(actual->lmk, expected->encrypt ? expected->lmk : zero_key, 16) == 0);
}
void test_peer_model(void)
{
    assert(esp_now_deinit() == ESP_OK);
    mock_reset();
    assert(esp_now_init() == ESP_OK);
    expected_peer_t model[24] = {0};
    unsigned insertion = 0;
    for (unsigned step = 0; step < 4000; ++step) {
        unsigned id = random_value() % 24;
        unsigned operation = random_value() % 3;
        esp_now_peer_info_t value;
        memset(&value, 0, sizeof(value));
        value.peer_addr[0] = (id % 7 == 0) ? 1 : 2;
        value.peer_addr[5] = (uint8_t)id;
        value.channel = (random_value() % 2 == 0) ? 0 : 6;
        value.ifidx = (wifi_interface_t)(random_value() % 2);
        value.encrypt = (random_value() % 2 == 0) && value.peer_addr[0] == 2;
        for (unsigned i = 0; i < 16; ++i) { value.lmk[i] = (uint8_t)random_value(); }
        value.priv = &model[id];
        unsigned total = 0, encrypted = 0;
        for (unsigned i = 0; i < 24; ++i) {
            total += model[i].present ? 1U : 0U;
            encrypted += model[i].present && model[i].value.encrypt ? 1U : 0U;
        }
        bool fail_crypto = random_value() % 13 == 0;
        mock.fail_crypto = fail_crypto;
        esp_err_t expected, actual;
        if (operation == 0) {
            expected = model[id].present ? ESP_ERR_ESPNOW_EXIST
                : ((total == 20 || (value.encrypt && encrypted == 17)) ? ESP_ERR_ESPNOW_FULL
                : ((value.encrypt && fail_crypto) ? ESP_ERR_ESPNOW_INTERNAL : ESP_OK));
            actual = esp_now_add_peer(&value);
            if (actual == ESP_OK) {
                model[id].present = true;
                model[id].value = value;
                model[id].insertion = ++insertion;
            }
        } else if (operation == 1) {
            expected = !model[id].present ? ESP_ERR_ESPNOW_NOT_FOUND
                : ((value.encrypt && !model[id].value.encrypt && encrypted == 17) ? ESP_ERR_ESPNOW_FULL
                : ((value.encrypt && fail_crypto) ? ESP_ERR_ESPNOW_INTERNAL : ESP_OK));
            actual = esp_now_mod_peer(&value);
            if (actual == ESP_OK) { model[id].value = value; }
        } else {
            expected = model[id].present ? ESP_OK : ESP_ERR_ESPNOW_NOT_FOUND;
            actual = esp_now_del_peer(value.peer_addr);
            if (actual == ESP_OK) { model[id].present = false; }
        }
        assert(actual == expected);
        mock.fail_crypto = false;
        total = 0; encrypted = 0;
        for (unsigned i = 0; i < 24; ++i) {
            esp_now_peer_info_t out;
            uint8_t mac[6] = {0};
            mac[0] = (i % 7 == 0) ? 1 : 2; mac[5] = (uint8_t)i;
            assert(esp_now_is_peer_exist(mac) == model[i].present);
            if (model[i].present) {
                ++total;
                encrypted += model[i].value.encrypt ? 1U : 0U;
                assert(esp_now_get_peer(mac, &out) == ESP_OK);
                same_peer(&out, &model[i].value);
            } else {
                assert(esp_now_get_peer(mac, &out) == ESP_ERR_ESPNOW_NOT_FOUND);
            }
        }
        esp_now_peer_num_t num;
        assert(esp_now_get_peer_num(&num) == ESP_OK);
        assert(num.total_num == (int)total && num.encrypt_num == (int)encrypted);
        unsigned masks = 0;
        for (unsigned slot = 0; slot < 25; ++slot) {
            assert(!(mock.slots[0][slot] && mock.slots[1][slot]));
            masks += mock.slots[0][slot] || mock.slots[1][slot] ? 1U : 0U;
        }
        assert(masks == encrypted);
        unsigned previous_insertion = 0, fetched = 0, unicast = 0;
        for (unsigned i = 0; i < 24; ++i) {
            unicast += model[i].present && model[i].value.peer_addr[0] == 2 ? 1U : 0U;
        }
        esp_now_peer_info_t out;
        for (bool first = true; esp_now_fetch_peer(first, &out) == ESP_OK; first = false) {
            unsigned index = out.peer_addr[5];
            assert(index < 24 && model[index].present && model[index].value.peer_addr[0] == 2);
            assert(model[index].insertion > previous_insertion);
            previous_insertion = model[index].insertion;
            same_peer(&out, &model[index].value);
            ++fetched;
        }
        assert(fetched == unicast);
    }
    assert(esp_now_deinit() == ESP_OK && mock.live_allocations == 0);
}

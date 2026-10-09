/* SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Host-only declarations adapted from supplied public SDK type excerpts. */
#ifndef HOST_ESP_WIFI_TYPES_H
#define HOST_ESP_WIFI_TYPES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum { WIFI_IF_STA, WIFI_IF_AP, WIFI_IF_MAX } wifi_interface_t;
typedef enum { WIFI_PHY_MODE_LR, WIFI_PHY_MODE_11B, WIFI_PHY_MODE_11G,
    WIFI_PHY_MODE_11A, WIFI_PHY_MODE_HT20, WIFI_PHY_MODE_HT40,
    WIFI_PHY_MODE_HE20, WIFI_PHY_MODE_VHT20 } wifi_phy_mode_t;
/* Values used by the contract; holes are intentionally left unenumerated. */
typedef enum { WIFI_PHY_RATE_1M_L = 0, WIFI_PHY_RATE_6M = 11,
    WIFI_PHY_RATE_MCS0_LGI = 16, WIFI_PHY_RATE_LORA_250K = 41,
    WIFI_PHY_RATE_LORA_500K = 42 } wifi_phy_rate_t;
typedef enum { WIFI_SEND_SUCCESS, WIFI_SEND_FAIL } wifi_tx_status_t;
typedef struct { uint8_t *des_addr; uint8_t *src_addr; wifi_interface_t ifidx;
    uint8_t *data; uint8_t data_len; wifi_phy_rate_t rate;
    wifi_tx_status_t tx_status; } wifi_tx_info_t;
typedef struct { wifi_phy_mode_t phymode; wifi_phy_rate_t rate;
    bool ersu; bool dcm; } wifi_tx_rate_config_t;
typedef enum { WIFI_OFFCHAN_TX_CANCEL, WIFI_OFFCHAN_TX_REQ,
    WIFI_OFFCHAN_TX_CONNECTING_REQ } wifi_action_tx_t;
typedef enum { WIFI_ROC_CANCEL, WIFI_ROC_REQ, WIFI_ROC_CONNECTING_REQ } wifi_roc_t;
typedef enum { WIFI_SECOND_CHAN_NONE, WIFI_SECOND_CHAN_ABOVE,
    WIFI_SECOND_CHAN_BELOW } wifi_second_chan_t;
/* Only a pointer to this opaque receive metadata is needed in host tests. */
typedef struct host_wifi_rx_ctrl wifi_pkt_rx_ctrl_t;
#endif

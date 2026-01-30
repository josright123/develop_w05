/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_eth_mac.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// DM9051 PTP-related ioctl commands
#define ETH_DM9051_CMD_PTP_ENABLE          (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 0)
#define ETH_DM9051_CMD_S_PTP_TIME          (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 1)
#define ETH_DM9051_CMD_G_PTP_TIME          (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 2)
#define ETH_DM9051_CMD_SET_TARGET_TIME     (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 3)
#define ETH_DM9051_CMD_GET_TARGET_TIME     (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 4)
#define ETH_DM9051_CMD_ADJ_PTP_TIME        (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 5)
#define ETH_DM9051_CMD_SET_TARGET_CB       (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 6)
#define ETH_DM9051_CMD_DEL_TARGET_CB       (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 7)
#define ETH_DM9051_CMD_ENABLE_RECEIVE_LOG  (ETH_CMD_CUSTOM_MAC_CMDS_OFFSET + 8)

/**
 * @brief DM9051 custom ioctl handler for PTP and other extensions
 *
 * @param mac Ethernet MAC instance
 * @param cmd ioctl command
 * @param data data for the command
 * @return
 *      - ESP_OK: succeed
 *      - ESP_ERR_INVALID_ARG: invalid argument
 *      - others: other failures
 */
esp_err_t emac_dm9051_custom_ioctl(esp_eth_mac_t *mac, int cmd, void *data);

#ifdef __cplusplus
}
#endif

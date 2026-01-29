/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "esp_log.h"
#include "esp_eth_time.h"

#if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)

static esp_eth_handle_t s_eth_hndl;

static int esp_eth_clock_esp_err_to_errno(esp_err_t esp_err)
{
    switch (esp_err)
    {
    case ESP_ERR_INVALID_ARG:
        return EINVAL;
    case ESP_ERR_INVALID_STATE:
        return EBUSY;
    case ESP_ERR_TIMEOUT:
        return ETIME;
    }
    // default "no err" when error cannot be isolated
    return 0;
}

int esp_eth_clock_adjtime(clockid_t clk_id, esp_eth_clock_adj_param_t *adj)
{
    switch (clk_id) {
    case CLOCK_PTP_SYSTEM:
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
        if (adj->mode == ETH_CLK_ADJ_FREQ_SCALE) {
            esp_err_t ret = esp_eth_ioctl(s_eth_hndl, ETH_MAC_ESP_CMD_ADJ_PTP_FREQ, &adj->freq_scale);
            if (ret != ESP_OK) {
                errno = esp_eth_clock_esp_err_to_errno(ret);
                return -1;
            }
        } else {
            errno = EINVAL;
            return -1;
        }
#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
        if (adj->mode == ETH_CLK_ADJ_FREQ_SCALE) {
            esp_err_t ret = esp_dm9051_ioctl(s_eth_hndl, DM9051_CMD_ADJ_PTP_TIME, &adj->freq_scale);
            if (ret != ESP_OK) {
                errno = esp_eth_clock_esp_err_to_errno(ret);
                return -1;
            }
        } else {
            errno = EINVAL;
            return -1;
        }
#endif
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int esp_eth_clock_settime(clockid_t clock_id, const struct timespec *tp)
{
    switch (clock_id) {
    case CLOCK_PTP_SYSTEM: {
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
        if (s_eth_hndl) {
            eth_mac_time_t ptp_time = {
                .seconds = tp->tv_sec,
                .nanoseconds = tp->tv_nsec
            };
            esp_err_t ret = esp_eth_ioctl(s_eth_hndl, ETH_MAC_ESP_CMD_S_PTP_TIME, &ptp_time);
            if (ret != ESP_OK) {
                errno = esp_eth_clock_esp_err_to_errno(ret);
                return -1;
            }
        } else {
            errno = ENODEV;
            return -1;
        }
#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
        if (s_eth_hndl) {
            dm9051_time_t ptp_time = {
                .seconds = tp->tv_sec,
                .nanoseconds = tp->tv_nsec
            };
            esp_err_t ret = esp_dm9051_ioctl(s_eth_hndl, DM9051_CMD_S_PTP_TIME, &ptp_time);
            if (ret != ESP_OK) {
                errno = esp_eth_clock_esp_err_to_errno(ret);
                return -1;
            }
        } else {
            errno = ENODEV;
            return -1;
        }
#endif
        break;
    }
    default:
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int esp_eth_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    switch (clock_id) {
    case CLOCK_PTP_SYSTEM: {
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
        if (s_eth_hndl) {
            eth_mac_time_t ptp_time;
            esp_err_t ret = esp_eth_ioctl(s_eth_hndl, ETH_MAC_ESP_CMD_G_PTP_TIME, &ptp_time);
            if (ret != ESP_OK) {
                errno = esp_eth_clock_esp_err_to_errno(ret);
                return -1;
            }
            tp->tv_sec = ptp_time.seconds;
            tp->tv_nsec = ptp_time.nanoseconds;
        } else {
            errno = ENODEV;
            return -1;
        }
#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
        if (s_eth_hndl) {
            dm9051_time_t ptp_time;
            esp_err_t ret = esp_dm9051_ioctl(s_eth_hndl, DM9051_CMD_G_PTP_TIME, &ptp_time);
            if (ret != ESP_OK) {
                errno = esp_eth_clock_esp_err_to_errno(ret);
                return -1;
            }
            tp->tv_sec = ptp_time.seconds;
            tp->tv_nsec = ptp_time.nanoseconds;
        } else {
            errno = ENODEV;
            return -1;
        }
#endif
        break;
    }
    default:
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int esp_eth_clock_set_target_time(clockid_t clock_id, struct timespec *tp)
{
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
    eth_mac_time_t mac_target_time = {
        .seconds = tp->tv_sec,
        .nanoseconds = tp->tv_nsec
    };
    esp_err_t ret = esp_eth_ioctl(s_eth_hndl, ETH_MAC_ESP_CMD_S_TARGET_TIME, &mac_target_time);
    if (ret != ESP_OK) {
        errno = esp_eth_clock_esp_err_to_errno(ret);
        return -1;
    }
    return 0;
#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
    dm9051_time_t mac_target_time = {
        .seconds = tp->tv_sec,
        .nanoseconds = tp->tv_nsec
    };
    esp_err_t ret = esp_dm9051_ioctl(s_eth_hndl, DM9051_CMD_S_TARGET_TIME, &mac_target_time);
    if (ret != ESP_OK) {
        errno = esp_eth_clock_esp_err_to_errno(ret);
        return -1;
    }
    return 0;
#endif
}

int esp_eth_clock_register_target_cb(clockid_t clock_id,
                                     ts_target_exceed_cb_from_isr_t ts_callback)
{
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
    esp_err_t ret = esp_eth_ioctl(s_eth_hndl, ETH_MAC_ESP_CMD_S_TARGET_CB, ts_callback);
    if (ret != ESP_OK) {
        errno = esp_eth_clock_esp_err_to_errno(ret);
        return -1;
    }
    return 0;
#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
    // TODO: Implement DM9051 PTP callback registration
    // esp_err_t ret = esp_dm9051_ioctl(s_eth_hndl, DM9051_CMD_S_TARGET_CB, ts_callback);
    // if (ret != ESP_OK) {
    esp_err_t ret = esp_dm9051_ioctl(s_eth_hndl, DM9051_CMD_S_TARGET_CB, ts_callback);
    if (ret != ESP_OK) {
        errno = esp_eth_clock_esp_err_to_errno(ret);
        return -1;
    }
    return 0
esp_err_t esp_eth_clock_init(clockid_t clock_id, esp_eth_clock_cfg_t *cfg)
{
    switch (clock_id) {
    case CLOCK_PTP_SYSTEM:
#if defined(CONFIG_ETH_USE_ESP32_EMAC)
        // PTP Clock is part of Ethernet system
        bool ptp_enable = true;
        if (esp_eth_ioctl(cfg->eth_hndl, ETH_MAC_ESP_CMD_PTP_ENABLE, &ptp_enable) != ESP_OK) {
            return ESP_FAIL;
        }
        s_eth_hndl = cfg->eth_hndl;
        break;
#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
        // Initialize DM9051 PTP
        bool ptp_enable = true;
        if (esp_dm9051_ioctl(cfg->eth_hndl, DM9051_CMD_PTP_ENABLE, &ptp_enable) != ESP_OK) {
            ESP_LOGE("esp_eth_clock", "Failed to enable DM9051 PTP");
            return ESP_FAIL;
        }
        s_eth_hndl = cfg->eth_hndl;
        ESP_LOGI("esp_eth_clock", "DM9051 PTP initialized successfully");
        break;
    }
    return ESP_OK;
}

#endif // CONFIG_ETH_USE_ESP32_EMAC || CONFIG_ETH_USE_ESP32_DM9051_PTP

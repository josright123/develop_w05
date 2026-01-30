/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_eth_mac_dm9051_ptp.h"
#include "esp_eth_com.h"
#include "esp_eth_time.h"
#include "esp_eth_mac_dm9051_internal.h"
#include "dm9051.h"

static const char *TAG = "dm9051.ptp";

/* DM9051 PTP Constants */
#define V51_ADJ_FREQ_BASE_ADDEND 171.7987 /* Base addend for frequency adjustment */
#define MAX_ADJUSTMENT 0xEFFFFFFF         /* Maximum adjustment value */

/* Global state for cumulative adjustments */
static int64_t s_last_rate = 0;

/**
 * @brief Read DM9051 register
 */
static esp_err_t dm9051_register_read(emac_dm9051_t *emac, uint8_t reg_addr, uint8_t *value)
{
    return emac->spi.read(emac->spi.ctx, 0, reg_addr, value, 1);
}

/**
 * @brief Write DM9051 register
 */
static esp_err_t dm9051_register_write(emac_dm9051_t *emac, uint8_t reg_addr, uint8_t value)
{
    return emac->spi.write(emac->spi.ctx, 1, reg_addr, &value, 1);
}

/**
 * @brief Initialize DM9051 PTP functionality
 */
static esp_err_t dm9051_ptp_enable(emac_dm9051_t *emac, bool enable)
{
    esp_err_t ret = ESP_OK;

    if (enable) {
        /* PTP restart sequence */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCR, PTPCR_RESTART), err, TAG, "write PTPCR failed");
        esp_rom_delay_us(1000);
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCR, 0x00), err, TAG, "clear PTPCR failed");

        /* Enable PTP functionality */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_PTP_ENABLE), err, TAG, "write PTPCCR failed");

        /* Disable TX timestamp capture initially */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_TCR, 0x00), err, TAG, "write TCR failed");

        /* Master/Slave Mode & 1588 Version Register */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPMSR, PTPMSR_RX_EN | PTPMSR_MULTICAST), err, TAG, "write PTPMSR failed");

        /* TX One Step configuration */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPTXCR, 0x00), err, TAG, "write PTPTXCR failed");

        /* 1-step sync packet configuration */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPDELR1, 0x4E), err, TAG, "write PTPDELR1 failed");
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPDELR2, 0x3C), err, TAG, "write PTPDELR2 failed");

        ESP_LOGI(TAG, "PTP enabled successfully (on ETH_DM9051_CMD_PTP_ENABLE 0x%04x)", ETH_DM9051_CMD_PTP_ENABLE);
    } else {
        /* Disable PTP */
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, 0x00), err, TAG, "disable PTP failed");
        ESP_LOGI(TAG, "PTP disabled");
    }

err:
    return ret;
}

/**
 * @brief Get PTP time from DM9051
 */
static esp_err_t dm9051_ptp_get_time(emac_dm9051_t *emac, eth_mac_time_t *timestamp)
{
    esp_err_t ret = ESP_OK;
    uint8_t time_buf[8];

    ESP_RETURN_ON_FALSE(timestamp, ESP_ERR_INVALID_ARG, TAG, "timestamp pointer is NULL");

    /* Setup register to read PTP clock time */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_IDX_RESET | PTPCCR_READ_TIME), err, TAG, "setup read time failed");

    /* Read 8 bytes of time data */
    for (int i = 0; i < 8; i++) {
        ESP_GOTO_ON_ERROR(dm9051_register_read(emac, DM9051_PTPDIDR, &time_buf[i]), err, TAG, "read time data failed");
    }

    /* Convert to timestamp format (nanoseconds first, then seconds) */
    timestamp->nanoseconds = (uint32_t)time_buf[0] | ((uint32_t)time_buf[1] << 8) |
                            ((uint32_t)time_buf[2] << 16) | ((uint32_t)time_buf[3] << 24);
    timestamp->seconds = (uint32_t)time_buf[4] | ((uint32_t)time_buf[5] << 8) |
                        ((uint32_t)time_buf[6] << 16) | ((uint32_t)time_buf[7] << 24);

err:
    return ret;
}

/**
 * @brief Set PTP time in DM9051
 */
static esp_err_t dm9051_ptp_set_time(emac_dm9051_t *emac, const eth_mac_time_t *timestamp)
{
    esp_err_t ret = ESP_OK;
    uint8_t time_buf[8];

    ESP_RETURN_ON_FALSE(timestamp, ESP_ERR_INVALID_ARG, TAG, "timestamp pointer is NULL");

    /* Convert timestamp to byte array (nanoseconds first, then seconds) */
    time_buf[0] = (uint8_t)(timestamp->nanoseconds & 0x000000FF);
    time_buf[1] = (uint8_t)((timestamp->nanoseconds & 0x0000FF00) >> 8);
    time_buf[2] = (uint8_t)((timestamp->nanoseconds & 0x00FF0000) >> 16);
    time_buf[3] = (uint8_t)((timestamp->nanoseconds & 0xFF000000) >> 24);
    time_buf[4] = (uint8_t)(timestamp->seconds & 0x000000FF);
    time_buf[5] = (uint8_t)((timestamp->seconds & 0x0000FF00) >> 8);
    time_buf[6] = (uint8_t)((timestamp->seconds & 0x00FF0000) >> 16);
    time_buf[7] = (uint8_t)((timestamp->seconds & 0xFF000000) >> 24);

    /* PTP restart sequence */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCR, PTPCR_RESTART), err, TAG, "write PTPCR failed");
    esp_rom_delay_us(2);
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCR, 0x00), err, TAG, "clear PTPCR failed");
    s_last_rate = 0; /* Reset accumulated adjustment */

    /* Reset index and write time data */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_IDX_RESET), err, TAG, "reset index failed");

    /* Write 8 bytes of time data */
    for (int i = 0; i < 8; i++) {
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPDIDR, time_buf[i]), err, TAG, "write time data failed");
    }

    /* Apply time setting */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_WRITE_TIME | PTPCCR_PTP_ENABLE), err, TAG, "apply time setting failed");

    ESP_LOGI(TAG, "PTP time set to %u.%09u", timestamp->seconds, timestamp->nanoseconds);

err:
    return ret;
}

/**
 * @brief Adjust PTP frequency
 * @param adjustment: Adjustment value
 * @param direction: 0 = faster, 1 = slower
 */
static esp_err_t dm9051_adjust_ptp_frequency(emac_dm9051_t *emac, uint32_t adjustment, int8_t direction)
{
    esp_err_t ret = ESP_OK;
    uint8_t rate_bytes[4];
    uint8_t control_value;
    uint8_t reg_index[4];
    uint8_t index_expected[4] = {0x10, 0x20, 0x30, 0x40};
    bool valid_indices = true;

    /* Parameter validation */
    ESP_RETURN_ON_FALSE(adjustment <= MAX_ADJUSTMENT, ESP_ERR_INVALID_ARG, TAG, 
                       "adjustment value 0x%08lX exceeds maximum", adjustment);

    /* Convert 32-bit adjustment to byte array (little-endian) */
    rate_bytes[0] = (uint8_t)(adjustment);
    rate_bytes[1] = (uint8_t)(adjustment >> 8);
    rate_bytes[2] = (uint8_t)(adjustment >> 16);
    rate_bytes[3] = (uint8_t)(adjustment >> 24);

    /* Reset register index */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_IDX_RESET), err, TAG, "reset index failed");

    /* Write and verify each byte */
    for (uint8_t i = 0; i < 4; i++) {
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPDIDR, rate_bytes[i]), err, TAG, "write rate byte failed");
        ESP_GOTO_ON_ERROR(dm9051_register_read(emac, DM9051_PTPIDXR, &reg_index[i]), err, TAG, "read index failed");

        /* Check if index matches expected value */
        if (reg_index[i] != index_expected[i]) {
            valid_indices = false;
        }
    }

    /* Apply adjustment only if indices are valid */
    if (valid_indices) {
        control_value = (direction == 1) ? PTPCCR_RATE_SLOWER : PTPCCR_RATE_FASTER;
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, control_value), err, TAG, "apply rate adjustment failed");
    } else {
        ESP_LOGW(TAG, "Rate adjustment skipped due to invalid indices");
        ret = ESP_ERR_INVALID_STATE;
    }

err:
    return ret;
}

/**
 * @brief Adjust PTP frequency based on ppb value
 * @param adj_ppb: Adjustment value in ppb (parts per billion)
 */
static esp_err_t dm9051_ptp_adj_freq(emac_dm9051_t *emac, int32_t adj_ppb)
{
    /* DM9051 PTP clock 25MHz calculation: 2^32 * 1ppb / 25MHz = 171.79869184 */
    int64_t signed_addend = (int64_t)(adj_ppb * V51_ADJ_FREQ_BASE_ADDEND);
    int64_t delta_rate = signed_addend - s_last_rate;

    /* Determine adjustment direction and value */
    uint32_t adjust_value;
    int8_t adjust_direction;

    if (delta_rate < 0) {
        adjust_direction = 1; // Slower
        adjust_value = (uint32_t)(-delta_rate);
    } else {
        adjust_direction = 0; // Faster
        adjust_value = (uint32_t)delta_rate;
    }

    /* Call hardware adjustment function */
    esp_err_t ret = dm9051_adjust_ptp_frequency(emac, adjust_value, adjust_direction);

    if (ret == ESP_OK) {
        /* Update cumulative adjustment */
        s_last_rate = signed_addend;
        ESP_LOGD(TAG, "PTP freq adjusted: %d ppb, delta_rate: %lld, direction: %s",
                adj_ppb, delta_rate, (delta_rate > 0) ? "faster" : (delta_rate < 0) ? "slower" : "no change");
    }

    return ret;
}

/**
 * @brief Update PTP time offset
 */
static esp_err_t dm9051_ptp_update_offset(emac_dm9051_t *emac, const eth_mac_time_t *timeoffset)
{
    esp_err_t ret = ESP_OK;
    uint8_t time_buf[8];

    ESP_RETURN_ON_FALSE(timeoffset, ESP_ERR_INVALID_ARG, TAG, "timeoffset pointer is NULL");

    /* Convert to timestamp format (nanoseconds first, then seconds) */
    time_buf[0] = (uint8_t)(timeoffset->nanoseconds & 0x000000FF);
    time_buf[1] = (uint8_t)((timeoffset->nanoseconds & 0x0000FF00) >> 8);
    time_buf[2] = (uint8_t)((timeoffset->nanoseconds & 0x00FF0000) >> 16);
    time_buf[3] = (uint8_t)((timeoffset->nanoseconds & 0xFF000000) >> 24);
    time_buf[4] = (uint8_t)(timeoffset->seconds & 0x000000FF);
    time_buf[5] = (uint8_t)((timeoffset->seconds & 0x0000FF00) >> 8);
    time_buf[6] = (uint8_t)((timeoffset->seconds & 0x00FF0000) >> 16);
    time_buf[7] = (uint8_t)((timeoffset->seconds & 0xFF000000) >> 24);

    /* Reset index and write time data */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_IDX_RESET), err, TAG, "reset index failed");

    /* Write 8 bytes of time data */
    for (int i = 0; i < 8; i++) {
        ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPDIDR, time_buf[i]), err, TAG, "write time data failed");
    }

    /* Apply time offset */
    ESP_GOTO_ON_ERROR(dm9051_register_write(emac, DM9051_PTPCCR, PTPCCR_WRITE_TIME_OFFSET), err, TAG, "apply time offset failed");

    ESP_LOGD(TAG, "PTP time offset updated: %d.%09u", timeoffset->seconds, timeoffset->nanoseconds);

err:
    return ret;
}

/**
 * @brief DM9051 custom ioctl handler
 */
esp_err_t emac_dm9051_custom_ioctl(esp_eth_mac_t *mac, int cmd, void *data)
{
    emac_dm9051_t *emac = __containerof(mac, emac_dm9051_t, parent);
    esp_err_t ret = ESP_OK;
    
    switch (cmd) {
    case ETH_DM9051_CMD_ENABLE_RECEIVE_LOG:
        emac->link_up_log_count = 0;
        ESP_LOGI(TAG, "Enabled logging for the first three received packets");
        break;
        
    case ETH_DM9051_CMD_PTP_ENABLE: {
        bool enable = (data != NULL) ? *(bool *)data : true;
        ret = dm9051_ptp_enable(emac, enable);
        break;
    }
        
    case ETH_DM9051_CMD_S_PTP_TIME: {
        if (data) {
            ret = dm9051_ptp_set_time(emac, (const eth_mac_time_t *)data);
        } else {
            ESP_LOGE(TAG, "Set PTP time: data pointer is NULL");
            ret = ESP_ERR_INVALID_ARG;
        }
        break;
    }
        
    case ETH_DM9051_CMD_G_PTP_TIME: {
        if (data) {
            ret = dm9051_ptp_get_time(emac, (eth_mac_time_t *)data);
        } else {
            ESP_LOGE(TAG, "Get PTP time: data pointer is NULL");
            ret = ESP_ERR_INVALID_ARG;
        }
        break;
    }
        
    case ETH_DM9051_CMD_SET_TARGET_TIME:
        /* Store target time but don't enable interrupt yet */
        if (data) {
            eth_mac_time_t *target_time = (eth_mac_time_t *)data;
            ESP_LOGD(TAG, "Target time set (stored but not enabled): %u.%09u",
                    target_time->seconds, target_time->nanoseconds);
            ret = ESP_OK;
        } else {
            ESP_LOGE(TAG, "Set target time: data pointer is NULL");
            ret = ESP_ERR_INVALID_ARG;
        }
        break;
        
    case ETH_DM9051_CMD_GET_TARGET_TIME:
        if (data) {
            memset(data, 0, sizeof(eth_mac_time_t));
            ESP_LOGD(TAG, "Get target time (returns zero - not yet implemented)");
            ret = ESP_OK;
        } else {
            ESP_LOGE(TAG, "Get target time: data pointer is NULL");
            ret = ESP_ERR_INVALID_ARG;
        }
        break;
        
    case ETH_DM9051_CMD_ADJ_PTP_TIME: {
        if (data) {
            eth_mac_time_t *offset = (eth_mac_time_t *)data;
            ret = dm9051_ptp_update_offset(emac, offset);
        } else {
            ESP_LOGE(TAG, "Adjust PTP time: data pointer is NULL");
            ret = ESP_ERR_INVALID_ARG;
        }
        break;
    }
        
    case ETH_DM9051_CMD_SET_TARGET_CB:
        emac->ts_target_exceed_cb = (ts_target_exceed_cb_from_isr_t)data;
        ESP_LOGI(TAG, "Target callback registered successfully");
        ret = ESP_OK;
        break;
        
    case ETH_DM9051_CMD_DEL_TARGET_CB:
        emac->ts_target_exceed_cb = NULL;
        ESP_LOGI(TAG, "Target callback removed");
        ret = ESP_OK;
        break;
        
    default:
        ESP_LOGE(TAG, "Unknown ioctl command: %d", cmd);
        ret = ESP_ERR_INVALID_ARG;
        break;
    }
    
    return ret;
}

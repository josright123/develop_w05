/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_eth_mac_spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DM9051_HASH_FILTER_TABLE_SIZE (64)
#define ETH_ADDR_LEN (6)

typedef bool (*ts_target_exceed_cb_from_isr_t)(esp_eth_mediator_t *eth, void *user_args);

typedef struct {
    spi_device_handle_t hdl;
    SemaphoreHandle_t lock;
} eth_spi_info_t;

typedef struct {
    void *ctx;
    void *(*init)(const void *spi_config);
    esp_err_t (*deinit)(void *spi_ctx);
    esp_err_t (*read)(void *spi_ctx, uint32_t cmd, uint32_t addr, void *data, uint32_t data_len);
    esp_err_t (*write)(void *spi_ctx, uint32_t cmd, uint32_t addr, const void *data, uint32_t data_len);
} eth_spi_custom_driver_t;

typedef struct {
    esp_eth_mac_t parent;
    esp_eth_mediator_t *eth;
    eth_spi_custom_driver_t spi;
    TaskHandle_t rx_task_hdl;
    SemaphoreHandle_t multi_reg_axs_mutex;
    uint32_t sw_reset_timeout_ms;
    int int_gpio_num;
    esp_timer_handle_t poll_timer;
    uint32_t poll_period_ms;
    uint8_t addr[ETH_ADDR_LEN];
    bool packets_remain;
    bool flow_ctrl_enabled;
    uint8_t *rx_buffer;
    uint8_t hash_filter_cnt[DM9051_HASH_FILTER_TABLE_SIZE];
    uint8_t link_up_log_count;
    ts_target_exceed_cb_from_isr_t ts_target_exceed_cb;
} emac_dm9051_t;

#ifdef __cplusplus
}
#endif

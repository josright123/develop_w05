#pragma once

#include <sys/time.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_eth_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_PTP_SYSTEM         ((clockid_t) 19)

#if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)

/**
 * @brief DM9051 PTP time structure (same format as eth_mac_time_t)
 *
 */
typedef struct {
    uint32_t seconds;       /*!< Seconds */
    uint32_t nanoseconds;   /*!< Nanoseconds */
} dm9051_time_t;

/**
 * @brief DM9051 PTP ioctl commands - use ESP-IDF official definitions
 * 
 * These commands are defined in esp_eth_mac_spi.h and must match exactly
 * to ensure proper routing through esp_eth_ioctl() to emac_dm9051_custom_ioctl()
 */
#include "esp_eth_mac_spi.h"

// Use official ESP-IDF command definitions:
// ETH_DM9051_CMD_PTP_ENABLE       = (0x0FFF + 0) = 0x0FFF
// ETH_DM9051_CMD_S_PTP_TIME       = (0x0FFF + 1) = 0x1000
// ETH_DM9051_CMD_G_PTP_TIME       = (0x0FFF + 2) = 0x1001
// ETH_DM9051_CMD_SET_TARGET_TIME  = (0x0FFF + 3) = 0x1002
// ETH_DM9051_CMD_GET_TARGET_TIME  = (0x0FFF + 4) = 0x1003
// ETH_DM9051_CMD_ADJ_PTP_TIME     = (0x0FFF + 5) = 0x1004
// ETH_DM9051_CMD_SET_TARGET_CB    = (0x0FFF + 6) = 0x1005
// ETH_DM9051_CMD_DEL_TARGET_CB    = (0x0FFF + 7) = 0x1006

// Compatibility aliases for code readability
#define DM9051_CMD_PTP_ENABLE        ETH_DM9051_CMD_PTP_ENABLE
#define DM9051_CMD_S_PTP_TIME        ETH_DM9051_CMD_S_PTP_TIME
#define DM9051_CMD_G_PTP_TIME        ETH_DM9051_CMD_G_PTP_TIME
#define DM9051_CMD_S_TARGET_TIME     ETH_DM9051_CMD_SET_TARGET_TIME
#define DM9051_CMD_S_TARGET_CB       ETH_DM9051_CMD_SET_TARGET_CB
#define DM9051_CMD_ADJ_PTP_TIME      ETH_DM9051_CMD_ADJ_PTP_TIME

/**
 * @brief DM9051 ioctl function - wrapper for esp_eth_ioctl
 *
 * This function directly calls esp_eth_ioctl, which will route the command
 * to the appropriate custom_ioctl handler (emac_dm9051_custom_ioctl)
 * based on the command offset (>= ETH_CMD_CUSTOM_MAC_CMDS).
 */
static inline esp_err_t esp_dm9051_ioctl(esp_eth_handle_t hdl, uint32_t cmd, void *data)
{
    return esp_eth_ioctl(hdl, cmd, data);
}

/**
 * @brief Configuration of clock during initialization
 *
 */
typedef struct {
    esp_eth_handle_t eth_hndl;
} esp_eth_clock_cfg_t;

/**
 * @brief The mode of clock adjustment.
 *
 */
typedef enum {
    ETH_CLK_ADJ_FREQ_SCALE,
} esp_eth_clock_adj_mode_t;

/**
 * @brief Structure containing parameters for adjusting the Ethernet clock.
 *
 */
typedef struct {
    /**
     * @brief The mode of clock adjustment.
     *
     */
    esp_eth_clock_adj_mode_t mode;

    /**
     * @brief The frequency scale factor when in ETH_CLK_ADJ_FREQ_SCALE mode.
     *
     * This value represents the ratio of the desired frequency to the actual
     * frequency. A value greater than 1 increases the frequency, while a value
     * less than 1 decreases the frequency.
     */
    double freq_scale;
} esp_eth_clock_adj_param_t;

/**
 * @brief Adjust the system clock frequency
 *
 * @param clk_id Identifier of the clock to adjust
 * @param buf Pointer to the adjustment parameters
 *
 * @return
 *     - 0: Success
 *     - -1: Failure
 */
int esp_eth_clock_adjtime(clockid_t clk_id, esp_eth_clock_adj_param_t *adj);

/**
 * @brief Set the system clock time
 *
 * @param clk_id Identifier of the clock to set
 * @param tp Pointer to the new time
 *
 * @return
 *     - 0: Success
 *     - -1: Failure
 */
int esp_eth_clock_settime(clockid_t clock_id, const struct timespec *tp);

/**
 * @brief Get the current system clock time
 *
 * @param clk_id Identifier of the clock to query
 * @param tp Pointer to the buffer to store the current time
 *
 * @return
 *     - 0: Success
 *     - -1: Failure
 */
int esp_eth_clock_gettime(clockid_t clock_id, struct timespec *tp);

/**
 * @brief Set the target time for the system clock.
 *
 * @param clk_id Identifier of the clock to set the target time for
 * @param tp Pointer to the target time
 *
 * @return
 *     - 0: Success
 *     - -1: Failure
 */
int esp_eth_clock_set_target_time(clockid_t clock_id, struct timespec *tp);

/**
 * @brief Callback function type for timestamp target exceed interrupt
 */
typedef bool (*ts_target_exceed_cb_from_isr_t)(esp_eth_mediator_t *eth, void *user_args);

/**
 * @brief Register callback function invoked on Time Stamp target time exceeded interrupt
 *
 * @param clock_id Identifier of the clock
 * @param ts_callback callback function to be registered
 * @return
 *     - 0: Success
 *     - -1: Failure
 */
int esp_eth_clock_register_target_cb(clockid_t clock_id,
                                     ts_target_exceed_cb_from_isr_t ts_callback);

/**
 * @brief Initialize the Ethernet clock subsystem
 *
 * @param clk_id Identifier of the clock to initialize
 * @param cfg Pointer to the configuration structure
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Failure
 */
esp_err_t esp_eth_clock_init(clockid_t clock_id, esp_eth_clock_cfg_t *cfg);

#endif // CONFIG_ETH_USE_ESP32_EMAC || CONFIG_ETH_USE_ESP32_DM9051_PTP

#ifdef __cplusplus
}
#endif

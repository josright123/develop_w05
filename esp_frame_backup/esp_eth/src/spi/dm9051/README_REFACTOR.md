# DM9051 MAC 驅動程式重構說明

## 概述

此重構將 DM9051 以太網 MAC 驅動程式分成兩個部分：
- **esp_eth_mac_dm9051.c**: 基本 MAC 驅動程式（恢復到原始 ESP-IDF 版本）
- **esp_eth_mac_dm9051_ptp.c**: PTP（精確時間協定）擴展功能

## 檔案結構

```
esp-idf/components/esp_eth/src/spi/dm9051/
├── esp_eth_mac_dm9051.c           # 基本 MAC 驅動程式
├── esp_eth_mac_dm9051_ptp.h       # PTP 擴展標頭檔
└── esp_eth_mac_dm9051_ptp.c       # PTP 擴展實作
```

## 使用方式

### 1. 僅使用基本 MAC 功能

如果您不需要 PTP 功能，可以直接使用原始的 DM9051 驅動程式，無需任何修改。

```c
#include "esp_eth_mac_spi.h"

// 標準初始化代碼
esp_eth_mac_t *mac = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config);
```

### 2. 使用 PTP 擴展功能

如果需要 PTP 功能支援，請包含 PTP 標頭檔並註冊 custom_ioctl 處理函數：

```c
#include "esp_eth_mac_spi.h"
#include "esp_eth_mac_dm9051_ptp.h"

// 初始化 MAC
esp_eth_mac_t *mac = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config);

// 註冊 PTP 擴展的 custom_ioctl 處理函數
mac->custom_ioctl = emac_dm9051_custom_ioctl;

// 現在可以使用 PTP 相關的 ioctl 命令
```

## PTP 相關的 ioctl 命令

PTP 擴展模組提供以下 ioctl 命令：

- `ETH_DM9051_CMD_PTP_ENABLE`: 啟用 PTP 功能
- `ETH_DM9051_CMD_S_PTP_TIME`: 設置 PTP 時間
- `ETH_DM9051_CMD_G_PTP_TIME`: 獲取 PTP 時間
- `ETH_DM9051_CMD_SET_TARGET_TIME`: 設置目標時間
- `ETH_DM9051_CMD_GET_TARGET_TIME`: 獲取目標時間
- `ETH_DM9051_CMD_ADJ_PTP_TIME`: 調整 PTP 時間
- `ETH_DM9051_CMD_SET_TARGET_CB`: 設置目標回調
- `ETH_DM9051_CMD_DEL_TARGET_CB`: 刪除目標回調
- `ETH_DM9051_CMD_ENABLE_RECEIVE_LOG`: 啟用接收日誌

## 重要注意事項

1. **向後兼容性**: 基本驅動程式保持與原始 ESP-IDF 版本兼容
2. **可選 PTP 支援**: PTP 功能是可選的，僅在需要時啟用
3. **模組化設計**: PTP 功能獨立於基本驅動程式，便於維護和更新

## 編譯配置

如果您的專案使用 PTP 功能，請確保在 CMakeLists.txt 中包含 PTP 源文件：

```cmake
idf_component_register(
    SRCS "esp_eth_mac_dm9051.c" "esp_eth_mac_dm9051_ptp.c"
    # ... 其他配置
)
```

## 未來擴展

PTP 功能目前為佔位實作（dummy implementation）。如需完整的 PTP 支援，需要：
1. 實作硬體時間戳
2. 實作 PTP 時鐘同步邏輯
3. 添加適當的中斷處理

## 變更歷史

- 2026-01-30: 初始重構 - 將 PTP 功能分離到獨立模組

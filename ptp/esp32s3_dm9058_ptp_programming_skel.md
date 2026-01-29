# ESP32-S3 DM9051 PTP 程式設計骨架分析

**文件建立日期**: 2026年1月28日  
**專案路徑**: `c:\Users\joseph\esp-w05\v5.5.2\test05\developer\esp_idf\ptp`

---

## 一、配置巨集出現頻率分析

### 1.1 `CONFIG_ETH_USE_ESP32_EMAC` 統計

**總出現次數**: 20 次（可能還有更多）

#### 檔案分布樹狀圖

```
ptp/
├── main/
│   └── ptp_main.c (7 次)
│       ├── L17:  #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
│       ├── L65:  #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
│       ├── L77:  #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│       ├── L101: #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || 1
│       ├── L103: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│       ├── L111: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│       └── L166: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│
├── components/
│   ├── esp_eth_time/
│   │   ├── esp_eth_time.c (8 次)
│   │   │   ├── L10:  #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
│   │   │   ├── L33:  #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│   │   │   ├── L71:  #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│   │   │   ├── L118: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│   │   │   ├── L161: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│   │   │   ├── L192: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│   │   │   ├── L216: #if defined(CONFIG_ETH_USE_ESP32_EMAC)
│   │   │   └── L240: #endif // CONFIG_ETH_USE_ESP32_EMAC || CONFIG_ETH_USE_ESP32_DM9051_PTP
│   │   │
│   │   └── esp_eth_time.h (2 次)
│   │       ├── L14:  #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
│   │       └── L130: #endif // CONFIG_ETH_USE_ESP32_EMAC || CONFIG_ETH_USE_ESP32_DM9051_PTP
│   │
│   └── ptpd/
│       ├── ptpd.c (1 次)
│       │   └── L29: #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
│       │
│       └── CMakeLists.txt (1 次)
│           └── L2:  if(CONFIG_ETH_USE_ESP32_EMAC)
│
└── logs/
    ├── sdkconfig__0_esp32p4_use_emac (1 次)
    │   └── L1204: CONFIG_ETH_USE_ESP32_EMAC=y
    │
    └── info_4.txt (1 次)
        └── L8: "Wrapped PTP code conditionally... CONFIG_ETH_USE_ESP32_EMAC"
```

#### 檔案統計摘要

| 檔案 | 出現次數 |
|------|---------|
| main/ptp_main.c | 7 |
| components/esp_eth_time/esp_eth_time.c | 8 |
| components/esp_eth_time/esp_eth_time.h | 2 |
| components/ptpd/ptpd.c | 1 |
| components/ptpd/CMakeLists.txt | 1 |
| logs/sdkconfig__0_esp32p4_use_emac | 1 |
| logs/info_4.txt | 1 |

---

### 1.2 `CONFIG_ETH_USE_ESP32_DM9051_PTP` 統計

**總出現次數**: 搜尋結果中與 `CONFIG_ETH_USE_ESP32_EMAC` 共同出現多次

主要出現位置：
- main/ptp_main.c: 多處 `#elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)` 條件編譯
- components/esp_eth_time/: 與 EMAC 配置共同條件編譯
- sdkconfig: L606 `CONFIG_ETH_USE_ESP32_DM9051_PTP=y` (已啟用)

---

## 二、當前配置狀態分析

### 2.1 sdkconfig 配置檢查

**檔案位置**: `sdkconfig`

#### ✅ `CONFIG_ETH_USE_ESP32_DM9051_PTP`
- **狀態**: 已啟用 (SET)
- **位置**: sdkconfig L606
- **值**: `CONFIG_ETH_USE_ESP32_DM9051_PTP=y`
- **所屬區段**: Example Configuration

```kconfig
#
# Example Configuration
#
CONFIG_ENV_GPIO_RANGE_MIN=0
CONFIG_ENV_GPIO_RANGE_MAX=48
CONFIG_ENV_GPIO_IN_RANGE_MAX=48
CONFIG_ENV_GPIO_OUT_RANGE_MAX=48
CONFIG_EXAMPLE_PTP_PULSE_GPIO=20
CONFIG_EXAMPLE_PTP_PULSE_WIDTH_NS=500000000
CONFIG_ETH_USE_ESP32_DM9051_PTP=y    ← 已啟用
# end of Example Configuration
```

#### ❌ `CONFIG_ETH_USE_ESP32_EMAC`
- **狀態**: 未啟用 (NOT SET)
- **備註**: 在 sdkconfig 中未找到此配置項，表示未被定義或被明確設為 not set

---

## 三、編譯與執行路徑分析

### 3.1 條件編譯邏輯

基於當前配置 (`CONFIG_ETH_USE_ESP32_DM9051_PTP=y`, `CONFIG_ETH_USE_ESP32_EMAC` 未設定)：

#### 會被編譯的程式碼區塊：

1. **OR 條件** - 任一條件滿足即編譯
   ```c
   #if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
   // ✅ 此區塊會被編譯 (因為 DM9051_PTP 已定義)
   #endif
   ```

2. **DM9051 專屬分支**
   ```c
   #elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
   // ✅ 此區塊會被編譯
   #endif
   ```

#### 不會被編譯的程式碼區塊：

1. **EMAC 專屬分支**
   ```c
   #if defined(CONFIG_ETH_USE_ESP32_EMAC)
   // ❌ 此區塊不會被編譯 (因為 EMAC 未定義)
   #endif
   ```

---

### 3.2 主要功能模組編譯狀態

| 模組/功能 | 編譯狀態 | 備註 |
|----------|---------|------|
| esp_eth_time.h 引入 | ✅ 編譯 | L17 OR 條件滿足 |
| ts_callback() | ✅ 編譯 | L65 OR 條件滿足 |
| ts_callback() 內 EMAC 邏輯 | ❌ 不編譯 | L77 EMAC 專屬 |
| ts_callback() 內 DM9051 邏輯 | ✅ 編譯 (註解) | L84 DM9051 分支，但為 TODO |
| app_main() PTP 主邏輯 | ✅ 編譯 | L101 包含 `|| 1` 強制編譯 |
| ptpd_start() 呼叫 | ❌ 不編譯 | L103 EMAC 專屬 |
| DM9051 PTP daemon 啟動 | ✅ 編譯 (註解) | L105 DM9051 分支，pid = -1 |
| EMAC 時鐘初始化 | ❌ 不編譯 | L111 EMAC 專屬 |
| DM9051 時鐘初始化 | ✅ 編譯 | L118 DM9051 分支 |
| GPIO 脈衝輸出初始化 | ✅ 編譯 | 無條件編譯 |
| EMAC 時間同步邏輯 | ❌ 不編譯 | L166 EMAC 專屬 |
| DM9051 時間同步邏輯 | ✅ 編譯 | L176 DM9051 分支 |

---

## 四、DM9051 PTP 實作清單

### 4.1 待實作函數 (TODO List)

以下函數在程式碼中被呼叫，但目前尚未實作（均為註解或 placeholder）：

| 函數名稱 | 檔案位置 | 狀態 | 用途 |
|---------|---------|------|------|
| `esp_dm9051_clock_gettime()` | ptp_main.c:L85, L120, L177 | TODO 註解 | 獲取 DM9051 PTP 時鐘時間 |
| `esp_dm9051_clock_set_target_time()` | ptp_main.c:L88, L189 | TODO 註解 | 設定 DM9051 PTP 目標時間 |
| `esp_dm9051_clock_register_target_cb()` | ptp_main.c:L123 | 已實作呼叫 | 註冊 DM9051 時間戳回調 |
| `dm9051_ptpd_start()` | ptp_main.c:L107 | TODO 註解 | 啟動 DM9051 PTP daemon |

---

### 4.2 關鍵程式碼區段分析

#### 區段 1: ts_callback() - 時間戳中斷回調 (L65-L93)

```c
#if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
IRAM_ATTR bool ts_callback(esp_eth_mediator_t *eth, void *user_args)
{
    gpio_set_level(CONFIG_EXAMPLE_PTP_PULSE_GPIO, s_gpio_level ^= 1);
    
    // Set the next target time
    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = CONFIG_EXAMPLE_PTP_PULSE_WIDTH_NS
    };
    timespecadd(&s_next_time, &interval, &s_next_time);

    // ❌ EMAC 分支不編譯
    #if defined(CONFIG_ETH_USE_ESP32_EMAC)
    // ...
    
    // ✅ DM9051 分支會編譯 (但為 TODO)
    #elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
    // TODO: Implement DM9051 PTP clock gettime
    // esp_dm9051_clock_gettime(CLOCK_PTP_SYSTEM, &curr_time);
    // check the next time is in the future
    // if (timespeccmp(&s_next_time, &curr_time, >)) {
    //     esp_dm9051_clock_set_target_time(CLOCK_PTP_SYSTEM, &s_next_time);
    // }
    #endif

    return false;
}
#endif
```

**當前行為**: 
- GPIO 脈衝切換功能正常
- 時間戳比較與下次目標時間設定功能未實作

---

#### 區段 2: app_main() - PTP daemon 啟動 (L101-L109)

```c
#if defined(CONFIG_ETH_USE_ESP32_EMAC) || defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || 1
    int pid;
    
    // ❌ 不編譯
    #if defined(CONFIG_ETH_USE_ESP32_EMAC)
    pid = ptpd_start("ETH_0");
    
    // ✅ 編譯 (但 pid = -1)
    #elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
    // TODO: Implement DM9051 PTP daemon start
    // pid = dm9051_ptpd_start("ETH_0");
    pid = -1; // Placeholder
    #endif
```

**當前行為**: 
- `pid` 被設為 -1
- 後續使用 `pid` 的 `ptpd_status(pid, &ptp_status)` 將會失敗

---

#### 區段 3: app_main() - 時鐘初始化與回調註冊 (L111-L124)

```c
    // ❌ 不編譯
    #if defined(CONFIG_ETH_USE_ESP32_EMAC)
    struct timespec cur_time;
    while (esp_eth_clock_gettime(CLOCK_PTP_SYSTEM, &cur_time) == -1) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    esp_eth_clock_register_target_cb(CLOCK_PTP_SYSTEM, ts_callback);
    
    // ✅ 編譯
    #elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP) || defined(ASSERT_DM9_PTP)
    struct timespec cur_time; // need a placeholder to avoid warning below 
    // TODO: Implement DM9051 PTP clock initialization and callback registration
    while (esp_dm9051_clock_gettime(CLOCK_PTP_SYSTEM, &cur_time) == -1) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    esp_dm9051_clock_register_target_cb(CLOCK_PTP_SYSTEM, ts_callback);
    #endif
```

**當前行為**: 
- `esp_dm9051_clock_gettime()` 未定義，編譯會失敗
- `esp_dm9051_clock_register_target_cb()` 未定義，編譯會失敗

---

#### 區段 4: app_main() - PTP 時間同步主迴圈 (L166-L189)

```c
        if ((clock_source_valid == true && clock_source_valid_last == false) || first_pass) {
            first_pass = false;
            
            // ❌ 不編譯
            #if defined(CONFIG_ETH_USE_ESP32_EMAC)
            esp_eth_clock_gettime(CLOCK_PTP_SYSTEM, &cur_time);
            // ... EMAC 邏輯 ...
            esp_eth_clock_set_target_time(CLOCK_PTP_SYSTEM, &s_next_time);
            
            // ✅ 編譯
            #elif defined(CONFIG_ETH_USE_ESP32_DM9051_PTP)
            // TODO: Implement DM9051 PTP time synchronization
            esp_dm9051_clock_gettime(CLOCK_PTP_SYSTEM, &cur_time);
            s_next_time.tv_sec = 1;
            timespecadd(&s_next_time, &cur_time, &s_next_time);
            s_next_time.tv_nsec = CONFIG_EXAMPLE_PTP_PULSE_WIDTH_NS;
            ESP_LOGI(TAG, "Starting Pulse train (DM9051)");
            ESP_LOGI(TAG, "curr time: %llu.%09lu", cur_time.tv_sec, cur_time.tv_nsec);
            ESP_LOGI(TAG, "next time: %llu.%09lu", s_next_time.tv_sec, s_next_time.tv_nsec);
            s_gpio_level = 0;
            gpio_set_level(CONFIG_EXAMPLE_PTP_PULSE_GPIO, s_gpio_level);
            esp_dm9051_clock_set_target_time(CLOCK_PTP_SYSTEM, &s_next_time);
            #endif
        }
```

**當前行為**: 
- DM9051 分支的時間同步邏輯已編寫完整
- 但依賴未實作的函數，編譯會失敗

---

## 五、編譯錯誤預測

### 5.1 預期的編譯錯誤

基於當前配置，編譯時會遇到以下未定義符號錯誤：

```
undefined reference to `esp_dm9051_clock_gettime'
undefined reference to `esp_dm9051_clock_register_target_cb'
undefined reference to `esp_dm9051_clock_set_target_time'
```

### 5.2 需要實作的模組

1. **esp_dm9051_time 模組** (類比 esp_eth_time)
   - 需要實作 DM9051 硬體的 PTP 時鐘存取功能
   - 可能需要在 `components/esp_eth_time/` 中添加 DM9051 相關實作
   - 或創建新的 `components/esp_dm9051_time/` 模組

2. **DM9051 PTP daemon**
   - 實作 `dm9051_ptpd_start()` 函數
   - 可能需要修改 `components/ptpd/` 以支援 DM9051

---

## 六、開發建議與後續步驟

### 6.1 短期目標 (編譯通過)

1. **建立 esp_dm9051_time 標頭檔**
   - 定義 DM9051 PTP API 函數原型
   - 暫時提供 stub 實作以通過編譯

2. **修改 ptpd 元件**
   - 添加 DM9051 支援分支
   - 實作或 stub `dm9051_ptpd_start()`

3. **條件編譯調整**
   - 考慮將 L101 的 `|| 1` 移除或調整邏輯
   - 確保 PTP 功能只在配置正確時啟用

### 6.2 中期目標 (功能實作)

1. **DM9051 硬體 PTP 暫存器存取**
   - 研究 DM9051 晶片的 PTP 暫存器規格
   - 實作 SPI 讀寫 PTP 暫存器的函數

2. **時鐘同步機制**
   - 實作 `esp_dm9051_clock_gettime()`
   - 實作 `esp_dm9051_clock_set_target_time()`
   - 實作中斷回調機制

3. **測試與驗證**
   - PTP 時間戳精度測試
   - GPIO 脈衝輸出時序驗證
   - 與標準 PTP master/slave 互通性測試

### 6.3 長期目標 (優化與文檔)

1. **效能優化**
   - 減少 PTP 同步延遲
   - 優化中斷處理效率

2. **文檔完善**
   - API 使用說明
   - 硬體接線指南
   - 配置選項說明

3. **範例程式**
   - PTP master 模式範例
   - PTP slave 模式範例
   - 多網口同步範例

---

## 七、參考資料

### 7.1 相關檔案清單

| 檔案路徑 | 說明 |
|---------|------|
| main/ptp_main.c | 主程式，包含 PTP 應用邏輯 |
| components/esp_eth_time/ | ESP32 EMAC PTP 時鐘實作 (可作為參考) |
| components/ptpd/ | PTP daemon 實作 |
| sdkconfig | 專案配置檔 (當前啟用 DM9051_PTP) |
| logs/GOOD_LOG_10_20260126.txt | 可能的成功執行記錄 |

### 7.2 配置選項

| 選項 | 當前值 | 說明 |
|------|--------|------|
| CONFIG_ETH_USE_ESP32_DM9051_PTP | y | 啟用 DM9051 PTP 支援 |
| CONFIG_ETH_USE_ESP32_EMAC | (未設定) | 未使用 ESP32 內建 EMAC |
| CONFIG_EXAMPLE_PTP_PULSE_GPIO | 20 | PTP 脈衝輸出 GPIO |
| CONFIG_EXAMPLE_PTP_PULSE_WIDTH_NS | 500000000 | 脈衝寬度 (500ms) |
| CONFIG_NETUTILS_PTPD | y | 啟用 PTP daemon |
| CONFIG_NETUTILS_PTPD_DOMAIN | 0 | PTP domain 編號 |

---

## 八、版本歷史

| 版本 | 日期 | 變更說明 |
|------|------|---------|
| 1.0 | 2026-01-28 | 初始文檔，基於程式碼靜態分析 |

---

**文檔結束**

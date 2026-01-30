set(ptpd_srcs "")
if(CONFIG_ETH_USE_ESP32_EMAC)
    list(APPEND ptpd_srcs "ptpd.c")
elseif(CONFIG_ETH_USE_ESP32_DM9051_PTP)
    list(APPEND ptpd_srcs "ptpd.c")
endif()

idf_component_register(SRCS "${ptpd_srcs}"
                       PRIV_REQUIRES esp_eth esp_netif
                       INCLUDE_DIRS "." "./include")

if(ptpd_srcs)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function)
endif()

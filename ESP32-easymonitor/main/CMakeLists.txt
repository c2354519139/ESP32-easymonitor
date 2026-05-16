idf_component_register(SRCS "deepseek_api.c" "binance_api.c" "wifi_sta.c" "main.c" "dht11.c" "st7735s.c" "lv_port.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_wifi esp_http_client json mbedtls lvgl)

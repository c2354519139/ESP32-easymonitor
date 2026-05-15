#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "wifi_sta.h"

static const char *TAG = "WiFi_STA";

/* 内部状态 */
static wifi_sta_status_t g_wifi_status = {0};
static esp_netif_t        *g_netif = NULL;

/* WiFi 事件回调：处理 连接断开 / 获取 IP 等 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi STA 已启动，等待连接...");
            g_wifi_status.connected = false;
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "已关联到 AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *ev =
                (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGW(TAG, "WiFi 断开，原因: %d", ev->reason);
            g_wifi_status.connected = false;
            g_wifi_status.ip[0] = '\0';
            // 自动重连
            esp_wifi_connect();
            break;
        }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        snprintf(g_wifi_status.ip, sizeof(g_wifi_status.ip),
                 IPSTR, IP2STR(&ev->ip_info.ip));
        g_wifi_status.connected = true;
        ESP_LOGI(TAG, "已获取 IP: %s", g_wifi_status.ip);

        // 获取到 IP 后启动 SNTP 时间同步
        static bool sntp_started = false;
        if (!sntp_started) {
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, "pool.ntp.org");
            esp_sntp_init();
            sntp_started = true;
            ESP_LOGI(TAG, "SNTP 时间同步已启动");
        }
    }
}

void wifi_sta_init(void)
{
    // 1. 初始化 TCP/IP 协议栈
    ESP_ERROR_CHECK(esp_netif_init());

    // 2. 创建默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. 创建默认 STA 网卡
    g_netif = esp_netif_create_default_wifi_sta();

    // 4. 初始化 WiFi 驱动（默认配置即可）
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. 注册事件回调
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                         ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                         IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    // 6. 设置工作模式为 STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 7. 启动 WiFi 网卡
    ESP_ERROR_CHECK(esp_wifi_start());

    g_wifi_status.connected = false;
    g_wifi_status.ip[0]     = '\0';
    g_wifi_status.ssid[0]   = '\0';
    g_wifi_status.rssi      = 0;

    ESP_LOGI(TAG, "WiFi STA 初始化完成");
}

void wifi_sta_connect(const char *ssid, const char *password)
{
    // 保存 SSID 到状态
    strncpy(g_wifi_status.ssid, ssid, sizeof(g_wifi_status.ssid) - 1);

    // 构造 WiFi 配置
    wifi_config_t cfg = {0};
    strncpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid) - 1);
    strncpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));

    // 开始连接（首次）或重连
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGE(TAG, "WiFi 未启动，请先调用 wifi_sta_init()");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "连接失败: %s", esp_err_to_name(err));
    }
}

wifi_sta_status_t wifi_sta_get_status(void)
{
    // 查询实时 RSSI（连接后才有效）
    if (g_wifi_status.connected) {
        esp_wifi_sta_get_rssi(&g_wifi_status.rssi);
    }
    return g_wifi_status;
}

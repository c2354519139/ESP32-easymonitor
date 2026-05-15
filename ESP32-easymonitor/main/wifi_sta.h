#ifndef _WIFI_STA_H_
#define _WIFI_STA_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ==================== 使用说明 ====================
 *
 * 1. 在 main.c 中包含此头文件：
 *       #include "wifi_sta.h"
 *
 * 2. 在 app_main() 中先初始化 NVS，再初始化 WiFi：
 *       nvs_flash_init();
 *       wifi_sta_init();  // 启动 WiFi 并注册事件回调
 *
 * 3. 调用连接函数，传入 SSID 和密码：
 *       wifi_sta_connect("你的WiFi名", "你的WiFi密码");
 *
 * 4. 在循环中获取连接状态，驱动屏幕显示：
 *       wifi_sta_status_t status = wifi_sta_get_status();
 *       // status.connected:  true=已连接, false=未连接
 *       // status.ip:        连接成功后的 IP 地址字符串
 *       // status.ssid:      正在连接的 WiFi 名称
 *
 * 5. 在 CMakeLists.txt 的 SRCS 中添加 "wifi_sta.c"
 */

/* WiFi 连接状态 */
typedef struct {
    bool connected;      // 是否已获取 IP
    char ssid[33];       // 当前 WiFi 名称（最长32字节）
    char ip[16];         // IP 地址字符串（如 "192.168.1.100"）
    int  rssi;           // 信号强度（dBm，连接后有效，范围约 -30 ~ -90）
} wifi_sta_status_t;

/**
 * @brief 初始化 WiFi STA 模式（启动网卡 + 注册事件处理）
 *        调用后 WiFi 进入就绪状态，等待 connect() 触发连接。
 */
void wifi_sta_init(void);

/**
 * @brief 连接到指定 WiFi 热点
 *
 * @param ssid     WiFi 名称（字符串，最长 32 字节）
 * @param password WiFi 密码（字符串，最长 63 字节）
 */
void wifi_sta_connect(const char *ssid, const char *password);

/**
 * @brief 获取当前 WiFi 连接状态，供屏幕或其他模块查询。
 *
 * @return wifi_sta_status_t 结构体，包含连接状态、SSID、IP、信号强度。
 */
wifi_sta_status_t wifi_sta_get_status(void);

#ifdef __cplusplus
}
#endif

#endif

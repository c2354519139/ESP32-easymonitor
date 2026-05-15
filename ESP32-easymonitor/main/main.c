#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <driver/rmt_rx.h>
#include <driver/rmt_tx.h>
#include <soc/rmt_reg.h>
#include "driver/gpio.h"
#include <esp_log.h>
#include <freertos/queue.h>
#include "rom/ets_sys.h"
#include "dht11.h"
#include "st7735s.h"
#include "wifi_sta.h"
#include "deepseek_api.h"
#include "binance_api.h"
#include "secrets.h"

#define DHT11_GPIO	45
#define ST7735S_ROTATE_MODE ST7735S_ROTATE_0

// 密钥等敏感配置在 secrets.h 中（已 gitignore）
// secrets.h 模板见 secrets.h.example

// ==================== 行情 ====================
#define CRYPTO_SYMBOL "BTC"  // Binance symbol

const static char *TAG = "DHT11_Demo";

int temp = 0, hum = 0;

/* 余额查询结果（balance_task 写入，主循环读取） */
static char g_balance_str[32] = "...";

/* 币安价格（price_task 写入，主循环读取） */
static float g_price = 0.0f;
static float g_price_prev = 0.0f;
static float g_change_pct = 0.0f;
static bool  g_price_valid = false;

/*
 * 竖屏模式（128×160），6×8 小字体，最多 21 字符/行
 * 注：显示屏 Y 轴反转（y=0 在底部，y=159 在顶部）
 * 屏幕布局（ROTATE_0）：
 *   y=152: 实时时钟（顶部）
 *   y=140: WiFi 状态
 *   y=128: IP 地址
 *   y=116: 余额
 *   y=104: BTC/USDT 价格
 *   y=92:  24h 涨跌幅
 */

static void balance_task(void *arg)
{
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(15000));
		wifi_sta_status_t ws = wifi_sta_get_status();
		if (!ws.connected) {
			snprintf(g_balance_str, sizeof(g_balance_str), "...");
			continue;
		}

		char bal_str[32];
		char bal_err[64];
		if (deepseek_get_balance(DEEPSEEK_API_KEY,
		                         bal_str, sizeof(bal_str),
		                         bal_err, sizeof(bal_err)) == 0) {
			snprintf(g_balance_str, sizeof(g_balance_str), "%s", bal_str);
			ESP_LOGI(TAG, "Balance: %s", g_balance_str);
		} else {
			ESP_LOGW(TAG, "Balance err: %s", bal_err);
			snprintf(g_balance_str, sizeof(g_balance_str), "err");
		}
	}
}

static void price_task(void *arg)
{
	while (1) {
		wifi_sta_status_t ws = wifi_sta_get_status();
		if (!ws.connected) {
			g_price_valid = false;
			vTaskDelay(pdMS_TO_TICKS(5000));
			continue;
		}

		char price_str[32];
		float change_pct = 0.0f;
		char err_str[64];
		if (binance_get_price(CRYPTO_SYMBOL,
		                      price_str, sizeof(price_str),
		                      &change_pct,
		                      err_str, sizeof(err_str)) == 0) {
			float new_price = atof(price_str);
			if (new_price > 0) {
				g_price_prev = g_price;
				g_price = new_price;
				g_change_pct = change_pct;
				g_price_valid = true;
				ESP_LOGI(TAG, "BTC: %.2f (%.2f%%)", g_price, g_change_pct);
			}
		} else {
			ESP_LOGW(TAG, "Price err: %s", err_str);
		}

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void app_main(void)
{
	// 设置时区为中国标准时间 (UTC+8)
	setenv("TZ", "CST-8", 1);
	tzset();

	ESP_ERROR_CHECK(nvs_flash_init());
	vTaskDelay(100 / portTICK_PERIOD_MS);

	ESP_LOGI(TAG, "[APP] APP Is Start!~\r\n");
	ESP_LOGI(TAG, "[APP] IDF Version is %d.%d.%d",
	         ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
	ESP_LOGI(TAG, "[APP] Free memory: %lu bytes", esp_get_free_heap_size());

	DHT11_Init(DHT11_GPIO);

	wifi_sta_init();
	wifi_sta_connect(WIFI_SSID, WIFI_PASSWORD);

	ESP_ERROR_CHECK(st7735s_init(ST7735S_ROTATE_MODE));
	st7735s_clear(ST7735S_BLUE);
	vTaskDelay(pdMS_TO_TICKS(1000));
	st7735s_clear(ST7735S_WHITE);
	st7735s_set_backlight(true);

	xTaskCreate(balance_task, "balance", 4096, NULL, 1, NULL);
	xTaskCreate(price_task, "price", 4096, NULL, 1, NULL);

	static int  update_tick = 0;

	while (1) {
		char buf[64];
		wifi_sta_status_t ws = wifi_sta_get_status();
		update_tick++;

		/* 实时时钟（行 0） */
		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);
		if (timeinfo.tm_year >= 126) {  // 2026+
			strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
		} else {
			snprintf(buf, sizeof(buf), "--:--:--");
		}
		// 秒数闪烁冒号（500ms 循环，&2 每1秒翻转）
		if (update_tick & 2) {
			char *p = strchr(buf, ':');
			if (p) *p = ' ';
			p = strrchr(buf, ':');
			if (p) *p = ' ';
		}
		st7735s_show_string(4, 150, buf,
		                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);

		if (ws.connected) {
			/* WiFi 状态（行 10） */
			st7735s_show_string(4, 140, "WiFi OK",
			                    ST7735S_GREEN, ST7735S_WHITE, ST7735S_FONT_6X8);

			/* IP 地址（行 20） */
			snprintf(buf, sizeof(buf), "IP %s ", ws.ip);
			st7735s_show_string(4, 128, buf,
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);

			/* 余额（行 30） */
			snprintf(buf, sizeof(buf), "Bal %s", g_balance_str);
			st7735s_show_string(4, 116, buf,
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);

			/* BTC/USDT 价格 */
			if (g_price_valid && g_price > 0) {
				uint16_t price_color;
				if (g_price_prev > 0) {
					if (g_price > g_price_prev)
						price_color = ST7735S_GREEN;
					else if (g_price < g_price_prev)
						price_color = ST7735S_RED;
					else
						price_color = ST7735S_BLACK;
				} else {
					price_color = ST7735S_BLACK;
				}
				const char *dot = (update_tick & 2) ? "." : " ";
				snprintf(buf, sizeof(buf), "BTC %.2f%s",
				         g_price, dot);
				st7735s_show_string(4, 104, buf,
				                    price_color, ST7735S_WHITE, ST7735S_FONT_6X8);

				/* 24h 涨跌幅 */
				uint16_t chg_color = (g_change_pct >= 0) ? ST7735S_RED : ST7735S_GREEN;
				snprintf(buf, sizeof(buf), "24h %+.2f%%", g_change_pct);
				st7735s_show_string(4, 92, buf,
				                    chg_color, ST7735S_WHITE, ST7735S_FONT_6X8);
			} else {
				snprintf(buf, sizeof(buf), "BTC/USDT ...");
				st7735s_show_string(4, 104, buf,
				                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
				st7735s_show_string(4, 92, "                     ",
				                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
			}
		} else {
			st7735s_show_string(4, 140, "WiFi waiting...",
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
			st7735s_show_string(4, 128, "                     ",
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
			st7735s_show_string(4, 116, "                     ",
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
			st7735s_show_string(4, 104, "                     ",
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
			st7735s_show_string(4, 92, "                     ",
			                    ST7735S_BLACK, ST7735S_WHITE, ST7735S_FONT_6X8);
			snprintf(g_balance_str, sizeof(g_balance_str), "...");
			g_price_valid = false;
		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

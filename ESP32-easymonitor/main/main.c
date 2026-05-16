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
#include "lvgl.h"
#include "dht11.h"
#include "st7735s.h"
#include "wifi_sta.h"
#include "deepseek_api.h"
#include "binance_api.h"
#include "lv_port.h"
#include "secrets.h"

#define DHT11_GPIO	45
#define ST7735S_ROTATE_MODE ST7735S_ROTATE_0

// ==================== 行情 ====================
#define CRYPTO_SYMBOL "BTC"

const static char *TAG = "DHT11_Demo";

int temp = 0, hum = 0;

/* 余额查询结果（balance_task 写入，主循环读取） */
static char g_balance_str[32] = "...";

/* 币安价格（price_task 写入，主循环读取） */
static float g_price = 0.0f;
static float g_price_prev = 0.0f;
static float g_change_pct = 0.0f;
static bool  g_price_valid = false;

/* LVGL 对象 */
static lv_obj_t *label_clock;
static lv_obj_t *label_wifi;
static lv_obj_t *label_ip;
static lv_obj_t *label_balance;
static lv_obj_t *label_price;
static lv_obj_t *label_change;
static lv_obj_t *screen;

/* 颜色样式 */
static lv_style_t style_black;
static lv_style_t style_red;
static lv_style_t style_green;

/* 全局样式 bg */
static lv_style_t style_bg;

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

/**
 * @brief 加密货币价格获取任务
 * 
 * 该任务定期从币安API获取指定加密货币（BTC）的实时价格和24小时涨跌幅。
 * 任务执行流程：
 * 1. 检查WiFi连接状态，未连接时等待5秒后重试
 * 2. 连接成功后调用binance_get_price获取价格数据
 * 3. 解析价格字符串并更新全局价格变量
 * 4. 设置价格有效标志并记录日志
 * 5. 等待2秒后进入下一轮循环
 * 
 * @param arg 任务参数（未使用）
 */
static void price_task(void *arg)
{
	/* 无限循环，持续获取价格 */
	while (1) {
		/* 获取当前WiFi连接状态 */
		wifi_sta_status_t ws = wifi_sta_get_status();
		
		/* WiFi未连接时，标记价格无效并等待5秒重试 */
		if (!ws.connected) {
			g_price_valid = false;  /* 设置价格无效标志 */
			vTaskDelay(pdMS_TO_TICKS(5000));  /* 等待5秒 */
			continue;  /* 跳过本轮，重新检查连接 */
		}

		/* 定义局部变量 */
		char price_str[32];      /* 存储价格字符串 */
		float change_pct = 0.0f; /* 24小时涨跌幅 */
		char err_str[64];        /* 错误信息缓冲区 */
		
		/* 调用币安API获取价格 */
		if (binance_get_price(CRYPTO_SYMBOL,       /* 加密货币符号（BTC） */
		                      price_str, sizeof(price_str),  /* 价格字符串输出缓冲区 */
		                      &change_pct,           /* 涨跌幅输出指针 */
		                      err_str, sizeof(err_str)) == 0) {  /* 错误信息输出缓冲区 */
			
			/* 将价格字符串转换为浮点数值 */
			float new_price = atof(price_str);
			
			/* 验证价格有效性 */
			if (new_price > 0) {
				g_price_prev = g_price;    /* 保存旧价格用于比较 */
				g_price = new_price;       /* 更新当前价格 */
				g_change_pct = change_pct; /* 更新涨跌幅 */
				g_price_valid = true;      /* 标记价格有效 */
				
				/* 记录日志：当前BTC价格和涨跌幅 */
				ESP_LOGI(TAG, "BTC: %.2f (%.2f%%)", g_price, g_change_pct);
			}
		} else {
			/* API调用失败，记录警告日志 */
			ESP_LOGW(TAG, "Price err: %s", err_str);
		}

		/* 等待2秒后再次获取价格 */
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

/*
 * 竖屏 128×160，面板 y=0 在底部，LVGL 透传坐标，flush 里做 Y 翻转。
 * 布局（LVGL y，0=顶部）：
 *   y=3:   实时时钟
 *   y=25:  WiFi 状态
 *   y=50:  IP 地址
 *   y=75:  余额
 *   y=100: BTC/USDT 价格
 *   y=125: 24h 涨跌幅
 */

static void ui_init(void)
{
	/* 白色背景 */
	lv_style_init(&style_bg);
	lv_style_set_bg_color(&style_bg, lv_color_white());
	lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
	lv_style_set_text_color(&style_bg, lv_color_black());

	/* 文字样式 */
	lv_style_init(&style_black);
	lv_style_set_text_color(&style_black, lv_color_black());

	lv_style_init(&style_red);
	lv_style_set_text_color(&style_red, lv_color_hex(0xF80000));  // RED

	lv_style_init(&style_green);
	lv_style_set_text_color(&style_green, lv_color_hex(0x00A000));  // GREEN

	screen = lv_obj_create(NULL);
	lv_obj_add_style(screen, &style_bg, 0);

	label_clock   = lv_label_create(screen);
	label_wifi    = lv_label_create(screen);
	label_ip      = lv_label_create(screen);
	label_balance = lv_label_create(screen);
	label_price   = lv_label_create(screen);
	label_change  = lv_label_create(screen);

	lv_obj_set_pos(label_clock,   5, 3);
	lv_obj_set_pos(label_wifi,    5, 25);
	lv_obj_set_pos(label_ip,      5, 50);
	lv_obj_set_pos(label_balance, 5, 75);
	lv_obj_set_pos(label_price,   5, 100);
	lv_obj_set_pos(label_change,  5, 125);

	lv_label_set_text(label_clock,   "--:--:--");
	lv_label_set_text(label_wifi,    "WiFi waiting...");
	lv_label_set_text(label_ip,      "");
	lv_label_set_text(label_balance, "");
	lv_label_set_text(label_price,   "BTC/USDT ...");
	lv_label_set_text(label_change,  "");

	lv_scr_load(screen);
}

void app_main(void)
{
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

	lv_port_init();
	ui_init();

	xTaskCreate(balance_task, "balance", 4096, NULL, 1, NULL);
	xTaskCreate(price_task, "price", 4096, NULL, 1, NULL);

	static int update_tick = 0;

	while (1) {
		char buf[64];
		wifi_sta_status_t ws = wifi_sta_get_status();
		update_tick++;

		/* 实时时钟 */
		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);
		if (timeinfo.tm_year >= 126) {
			strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
		} else {
			snprintf(buf, sizeof(buf), "--:--:--");
		}
		if (update_tick & 2) {
			char *p = strchr(buf, ':');
			if (p) *p = ' ';
			p = strrchr(buf, ':');
			if (p) *p = ' ';
		}
		lv_label_set_text(label_clock, buf);

		if (ws.connected) {
			lv_obj_set_style_text_color(label_wifi, lv_color_hex(0x00A000), 0);
			lv_label_set_text(label_wifi, "WiFi OK");

			snprintf(buf, sizeof(buf), "IP %s", ws.ip);
			lv_label_set_text(label_ip, buf);

			snprintf(buf, sizeof(buf), "Bal %s", g_balance_str);
			lv_label_set_text(label_balance, buf);

			if (g_price_valid && g_price > 0) {
				lv_color_t price_color;
				if (g_price_prev > 0) {
					if (g_price > g_price_prev)
						price_color = lv_color_hex(0x00A000);
					else if (g_price < g_price_prev)
						price_color = lv_color_hex(0xF80000);
					else
						price_color = lv_color_black();
				} else {
					price_color = lv_color_black();
				}
				const char *dot = (update_tick & 2) ? "." : " ";
				snprintf(buf, sizeof(buf), "BTC %.2f%s",
				         g_price, dot);
				lv_obj_set_style_text_color(label_price, price_color, 0);
				lv_label_set_text(label_price, buf);

				lv_color_t chg_color = (g_change_pct >= 0) ? lv_color_hex(0xF80000) : lv_color_hex(0x00A000);
				snprintf(buf, sizeof(buf), "24h %+.2f%%", g_change_pct);
				lv_obj_set_style_text_color(label_change, chg_color, 0);
				lv_label_set_text(label_change, buf);
			} else {
				lv_obj_set_style_text_color(label_price, lv_color_black(), 0);
				lv_label_set_text(label_price, "BTC/USDT ...");
				lv_label_set_text(label_change, "");
			}
		} else {
			lv_obj_set_style_text_color(label_wifi, lv_color_black(), 0);
			lv_label_set_text(label_wifi, "WiFi waiting...");
			lv_label_set_text(label_ip, "");
			lv_label_set_text(label_balance, "");
			lv_label_set_text(label_price, "BTC/USDT ...");
			lv_label_set_text(label_change, "");
			snprintf(g_balance_str, sizeof(g_balance_str), "...");
			g_price_valid = false;
		}

		lv_task_handler();
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
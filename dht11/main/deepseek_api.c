#include <string.h>
#include <stdio.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"
#include "deepseek_api.h"

static const char *TAG = "DeepSeek";

/* 接收缓冲区 */
static char  g_recv_buf[1024];
static int   g_recv_len = 0;

/* HTTP 事件回调：将服务器返回的数据拼接到 g_recv_buf */
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (g_recv_len + evt->data_len < sizeof(g_recv_buf) - 1) {
            memcpy(g_recv_buf + g_recv_len, evt->data, evt->data_len);
            g_recv_len += evt->data_len;
            g_recv_buf[g_recv_len] = '\0';
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

int deepseek_get_balance(const char *api_key, char *balance, int bal_len,
                         char *error_info, int err_len)
{
    if (!api_key || !balance || !error_info) return -1;
    balance[0]    = '\0';
    error_info[0] = '\0';

    // 1. 清空接收缓冲区
    g_recv_len = 0;
    memset(g_recv_buf, 0, sizeof(g_recv_buf));

    // 2. 配置 HTTPS 客户端
    esp_http_client_config_t cfg = {
        .url = "https://api.deepseek.com/user/balance",
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .skip_cert_common_name_check = true,
        .timeout_ms = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        snprintf(error_info, err_len, "init fail");
        return -1;
    }

    // 3. 设置请求头
    char auth[128];
    snprintf(auth, sizeof(auth), "Bearer %s", api_key);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Accept", "application/json");

    // 4. 发起请求
    esp_err_t err = esp_http_client_perform(client);
    int status = 0;
    if (err == ESP_OK) {
        status = esp_http_client_get_status_code(client);
    }

    ESP_LOGI(TAG, "HTTP err=%d status=%d", err, status);

    if (err != ESP_OK) {
        snprintf(error_info, err_len, "HTTP:%s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return -1;
    }
    if (status != 200) {
        snprintf(error_info, err_len, "HTTP %d", status);
        esp_http_client_cleanup(client);
        return -1;
    }

    // 5. 解析 JSON
    cJSON *root = cJSON_Parse(g_recv_buf);
    if (!root) {
        snprintf(error_info, err_len, "JSON err");
        esp_http_client_cleanup(client);
        return -1;
    }

    // 检查 is_available
    cJSON *avail = cJSON_GetObjectItem(root, "is_available");
    if (!avail || !cJSON_IsTrue(avail)) {
        snprintf(error_info, err_len, "not avail");
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return -1;
    }

    // 提取第一个 balance_info 的 total_balance 和 currency
    cJSON *infos = cJSON_GetObjectItem(root, "balance_infos");
    if (!infos || !cJSON_IsArray(infos) || cJSON_GetArraySize(infos) == 0) {
        snprintf(error_info, err_len, "no info");
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return -1;
    }

    cJSON *info     = cJSON_GetArrayItem(infos, 0);
    cJSON *amount   = cJSON_GetObjectItem(info, "total_balance");
    cJSON *currency = cJSON_GetObjectItem(info, "currency");

    if (amount && cJSON_IsString(amount)) {
        snprintf(balance, bal_len, "%s %s",
                 amount->valuestring,
                 (currency && cJSON_IsString(currency)) ? currency->valuestring : "CNY");
    } else {
        snprintf(error_info, err_len, "no amount");
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return -1;
    }

    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    return 0;
}

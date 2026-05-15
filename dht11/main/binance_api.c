#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "binance_api.h"

static char  g_recv_buf[1024];
static int   g_recv_len = 0;

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

int binance_get_price(const char *symbol, char *price, int price_len,
                      float *change_percent,
                      char *error_info, int err_len)
{
    if (!symbol || !price || !error_info) return -1;
    price[0]    = '\0';
    if (change_percent) *change_percent = 0.0f;
    error_info[0] = '\0';

    g_recv_len = 0;
    memset(g_recv_buf, 0, sizeof(g_recv_buf));

    char url[256];
    snprintf(url, sizeof(url),
             "https://binance-proxy.cyjokok.top/binance/api/v3/ticker/24hr?symbol=%sUSDT",
             symbol);

    esp_http_client_config_t cfg = {
        .url = url,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 8000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        snprintf(error_info, err_len, "init fail");
        return -1;
    }

    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_perform(client);
    int status = 0;
    if (err == ESP_OK) {
        status = esp_http_client_get_status_code(client);
    }

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

    cJSON *root = cJSON_Parse(g_recv_buf);
    if (!root) {
        snprintf(error_info, err_len, "JSON err");
        esp_http_client_cleanup(client);
        return -1;
    }

    cJSON *p = cJSON_GetObjectItem(root, "lastPrice");
    if (p && cJSON_IsString(p)) {
        snprintf(price, price_len, "%s", p->valuestring);
    } else {
        snprintf(error_info, err_len, "no price");
        cJSON_Delete(root);
        esp_http_client_cleanup(client);
        return -1;
    }

    if (change_percent) {
        cJSON *cp = cJSON_GetObjectItem(root, "priceChangePercent");
        if (cp && cJSON_IsString(cp)) {
            *change_percent = atof(cp->valuestring);
        }
    }

    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    return 0;
}

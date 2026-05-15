#ifndef _BINANCE_API_H_
#define _BINANCE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

int binance_get_price(const char *symbol, char *price, int price_len,
                      float *change_percent,
                      char *error_info, int err_len);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _DEEPSEEK_API_H_
#define _DEEPSEEK_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 查询 DeepSeek 账户余额（阻塞调用，需在 WiFi 已连接后使用）
 *
 * @param api_key    DeepSeek API Key（以 sk- 开头的字符串）
 * @param balance    输出缓冲区，存放余额字符串，如 "10.50 CNY"
 * @param bal_len    balance 缓冲区长度
 * @param error_info 输出缓冲区，存放错误信息，成功时为空字符串
 * @param err_len    error_info 缓冲区长度
 * @return 0=成功, -1=失败
 */
int deepseek_get_balance(const char *api_key, char *balance, int bal_len,
                         char *error_info, int err_len);

#ifdef __cplusplus
}
#endif

#endif

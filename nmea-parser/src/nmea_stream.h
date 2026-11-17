/**
 * ============================================================
 * nmea_stream.h — NMEA 流式装配器（解决串口分包/粘包）
 * ============================================================
 * 为什么需要它：
 *   UART 是按字节来的。一次中断可能只收到半个句子，
 *   也可能一口气收到三条半。nmea_parse() 要求"完整句子"，
 *   中间这一层就是流式装配。
 *
 * 设计：
 *   - 零 malloc：固定 128 字节缓冲（NMEA 规范最长 82 字节）
 *   - 状态机：等待 '$' → 收集 → 遇到 \r 或 \n 结束
 *   - 溢出保护：超过 127 字节直接丢弃，等下一个 '$'
 *   - 回调：每拼出一条完整句子就回调一次
 *
 * 用法：
 *   nmea_stream_t st;
 *   nmea_stream_init(&st, on_sentence, NULL);
 *   // 串口中断里：
 *   nmea_stream_feed(&st, byte);
 * ============================================================
 */

#ifndef NMEA_STREAM_H
#define NMEA_STREAM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 每装配出一条完整句子（不含 \r\n）就回调一次。 */
typedef void (*nmea_stream_cb)(const char *sentence, void *user);

typedef struct {
    char     buf[128];
    uint16_t len;
    uint8_t  active;      /* 1 = 正在收集一条句子 */
    uint16_t dropped;     /* 溢出丢弃的句子数（调试用） */
    uint16_t delivered;   /* 成功交付的句子数（调试用） */
    nmea_stream_cb cb;
    void    *user;
} nmea_stream_t;

void nmea_stream_init(nmea_stream_t *st, nmea_stream_cb cb, void *user);
void nmea_stream_feed(nmea_stream_t *st, uint8_t byte);
void nmea_stream_feed_bytes(nmea_stream_t *st, const uint8_t *data, uint32_t len);
void nmea_stream_reset(nmea_stream_t *st);

#ifdef __cplusplus
}
#endif

#endif /* NMEA_STREAM_H */

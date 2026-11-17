/**
 * ============================================================
 * nmea_stream.c — NMEA 流式装配器实现
 * ============================================================
 * 逐字节喂入，拼出完整句子后回调。ISR 安全（无动态分配、无阻塞）。
 * ============================================================
 */

#include "nmea_stream.h"

#include <string.h>

void nmea_stream_init(nmea_stream_t *st, nmea_stream_cb cb, void *user)
{
    if (st == 0) {
        return;
    }
    memset(st, 0, sizeof(*st));
    st->cb = cb;
    st->user = user;
}

void nmea_stream_reset(nmea_stream_t *st)
{
    if (st == 0) {
        return;
    }
    st->len = 0;
    st->active = 0;
}

void nmea_stream_feed(nmea_stream_t *st, uint8_t byte)
{
    if (st == 0) {
        return;
    }

    /* 任何位置出现 '$' 都视为新句子开始（丢弃残句，重新同步） */
    if (byte == '$') {
        st->len = 0;
        st->buf[st->len++] = '$';
        st->active = 1;
        return;
    }

    if (!st->active) {
        return;                 /* 还没看到 '$'，噪声直接丢 */
    }

    if (byte == '\r' || byte == '\n') {
        if (st->len >= 2) {     /* 至少 "$x" 才值得交付 */
            st->buf[st->len] = '\0';
            st->delivered++;
            if (st->cb != 0) {
                st->cb(st->buf, st->user);
            }
        }
        st->len = 0;
        st->active = 0;
        return;
    }

    if (st->len < sizeof(st->buf) - 1) {
        st->buf[st->len++] = (char)byte;
    } else {
        /* 超长：丢弃这一条，等下一个 '$'，避免缓冲区越界 */
        st->dropped++;
        st->len = 0;
        st->active = 0;
    }
}

void nmea_stream_feed_bytes(nmea_stream_t *st, const uint8_t *data, uint32_t len)
{
    if (st == 0 || data == 0) {
        return;
    }
    for (uint32_t i = 0; i < len; i++) {
        nmea_stream_feed(st, data[i]);
    }
}

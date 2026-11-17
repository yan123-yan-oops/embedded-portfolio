/**
 * ============================================================
 * stream_test.c — 流式装配器测试
 * ============================================================
 * 用三种喂法读 sample_track.nmea，结果必须一致：
 *   1) 整块喂入（模拟"一次收全"）
 *   2) 逐字节喂入（模拟"最坏分包"，每字节一个中断）
 *   3) 随机块长喂入（模拟真实串口抖动）
 *
 * 判定标准：三种方式装配出的句子数相同，且全部解析成功（CRC 错 0）。
 *
 * 编译：
 *   gcc -g -Wall -Wextra -std=c11 stream_test.c nmea_stream.c nmea_parser.c -o stream_test.exe
 * ============================================================
 */

#include <stdio.h>
#include <string.h>

#include "nmea_parser.h"
#include "nmea_stream.h"

static int g_total;
static int g_ok;
static int g_crc_fail;
static int g_other_fail;

static void on_sentence(const char *sentence, void *user)
{
    (void)user;

    gps_data_t gps;
    nmea_status_t status = nmea_parse(sentence, &gps);

    g_total++;
    if (status == NMEA_OK) {
        g_ok++;
    } else if (status == NMEA_CRC_FAIL) {
        g_crc_fail++;
    } else {
        g_other_fail++;
    }
}

static long read_file(const char *path, char *buf, long cap)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }
    long n = (long)fread(buf, 1, (size_t)(cap - 1), fp);
    fclose(fp);
    if (n < 0) {
        n = 0;
    }
    buf[n] = '\0';
    return n;
}

static void reset_counters(void)
{
    g_total = 0;
    g_ok = 0;
    g_crc_fail = 0;
    g_other_fail = 0;
}

static void report(const char *name, const nmea_stream_t *st)
{
    printf("%-24s 收到 %2d 条 | 解析成功 %2d | CRC错 %d | 其他失败 %d | 丢弃 %u\n",
           name, g_total, g_ok, g_crc_fail, g_other_fail, st->dropped);
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "sample_track.nmea";
    static char data[16384];

    long n = read_file(path, data, (long)sizeof(data));
    if (n <= 0) {
        fprintf(stderr, "读不到文件: %s\n", path);
        return 1;
    }
    printf("文件: %s（%ld 字节）\n\n", path, n);

    nmea_stream_t st;
    reset_counters();

    /* ---- 1. 整块喂入 ---- */
    nmea_stream_init(&st, on_sentence, NULL);
    nmea_stream_feed_bytes(&st, (const uint8_t *)data, (uint32_t)n);
    int block_total = g_total;
    report("[1] 整块喂入", &st);

    /* ---- 2. 逐字节喂入（最坏分包） ---- */
    reset_counters();
    nmea_stream_init(&st, on_sentence, NULL);
    for (long i = 0; i < n; i++) {
        nmea_stream_feed(&st, (uint8_t)data[i]);
    }
    int byte_total = g_total;
    report("[2] 逐字节喂入", &st);

    /* ---- 3. 随机块长喂入（1~7 字节一段） ---- */
    reset_counters();
    nmea_stream_init(&st, on_sentence, NULL);
    {
        unsigned seed = 12345u;
        long i = 0;
        while (i < n) {
            seed = seed * 1103515245u + 12345u;
            int chunk = (int)((seed >> 16) % 7u) + 1;
            long take = n - i;
            if (take > chunk) {
                take = chunk;
            }
            nmea_stream_feed_bytes(&st, (const uint8_t *)(data + i), (uint32_t)take);
            i += take;
        }
    }
    int rand_total = g_total;
    report("[3] 随机分包", &st);

    /* ---- 结论 ---- */
    printf("\n");
    int consistent = (block_total == byte_total) && (byte_total == rand_total);
    printf("三种喂法句数一致: %s（%d / %d / %d）\n",
           consistent ? "是" : "否", block_total, byte_total, rand_total);
    printf("CRC 失败: %s\n", (g_crc_fail == 0) ? "无" : "有（说明装配出错）");

    if (!consistent || g_crc_fail != 0 || g_ok != g_total) {
        printf("\n❌ 流式装配测试失败\n");
        return 2;
    }
    printf("\n✅ 流式装配测试通过：分包/粘包下不丢句、不错帧\n");
    return 0;
}

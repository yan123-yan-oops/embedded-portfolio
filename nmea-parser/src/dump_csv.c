/**
 * ============================================================
 * dump_csv.c — 把 C 解析器的结果导出成 CSV
 * ============================================================
 * 用途：给 MATLAB 侧做"真·交叉验证"。
 * 没有它，MATLAB 只能自己再解析一遍 .nmea，那不叫双端验证，
 * 叫"两边各写一遍、各自相信自己"。
 *
 * 用法：
 *   gcc -g -Wall -Wextra -std=c11 dump_csv.c nmea_parser.c -o dump_csv.exe
 *   dump_csv.exe sample_track.nmea > c_output.csv
 *
 * 输出列：
 *   type,status,utc,lat,lon,fix_quality,satellites,hdop,altitude,speed_knot,course_deg,valid
 * ============================================================
 */

#include <stdio.h>
#include <string.h>

#include "nmea_parser.h"

static const char *status_name(nmea_status_t status)
{
    switch (status) {
        case NMEA_OK:       return "OK";
        case NMEA_CRC_FAIL: return "CRC_FAIL";
        case NMEA_INVALID:  return "INVALID";
        case NMEA_ERROR:    return "ERROR";
        default:            return "UNKNOWN";
    }
}

static const char *sentence_type(const char *line)
{
    if (strncmp(line + 1, "GPGGA", 5) == 0 || strncmp(line + 1, "GNGGA", 5) == 0) return "GGA";
    if (strncmp(line + 1, "GPRMC", 5) == 0 || strncmp(line + 1, "GNRMC", 5) == 0) return "RMC";
    return "OTHER";
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "sample_track.nmea";

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "无法打开文件: %s\n", path);
        return 1;
    }

    printf("type,status,utc,lat,lon,fix_quality,satellites,hdop,altitude,speed_knot,course_deg,valid\n");

    char line[256];
    int total = 0;
    int ok = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p != '$') {
            continue;
        }

        size_t len = strlen(p);
        while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r')) {
            p[--len] = '\0';
        }
        if (len < 10) {
            continue;
        }

        gps_data_t gps;
        nmea_status_t status = nmea_parse(p, &gps);

        total++;
        if (status == NMEA_OK) {
            ok++;
        }

        printf("%s,%s,%02u:%02u:%02u.%03u,%.6f,%.6f,%u,%u,%.2f,%.2f,%.2f,%.2f,%d\n",
               sentence_type(p), status_name(status),
               gps.hour, gps.minute, gps.second, gps.millisecond,
               gps.latitude, gps.longitude,
               gps.fix_quality, gps.satellites,
               gps.hdop, gps.altitude,
               gps.speed_knot, gps.course_deg,
               gps.valid ? 1 : 0);
    }

    fclose(fp);
    fprintf(stderr, "共 %d 条语句，解析成功 %d 条\n", total, ok);
    return (ok == total) ? 0 : 2;
}

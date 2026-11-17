# NMEA 0183 解析器

从串口字节流到结构化定位数据的最小实现：**零动态内存、零第三方依赖、可移植到 MCU**。

## 背景

北斗/GPS 模块通过 UART 持续输出 NMEA 语句，主控需要从中提取 UTC 时间、经纬度、
定位质量、卫星数、海拔、速度与航向。但串口是**按字节**到达的：一次中断可能只收到半个句子，
也可能一口气收到三条半。所以需要两层：

1. **流式装配层**（`nmea_stream`）：把字节拼成完整句子；
2. **解析层**（`nmea_parser`）：把句子拆成结构体字段。

## 设计

```
UART 中断 ──逐字节──> nmea_stream ──完整句子──> nmea_parse ──> gps_data_t
                        │
                        └─ 128 字节缓冲，$ 起始、\r\n 结束，超长丢弃并重新同步
```

| 文件 | 职责 |
| --- | --- |
| `nmea_parser.c/h` | 校验和、字段切分、GGA/RMC 解析、经纬度换算 |
| `nmea_stream.c/h` | 逐字节装配，处理分包/粘包 |
| `main.c` | 演示：5 条语句（含 1 条 CRC 错误） |
| `dump_csv.c` | 把解析结果导出 CSV，供 MATLAB 比对 |
| `stream_test.c` | 回归测试：三种喂法必须得到相同结果 |

**几个刻意的选择**：

- **不用 `strtok`**：它会修改原字符串、不可重入，中断与主循环共用时会互相破坏；
- **固定缓冲**：`nmea_fields_t` 16×32 字节，不 `malloc`，适合无堆环境；
- **校验和必须过**：`$` 与 `*` 之间逐字节 XOR，不匹配直接返回 `NMEA_CRC_FAIL`；
- **超长丢弃而非截断**：截断的句子校验和必然错，不如整条丢掉重新找 `$`。

## 构建与运行

```bash
make                  # 生成 demo / stream_test / dump_csv
./demo                # 内置 5 条语句的演示
./stream_test test/sample_track.nmea   # 三种喂法回归测试
./dump_csv test/sample_track.nmea > out.csv
```

## 实测结果

```
$ ./stream_test test/sample_track.nmea
[1] 整块喂入     收到 32 条 | 解析成功 32 | CRC错 0 | 丢弃 0
[2] 逐字节喂入   收到 32 条 | 解析成功 32 | CRC错 0 | 丢弃 0
[3] 随机分包     收到 32 条 | 解析成功 32 | CRC错 0 | 丢弃 0
三种喂法句数一致: 是（32 / 32 / 32）
```

- 合法语句 32/32 解析成功，故意改坏校验和的语句被正确拒绝；
- 三种喂法结果完全一致 → **分包/粘包下不丢句、不错帧**；
- 与 MATLAB 独立实现逐点比对，经纬度最大偏差 **< 1e-6 度**（≈0.11 m，等于 `DDMM.MMMM` 的量化下限）。

## 接口

```c
nmea_status_t nmea_parse(const char *sentence, gps_data_t *out);
uint8_t       nmea_calc_checksum(const char *sentence);
void          nmea_print_gps(const gps_data_t *gps);

void nmea_stream_init(nmea_stream_t *st, nmea_stream_cb cb, void *user);
void nmea_stream_feed(nmea_stream_t *st, uint8_t byte);
```

## 已知限制

- 只支持 `GPGGA` / `GPRMC`；北斗模块常输出 `GNGGA` / `GNRMC`，需扩展语句头判断（一行改动）；
- 不解析 GSA/GSV，因此没有卫星方位角与 DOP 全量信息。

## 目录

```
├── src/   nmea_parser.c/h、nmea_stream.c/h、main.c、dump_csv.c、stream_test.c
├── test/  sample_track.nmea
└── Makefile
```

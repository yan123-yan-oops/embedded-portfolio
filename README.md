# 嵌入式作品集

两个用 C 从零写的模块，都能独立编译、带测试、有可复现的数字。

| 项目 | 解决的问题 | 关键结果 |
| --- | --- | --- |
| [NMEA 0183 解析器](nmea-parser/) | 串口来的 GPS/北斗语句，要可靠地解析成经纬度、时间、定位质量 | 32/32 条解析成功；整块 / 逐字节 / 随机 1–7 字节分包三种喂法 **0 丢包、0 CRC 错**；与 MATLAB 独立实现比对，经纬度偏差 **< 1e-6 度** |
| [静态内存池](memory-pool/) | 采集系统频繁申请释放消息块，`malloc` 带来碎片和不确定时延 | 固定块分配 **O(1)**；混合分配后释放，空闲 768 B 但最大连续仅 **568 B（碎片率 26%）**，量化了"总量够、分不出" |
| [北斗 B1I 抗干扰仿真](anti-jamming/) | 抗干扰算法上 FPGA/MCU 之前，先验证到底有没有效 | 捕获度量 8.68 → 31.47 dB（**+22.79 dB**）；干扰时相关峰偏到 61423，抗干扰后回到真实码相位 0 |

## 快速开始

```bash
cd nmea-parser && make && ./demo && ./stream_test test/sample_track.nmea
cd ../memory-pool && make && ./demo
```

需要 GCC（C11）。两个项目都**零第三方依赖、零动态内存**。

## 技术点

- **协议解析**：校验和、字段切分（不用 `strtok`）、`DDMM.MMMM` → 十进制度、S/W 取负
- **流式装配**：`$` 起始、`\r\n` 结束、128 字节上限，超长丢弃并重新同步
- **内存管理**：固定块位图 + 变长首次适配 + 相邻空闲块合并
- **信号处理**：B1I 中频生成、CW/宽带/脉冲干扰建模、LMS 陷波、FFT 相关捕获、JSR 分析
- **可移植性**：无 OS 依赖、无 `malloc`，同一份代码在 PC 与 MCU 上跑
- **可验证性**：自带测试向量、逐点比对与双实现互验

## 目录

```
├── nmea-parser/
│   ├── src/          nmea_parser.c/h、nmea_stream.c/h、main.c、dump_csv.c、stream_test.c
│   ├── test/         sample_track.nmea（真实轨迹样本）
│   └── Makefile
├── memory-pool/
│   ├── src/memory_pool.c
│   └── Makefile
└── anti-jamming/
    ├── src/full_chain_check.c   C 参考实现（信号/干扰/陷波/捕获）
    ├── matlab/                  MATLAB 全链路与双端比对（待实机运行）
    └── Makefile
```

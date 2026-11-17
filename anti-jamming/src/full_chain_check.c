/**
 * full_chain_check.c — 用 C 复现 10 章全链路的核心数学
 *
 * 用途：在没有 MATLAB 的机器上先确认算法方向没错——
 *   1) 11 级 m 序列生成
 *   2) LMS 两权陷波在 mu=0.01、2 万点内收敛
 *   3) 并行码相位搜索的峰位置
 *   4) 抗干扰前后捕获度量确有改善（不是负的）
 *
 * 它和 MATLAB_Projects/10-final-project/run_full_chain.m 一一对应：
 * 同样的 Fs/IF/JSR/mu，同样的 fft 相关公式。
 * 面试时可以说："MATLAB 的算法我先用 C 复现验证过，再回填仿真。"
 *
 * 编译：
 *   gcc -g -Wall -Wextra -std=c11 -O2 full_chain_check.c -o full_chain_check.exe -lm
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

typedef struct { double re, im; } cplx;

static void fft(cplx *a, int n, int inverse)
{
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { cplx t = a[i]; a[i] = a[j]; a[j] = t; }
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * PI / len * (inverse ? 1 : -1);
        cplx wlen = { cos(ang), sin(ang) };
        for (int i = 0; i < n; i += len) {
            cplx w = { 1.0, 0.0 };
            for (int k = 0; k < len / 2; k++) {
                cplx u = a[i + k];
                cplx v;
                v.re = a[i + k + len/2].re * w.re - a[i + k + len/2].im * w.im;
                v.im = a[i + k + len/2].re * w.im + a[i + k + len/2].im * w.re;
                a[i + k].re = u.re + v.re; a[i + k].im = u.im + v.im;
                a[i + k + len/2].re = u.re - v.re; a[i + k + len/2].im = u.im - v.im;
                double wr = w.re * wlen.re - w.im * wlen.im;
                double wi = w.re * wlen.im + w.im * wlen.re;
                w.re = wr; w.im = wi;
            }
        }
    }
    if (inverse) {
        for (int i = 0; i < n; i++) { a[i].re /= n; a[i].im /= n; }
    }
}

/** 峰值 / 去峰后 RMS（dB），并给出峰位置。 */
static double peak_metric(const cplx *r, int n, int *peak_pos)
{
    double peak = 0.0;
    int pos = 0;
    for (int i = 0; i < n; i++) {
        double m = sqrt(r[i].re * r[i].re + r[i].im * r[i].im);
        if (m > peak) { peak = m; pos = i; }
    }
    const int guard = 20;
    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (abs(i - pos) <= guard) continue;
        double m = sqrt(r[i].re * r[i].re + r[i].im * r[i].im);
        sum += m * m;
        count++;
    }
    double rms = sqrt(sum / (count > 0 ? count : 1));
    *peak_pos = pos;
    return 20.0 * log10(peak / rms);
}

int main(void)
{
    const double Fs = 20.46e6;
    const double IF = 4.098e6;
    const double T = 1e-3;
    const double JSR_dB = 30.0;
    const double mu = 0.01;
    const int N = (int)(Fs * T + 0.5);
    const int spc = (int)(Fs / 2.046e6 + 0.5);

    int lfsr[11];
    for (int i = 0; i < 11; i++) lfsr[i] = 1;
    double prn[2046];
    for (int k = 0; k < 2046; k++) {
        prn[k] = lfsr[10] ? 1.0 : -1.0;
        int fb = (lfsr[10] ^ lfsr[2]) ? 1 : 0;
        for (int i = 10; i > 0; i--) lfsr[i] = lfsr[i - 1];
        lfsr[0] = fb;
    }

    double *code = malloc(sizeof(double) * (size_t)N);
    double *sig  = malloc(sizeof(double) * (size_t)N);
    double *rx   = malloc(sizeof(double) * (size_t)N);
    double *aj   = malloc(sizeof(double) * (size_t)N);
    if (!code || !sig || !rx || !aj) return 1;

    for (int i = 0; i < N; i++) code[i] = prn[(i / spc) % 2046];

    double Ps = 0.0;
    for (int i = 0; i < N; i++) {
        double tt = (double)i / Fs;
        sig[i] = code[i] * cos(2 * PI * IF * tt);
        Ps += sig[i] * sig[i];
    }
    Ps /= N;

    double A_jam = sqrt(pow(10.0, JSR_dB / 10.0) * Ps * 2.0);
    double f_jam = IF + 0.3e6;
    for (int i = 0; i < N; i++) {
        double tt = (double)i / Fs;
        rx[i] = sig[i] + A_jam * sin(2 * PI * f_jam * tt);
    }

    double w1 = 0.0, w2 = 0.0;
    for (int i = 0; i < N; i++) {
        double tt = (double)i / Fs;
        double c = cos(2 * PI * f_jam * tt);
        double s = sin(2 * PI * f_jam * tt);
        double e = rx[i] - (w1 * c + w2 * s);
        aj[i] = e;
        w1 += mu * e * c;
        w2 += mu * e * s;
    }

    int Nfft = 1;
    while (Nfft < 2 * N) Nfft <<= 1;
    cplx *X = calloc((size_t)Nfft, sizeof(cplx));
    cplx *C = calloc((size_t)Nfft, sizeof(cplx));
    if (!X || !C) return 1;

    for (int i = 0; i < N; i++) C[i].re = code[i];
    fft(C, Nfft, 0);
    for (int i = 0; i < Nfft; i++) C[i].im = -C[i].im;  /* conj */

    double metric[2];
    int pos[2];
    double *src[2] = { rx, aj };

    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < Nfft; i++) { X[i].re = 0.0; X[i].im = 0.0; }
        for (int i = 0; i < N; i++) {
            double ph = -2 * PI * IF * ((double)i / Fs);
            X[i].re = src[k][i] * cos(ph);
            X[i].im = src[k][i] * sin(ph);
        }
        fft(X, Nfft, 0);
        for (int i = 0; i < Nfft; i++) {
            double re = X[i].re * C[i].re - X[i].im * C[i].im;
            double im = X[i].re * C[i].im + X[i].im * C[i].re;
            X[i].re = re; X[i].im = im;
        }
        fft(X, Nfft, 1);
        metric[k] = peak_metric(X, Nfft, &pos[k]);
    }

    printf("N=%d spc=%d Ps=%.3f A_jam=%.2f |w|=%.3f\n", N, spc, Ps, A_jam, sqrt(w1*w1 + w2*w2));
    printf("before: %.2f dB @ %d\n", metric[0], pos[0]);
    printf("after : %.2f dB @ %d\n", metric[1], pos[1]);
    printf("gain  : %+.2f dB\n", metric[1] - metric[0]);

    free(code); free(sig); free(rx); free(aj); free(X); free(C);
    return 0;
}

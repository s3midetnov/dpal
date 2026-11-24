/*
 * dpal.c — ищет числа n, которые являются палиндромами
 * одновременно в десятичной и в двоичной системе.
 *
 * Использование:  ./dpal L [префикс]
 *   L       — число десятичных цифр (1..37; нечётное — чётные делятся на 11)
 *   префикс — необязательно: первые 1–2 десятичные цифры (для распараллеливания)
 * Печатает найденные числа по одному в строке в stdout.
 *
 * Идея: перебираем пары внешних десятичных цифр. Известные младшие цифры
 * дают n mod 10^k (значит, младшие k бит), а интервал, заданный старшими
 * цифрами, — общий префикс старших бит. Двоичный палиндром требует, чтобы
 * старшие биты были зеркалом младших → отсекаем. Когда модуль
 * 5^k * 2^t по китайской теореме об остатках становится больше длины
 * интервала, кандидатов единицы — проверяем их напрямую.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned __int128 u128;
typedef unsigned long long u64;

static u128 P10[39];
static u64 P5[28];
static int L, B;
static u128 LB, HB;
static const char *PREFIX = "";

static void print128(u128 x) {
    char s[50]; int i = 49; s[i] = 0;
    if (!x) s[--i] = '0';
    while (x) { s[--i] = '0' + (int)(x % 10); x /= 10; }
    printf("%s\n", &s[i]); fflush(stdout);
}
static int ispal2(u128 n)  { u128 r = 0, m = n; while (m) { r = (r << 1) | (m & 1); m >>= 1; } return r == n; }
static int ispal10(u128 n) { u128 r = 0, m = n; while (m) { r = r * 10 + m % 10; m /= 10; } return r == n; }
static u64 mulm(u64 a, u64 b, u64 m) { return (u64)((u128)a * b % m); }
static u64 powm(u64 a, u64 e, u64 m) { u64 r = 1 % m; a %= m; while (e) { if (e & 1) r = mulm(r, a, m); a = mulm(a, a, m); e >>= 1; } return r; }

static void rec(int k, u128 top, u128 low) {
    int free = L - 2 * k; if (free < 0) free = 0;
    u128 base = top + low;
    u128 lo = base, hi = base + (free > 0 ? (P10[L - k] - P10[k]) : 0);
    if (hi < LB || lo > HB) return;
    if (lo < LB) lo = LB;
    if (hi > HB) hi = HB;

    /* общий префикс старших бит интервала */
    int t = 0;
    for (int i = B - 1; i >= 0; i--) { if (((lo >> i) & 1) != ((hi >> i) & 1)) break; t++; }
    /* старшие биты должны зеркалить младшие k бит */
    int m = t < k ? t : k;
    for (int i = 0; i < m; i++)
        if (((lo >> (B - 1 - i)) & 1) != ((low >> i) & 1)) return;

    /* КТО: n ≡ low (mod 5^k), n ≡ reverse(top t bits) (mod 2^t) */
    if (k > 0) {
        int tt = t; if (tt > B / 2 + 1) tt = B / 2 + 1;
        int b5 = 0; { u64 x = P5[k]; while (x) { b5++; x >>= 1; } }
        if (tt + b5 > 125) tt = 125 - b5;
        if (tt >= k) {
            u128 b = 0;
            for (int i = 0; i < tt; i++) b |= (u128)((lo >> (B - 1 - i)) & 1) << i;
            u128 M = ((u128)P5[k]) << tt;
            if ((hi - lo) / M < 8) {
                u64 m5 = P5[k];
                u64 l5 = (u64)(low % m5), br = (u64)(b % m5);
                u64 inv = powm(powm(2, tt, m5), m5 / 5 * 4 - 1, m5);
                u64 x = mulm((l5 + m5 - br) % m5, inv, m5);
                u128 r = b + ((u128)x << tt);
                u128 n = (lo <= r) ? r : r + ((lo - r + M - 1) / M) * M;
                for (; n <= hi; n += M) {
                    if (ispal10(n) && ispal2(n)) print128(n);
                    if (n + M < n) break;
                }
                return;
            }
        }
    }
    if (free == 0) { if (ispal2(lo) && ispal10(lo)) print128(lo); return; }
    if (free == 1) {
        for (int d = 0; d < 10; d++) {
            u128 n = base + (u128)d * P10[k];
            if (n >= LB && n <= HB && ispal2(n)) print128(n);
        }
        return;
    }
    for (int d = (k == 0 ? 1 : 0); d < 10; d++) {
        if (k < (int)strlen(PREFIX) && PREFIX[k] - '0' != d) continue;
        rec(k + 1, top + (u128)d * P10[L - 1 - k], low + (u128)d * P10[k]);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s L [prefix]\n", argv[0]); return 1; }
    L = atoi(argv[1]);
    if (argc > 2) PREFIX = argv[2];
    if (L < 1 || L > 37) { fprintf(stderr, "L must be 1..37 (128-bit limit)\n"); return 1; }
    P10[0] = 1; for (int i = 1; i < 39; i++) P10[i] = P10[i - 1] * 10;
    P5[0] = 1;  for (int i = 1; i < 28; i++) P5[i] = P5[i - 1] * 5;
    for (B = 1; B <= 127; B++) {
        u128 lb = (u128)1 << (B - 1), hb = lb - 1 + lb;
        if (hb < P10[L - 1]) continue;
        if (lb >= P10[L]) break;
        LB = lb; HB = hb;
        rec(0, 0, 0);
    }
    return 0;
}

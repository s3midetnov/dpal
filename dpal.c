/*
 * dpal.c — ищет числа n, которые являются палиндромами
 * одновременно в десятичной и в двоичной системе.
 * Версия с 256-битной арифметикой: L до 75 десятичных цифр.
 *
 * Использование:  ./dpal L [префикс]
 *   L       — число десятичных цифр (нечётное; чётные палиндромы делятся на 11)
 *   префикс — необязательно: первые десятичные цифры (для распараллеливания)
 * Печатает найденные числа по одному в строке в stdout.
 *
 * Идея: перебираем пары внешних десятичных цифр. Известные младшие цифры
 * дают n mod 10^k (значит, младшие k бит), а интервал, заданный старшими
 * цифрами, — общий префикс старших бит. Двоичный палиндром требует, чтобы
 * старшие биты были зеркалом младших → отсекаем. Когда модуль
 * M = 5^k * 2^t (китайская теорема об остатках) становится больше длины
 * интервала, кандидатов единицы — проверяем их напрямую.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef unsigned __int128 u128;
typedef uint64_t u64;

#define NW 4               /* 4 x 64 = 256 бит */
#define MAXL 75            /* 10^75 < 2^250 */
#define K5MAX 27           /* 5^27 < 2^64 */

typedef struct { u64 w[NW]; } U;

static inline U U0(void) { U r; memset(&r, 0, sizeof r); return r; }
static inline U Uu(u64 x) { U r = U0(); r.w[0] = x; return r; }
static inline U add(U a, U b) { U r; u128 c = 0; for (int i = 0; i < NW; i++) { c += (u128)a.w[i] + b.w[i]; r.w[i] = (u64)c; c >>= 64; } return r; }
static inline U sub(U a, U b) { U r; u64 br = 0; for (int i = 0; i < NW; i++) { u128 d = (u128)a.w[i] - b.w[i] - br; r.w[i] = (u64)d; br = (u64)(d >> 64) & 1; } return r; }
static inline int cmp(U a, U b) { for (int i = NW - 1; i >= 0; i--) { if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1; } return 0; }
static inline U mulsmall(U a, u64 m) { U r; u128 c = 0; for (int i = 0; i < NW; i++) { c += (u128)a.w[i] * m; r.w[i] = (u64)c; c >>= 64; } return r; }
static inline int bit(U a, int i) { return (int)((a.w[i >> 6] >> (i & 63)) & 1); }
static inline void setbit(U *a, int i) { a->w[i >> 6] |= (u64)1 << (i & 63); }
static inline int msb(U a) { for (int i = NW - 1; i >= 0; i--) if (a.w[i]) return i * 64 + 63 - __builtin_clzll(a.w[i]); return -1; }
static inline U xr(U a, U b) { U r; for (int i = 0; i < NW; i++) r.w[i] = a.w[i] ^ b.w[i]; return r; }
static inline U shl(U a, int s) {
    U r = U0(); int q = s >> 6, b = s & 63;
    for (int i = NW - 1; i >= q; i--) {
        r.w[i] = a.w[i - q] << b;
        if (b && i - q - 1 >= 0) r.w[i] |= a.w[i - q - 1] >> (64 - b);
    }
    return r;
}
static inline U masklow(U a, int t) {
    for (int i = 0; i < NW; i++) {
        int lo = i * 64;
        if (t <= lo) a.w[i] = 0;
        else if (t < lo + 64) a.w[i] &= ((u64)1 << (t - lo)) - 1;
    }
    return a;
}
static inline u64 modsmall(U a, u64 m) { u64 r = 0; for (int i = NW - 1; i >= 0; i--) r = (u64)((((u128)r << 64) | a.w[i]) % m); return r; }
static u64 divsmall(U *a, u64 m) { u128 r = 0; for (int i = NW - 1; i >= 0; i--) { r = (r << 64) | a->w[i]; a->w[i] = (u64)(r / m); r %= m; } return (u64)r; }
static int iszero(U a) { for (int i = 0; i < NW; i++) if (a.w[i]) return 0; return 1; }

static u64 mulm(u64 a, u64 b, u64 m) { return (u64)((u128)a * b % m); }
static u64 powm(u64 a, u64 e, u64 m) { u64 r = 1 % m; a %= m; while (e) { if (e & 1) r = mulm(r, a, m); a = mulm(a, a, m); e >>= 1; } return r; }

static void todec(U x, char *out) {
    char buf[100]; int n = 0;
    if (iszero(x)) { strcpy(out, "0"); return; }
    while (!iszero(x)) { u64 r = divsmall(&x, 10); buf[n++] = '0' + (int)r; }
    for (int i = 0; i < n; i++) out[i] = buf[n - 1 - i];
    out[n] = 0;
}
static void print(U x) { char s[100]; todec(x, s); printf("%s\n", s); fflush(stdout); }
static int ispal10(U x) { char s[100]; todec(x, s); int n = strlen(s); for (int i = 0; i < n / 2; i++) if (s[i] != s[n - 1 - i]) return 0; return 1; }

static U P10[MAXL + 2];
static u64 P5[K5MAX + 1];
static int L, B;
static U LB, HB;
static const char *PREFIX = "";
static int plen = 0;

static int ispal2(U n) { for (int i = 0; i < B / 2; i++) if (bit(n, i) != bit(n, B - 1 - i)) return 0; return 1; }

static void rec(int k, U top, U low) {
    int free = L - 2 * k; if (free < 0) free = 0;
    U base = add(top, low);
    U lo = base, hi = base;
    if (free > 0) hi = add(base, sub(P10[L - k], P10[k]));
    if (cmp(hi, LB) < 0 || cmp(lo, HB) > 0) return;
    if (cmp(lo, LB) < 0) lo = LB;
    if (cmp(hi, HB) > 0) hi = HB;

    /* общий префикс старших бит интервала */
    int t = B - 1 - msb(xr(lo, hi));   /* msb = -1 если равны → t = B */
    int m = t < k ? t : k;
    for (int i = 0; i < m; i++)
        if (bit(lo, B - 1 - i) != bit(low, i)) return;

    /* КТО: n ≡ low (mod 5^k5), n ≡ reverse(top tt bits) (mod 2^tt) */
    if (k > 0) {
        int k5 = k < K5MAX ? k : K5MAX;
        u64 m5 = P5[k5];
        int b5 = 64 - __builtin_clzll(m5);
        int tt = t; if (tt > B / 2 + 1) tt = B / 2 + 1;
        if (tt + b5 > NW * 64 - 6) tt = NW * 64 - 6 - b5;
        if (tt >= k) {
            U M = shl(Uu(m5), tt);
            U width = sub(hi, lo);
            if (cmp(width, shl(M, 3)) < 0) {
                U b = U0();
                for (int i = 0; i < tt; i++) if (bit(lo, B - 1 - i)) setbit(&b, i);
                u64 inv = powm(powm(2, tt, m5), m5 / 5 * 4 - 1, m5);   /* φ(5^j)=4·5^(j-1) */
                /* r = искомый остаток по модулю M */
                u64 x = mulm((modsmall(low, m5) + m5 - modsmall(b, m5)) % m5, inv, m5);
                U r = add(b, shl(Uu(x), tt));
                /* s = lo mod M */
                U lo2 = masklow(lo, tt);
                u64 y = mulm((modsmall(lo, m5) + m5 - modsmall(lo2, m5)) % m5, inv, m5);
                U s = add(lo2, shl(Uu(y), tt));
                U n = add(sub(lo, s), r);
                if (cmp(r, s) < 0) n = add(n, M);
                for (; cmp(n, hi) <= 0; n = add(n, M))
                    if (ispal2(n) && ispal10(n)) print(n);
                return;
            }
        }
    }
    if (free == 0) { if (ispal2(lo) && ispal10(lo)) print(lo); return; }
    if (free == 1) {
        for (int d = 0; d < 10; d++) {
            U n = add(base, mulsmall(P10[k], d));
            if (cmp(n, LB) >= 0 && cmp(n, HB) <= 0 && ispal2(n)) print(n);
        }
        return;
    }
    for (int d = (k == 0 ? 1 : 0); d < 10; d++) {
        if (k < plen && PREFIX[k] - '0' != d) continue;
        rec(k + 1, add(top, mulsmall(P10[L - 1 - k], d)), add(low, mulsmall(P10[k], d)));
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s L [prefix]\n", argv[0]); return 1; }
    L = atoi(argv[1]);
    if (argc > 2) { PREFIX = argv[2]; plen = strlen(PREFIX); }
    if (L < 1 || L > MAXL) { fprintf(stderr, "L must be 1..%d\n", MAXL); return 1; }
    if (plen * 2 > L) { fprintf(stderr, "prefix too long for L\n"); return 1; }
    P10[0] = Uu(1); for (int i = 1; i <= MAXL + 1; i++) P10[i] = mulsmall(P10[i - 1], 10);
    P5[0] = 1; for (int i = 1; i <= K5MAX; i++) P5[i] = P5[i - 1] * 5;
    for (B = 1; B < NW * 64 - 4; B++) {
        U lb = shl(Uu(1), B - 1);
        U hb = sub(shl(Uu(1), B), Uu(1));
        if (cmp(hb, P10[L - 1]) < 0) continue;
        if (cmp(lb, P10[L]) >= 0) break;
        LB = lb; HB = hb;
        rec(0, U0(), U0());
    }
    return 0;
}

#!/usr/bin/env python3
"""Читает числа (по одному в строке) из файлов или stdin.
Печатает те, где n простое, и отмечает, простая ли двоичная запись n,
прочитанная как десятичное число. Строки с HIT — искомые числа."""
import fileinput
import gmpy2

for line in fileinput.input():
    line = line.strip()
    if not line:
        continue
    n = int(line)
    if not gmpy2.is_prime(n, 50):
        continue
    b = bin(n)[2:]
    ok = gmpy2.is_prime(int(b), 50)
    print(("HIT  " if ok else "prime") + f"  n={n}  bin={b}", flush=True)

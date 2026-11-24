#!/usr/bin/env bash
# Запуск: ./run.sh 35 37      (длины через пробел; по умолчанию 35 37)
# Работает на всех ядрах, задачи разбиты по первым двум цифрам.
# Можно прерывать и перезапускать: готовые куски пропускаются.
set -euo pipefail
cd "$(dirname "$0")"
gcc -O3 -march=native -o dpal dpal.c
mkdir -p out
LENS=${*:-"35 37"}
JOBS=$(nproc)

for L in $LENS; do
  for a in 1 3 5 7 9; do        # первая цифра нечётная (n нечётное)
    for b in 0 1 2 3 4 5 6 7 8 9; do
      echo "$L $a$b"
    done
  done
done | xargs -P "$JOBS" -L1 sh -c '
  f="out/L$0_$1"
  [ -f "$f.done" ] && exit 0
  ./dpal "$0" "$1" > "$f.txt" && touch "$f.done" && echo "готово L=$0 префикс=$1"
'

echo "=== проверка простоты ==="
cat out/*.txt | sort -un | python3 check.py > results.txt
cat results.txt
grep -q HIT results.txt && echo ">>> ЕСТЬ РЕШЕНИЕ (строки HIT)" || echo "Решений (HIT) не найдено"

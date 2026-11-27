#!/usr/bin/env bash
# Запуск:   ./run.sh 39 41            (длины через пробел; по умолчанию 39)
# Больше кусков (для машин с десятками ядер):  PD=3 ./run.sh 39 41
# Работает на всех ядрах, задачи разбиты по первым PD цифрам (по умолчанию 2).
# Можно прерывать и перезапускать: готовые куски пропускаются.
set -euo pipefail
cd "$(dirname "$0")"
gcc -O3 -march=native -o dpal dpal.c
mkdir -p out
LENS=${*:-"39"}
PD=${PD:-2}
JOBS=$(nproc)

prefixes() {   # все префиксы длины PD, первая цифра нечётная (n нечётное)
  local d=$1
  if [ "$d" -eq 1 ]; then printf "%s\n" 1 3 5 7 9; return; fi
  prefixes $((d-1)) | while read -r p; do for c in 0 1 2 3 4 5 6 7 8 9; do echo "$p$c"; done; done
}

for L in $LENS; do
  if [ $((L % 2)) -eq 0 ]; then echo "L=$L чётное — пропускаю (делится на 11)" >&2; continue; fi
  prefixes "$PD" | while read -r p; do echo "$L $p"; done
done | xargs -P "$JOBS" -L1 sh -c '
  f="out/L$0_$1"
  [ -f "$f.done" ] && exit 0
  ./dpal "$0" "$1" > "$f.txt" && touch "$f.done" && echo "готово L=$0 префикс=$1  ($(ls out/*.done | wc -l) кусков всего)"
'

echo "=== проверка простоты ==="
cat out/*.txt | sort -un | python3 check.py > results.txt
cat results.txt
grep -q HIT results.txt && echo ">>> ЕСТЬ РЕШЕНИЕ (строки HIT)" || echo "Решений (HIT) не найдено"

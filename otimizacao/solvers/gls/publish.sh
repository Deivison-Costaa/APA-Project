#!/usr/bin/env bash
# usage: publish.sh <instance-name> <solution-file>
# Validates the solution with the independent checker and, if it is strictly better
# than the pool's best, publishes it atomically as <inst>_<cost>_gls.txt.
set -euo pipefail
W=/tmp/claude-1000/-home-ddscosta--rea-de-trabalho-Faculdade-APA-APA-Project--claude-code-/5aab076d-8aac-4e11-afe9-0cef821c0125/scratchpad/work
inst="$1"; sol="$2"
[ -f "$sol" ] || { echo "no solution file $sol"; exit 1; }
out=$(python3 "$W/tools/check.py" "$W/instances/$inst.txt" "$sol") || { echo "INVALID: $out"; exit 1; }
cost=$(head -1 "$sol" | tr -d '[:space:]')
best=$(ls "$W/pool/$inst" | sort -t_ -k2 -n | head -1 | cut -d_ -f2 | cut -d. -f1)
echo "$out | pool best=${best:-none}"
if [ -z "$best" ] || [ "$cost" -lt "$best" ]; then
  tmp="$W/pool/$inst/.tmp_gls_$$"
  cp "$sol" "$tmp"
  mv "$tmp" "$W/pool/$inst/${inst}_${cost}_gls.txt"
  echo "PUBLISHED ${inst}_${cost}_gls.txt"
fi

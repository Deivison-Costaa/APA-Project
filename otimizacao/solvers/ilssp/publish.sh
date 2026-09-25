#!/usr/bin/env bash
# Validates a solution with the independent checker and publishes it to the
# shared pool atomically if it is strictly better than the pool's best.
# usage: publish.sh <inst-name> <solution-file>
set -euo pipefail
W="$(cd "$(dirname "$0")/../.." && pwd)"
inst="$1"
sol="$2"
[[ -f "$sol" ]] || { echo "no file $sol"; exit 1; }
python3 "$W/tools/check.py" "$W/instances/$inst.txt" "$sol" >/dev/null || { echo "INVALID $sol"; exit 1; }
cost="$(head -1 "$sol" | tr -d '[:space:]')"
bestf="$(ls "$W/pool/$inst" | grep -E '^'"$inst"'_[0-9]+' | sort -t_ -k2 -n | head -1 || true)"
best="$(echo "$bestf" | cut -d_ -f2 | sed 's/\.txt$//')"
if [[ -z "$best" || "$cost" -lt "$best" ]]; then
  tmp="$W/pool/$inst/.tmp_ilssp_$$"
  cp "$sol" "$tmp"
  mv "$tmp" "$W/pool/$inst/${inst}_${cost}_ilssp.txt"
  echo "PUBLISHED $inst $cost (pool best was ${best:-none})"
else
  echo "not better: $inst $cost (pool best $best)"
fi

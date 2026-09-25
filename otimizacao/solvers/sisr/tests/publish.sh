#!/usr/bin/env bash
# publish.sh <instance-name> <solution-file>
# Validates with the independent checker and publishes atomically to the shared
# pool only if strictly better than the pool's current best.
set -euo pipefail
WORK="$(cd "$(dirname "$0")/../../.." && pwd)"
inst="$1"; sol="$2"
pool="$WORK/pool/$inst"
python3 "$WORK/tools/check.py" "$WORK/instances/$inst.txt" "$sol" >/dev/null || { echo "INVALID $sol"; exit 1; }
cost=$(head -n1 "$sol" | tr -d '[:space:]')
best=$(ls "$pool" | grep -E "^${inst}_[0-9]+" | sed -E "s/^${inst}_([0-9]+).*/\1/" | sort -n | head -n1)
if [[ -n "$best" && "$cost" -ge "$best" ]]; then echo "$inst: $cost not better than pool best $best"; exit 0; fi
tmp="$pool/.tmp_sisr_$$"
cp "$sol" "$tmp"
mv "$tmp" "$pool/${inst}_${cost}_sisr.txt"
echo "$inst: published $cost (previous pool best ${best:-none})"

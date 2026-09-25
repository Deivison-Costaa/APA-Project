#!/usr/bin/env bash
# batch.sh <time> <tag> "<inst> <args...>" ["<inst> <args...>" ...]
# Runs up to 3 configurations in parallel (CPU budget), validates each result.
set -uo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(cd "$DIR/../.." && pwd)"
tl="$1"; tag="$2"; shift 2
i=0
pids=()
for cfg in "$@"; do
  read -r inst args <<<"$cfg"
  name="${tag}_${i}_${inst}"
  echo "$name: $args" > "$DIR/runs/$name.log"
  timeout $((${tl%.*} + 30)) "$DIR/sisr" "$WORK/instances/$inst.txt" --time "$tl" $args --out "$DIR/runs/$name.txt" >> "$DIR/runs/$name.log" 2>&1 &
  pids+=($!)
  i=$((i + 1))
done
wait "${pids[@]}"
i=0
for cfg in "$@"; do
  read -r inst args <<<"$cfg"
  name="${tag}_${i}_${inst}"
  res=$(python3 "$WORK/tools/check.py" "$WORK/instances/$inst.txt" "$DIR/runs/$name.txt" 2>&1 | sed 's/.*: //')
  its=$(grep -o 'iters=[0-9]* ([0-9]* it/s' "$DIR/runs/$name.log" | tail -n1)
  echo "$name | $args | $res | $its"
  i=$((i + 1))
done

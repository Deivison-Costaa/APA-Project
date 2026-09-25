#!/usr/bin/env bash
# Final COLD benchmark: 300 s, 1 thread, seed 1, the 3 instances in parallel.
DIR="$(cd "$(dirname "$0")/.." && pwd)"; WORK="$(cd "$DIR/../.." && pwd)"
for inst in n500m10E n700m12E n1000m15E; do
  timeout 330 "$DIR/sisr" "$WORK/instances/$inst.txt" --time 300 --threads 1 --seed 1 \
    --out "$DIR/bench/cold_$inst.txt" > "$DIR/bench/cold_$inst.log" 2>&1 &
done
wait
for inst in n500m10E n700m12E n1000m15E; do
  python3 "$WORK/tools/check.py" "$WORK/instances/$inst.txt" "$DIR/bench/cold_$inst.txt"
done

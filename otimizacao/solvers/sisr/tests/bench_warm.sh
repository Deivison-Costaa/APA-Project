#!/usr/bin/env bash
# Final WARM benchmark: from the pool's best file at launch, 600 s, 1 thread, 3 instances in
# parallel; validates and publishes improvements to the pool.
DIR="$(cd "$(dirname "$0")/.." && pwd)"; WORK="$(cd "$DIR/../.." && pwd)"
for inst in n500m10E n700m12E n1000m15E; do
  best=$(ls "$WORK/pool/$inst" | grep -E "^${inst}_[0-9]+" | sort -t_ -k2 -n | head -n1)
  cp "$WORK/pool/$inst/$best" "$DIR/bench/warm_start_$inst.txt"
  echo "$inst start $best"
  timeout 640 "$DIR/sisr" "$WORK/instances/$inst.txt" --time 600 --threads 1 --seed 1 \
    --init "$DIR/bench/warm_start_$inst.txt" --out "$DIR/bench/warm_$inst.txt" > "$DIR/bench/warm_$inst.log" 2>&1 &
done
wait
for inst in n500m10E n700m12E n1000m15E; do
  echo "$inst start $(head -n1 "$DIR/bench/warm_start_$inst.txt") end $(head -n1 "$DIR/bench/warm_$inst.txt")"
  python3 "$WORK/tools/check.py" "$WORK/instances/$inst.txt" "$DIR/bench/warm_$inst.txt" && \
    "$DIR/tests/publish.sh" "$inst" "$DIR/bench/warm_$inst.txt"
done

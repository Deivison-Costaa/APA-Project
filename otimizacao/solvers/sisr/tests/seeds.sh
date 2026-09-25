#!/usr/bin/env bash
# seeds.sh <time> <tag> <inst> "<args>" : runs seeds 1..3 in parallel, prints costs and mean.
set -uo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
tl="$1"; tag="$2"; inst="$3"; args="$4"
out=$("$DIR/batch.sh" "$tl" "$tag" "$inst $args --seed 1" "$inst $args --seed 2" "$inst $args --seed 3")
costs=$(echo "$out" | grep -o 'cost=[0-9]*' | cut -d= -f2 | tr '\n' ' ')
its=$(echo "$out" | grep -o '([0-9]* it/s' | head -n1)
mean=$(echo "$costs" | awk '{s=0; for(i=1;i<=NF;i++) s+=$i; printf "%.0f", s/NF}')
echo "$tag $inst [$args] -> $costs mean=$mean $its"

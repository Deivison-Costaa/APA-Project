#!/bin/bash
# prints mean/min of per-run bests (restart lines) of the given logs
for f in "$@"; do
  grep -oE "pop best [0-9]+" $f | awk -v f=$(basename $f .log) '{s+=$3; n++; if(min==""||$3<min)min=$3; v=v" "$3} END {printf "%-22s runs %2d mean %.1f min %d |%s\n", f, n, s/n, min, v}'
done

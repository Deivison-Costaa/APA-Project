#!/bin/bash
cd $(dirname $0)
for inst in n700m12E n1000m15E; do
 for ej in 0 1; do
  (for s in 1 2 3; do ./bench.sh g_${inst}_ej${ej}_s$s $inst 60 --xover mix --seed $s --ejection $ej & done; wait)
 done
done

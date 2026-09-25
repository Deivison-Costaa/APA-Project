#!/bin/bash
# Crossover benefit: same budget (120 s, seed 1, 1 thread) with
#   none = multi-start LS (random greedy + full descent, population only keeps results)
#   ruin = population + ruin&recreate mutation only (no crossover)
#   tw   = population + time-window crossover only
#   mix  = default (70 % tw, 30 % ruin)
cd $(dirname $0)
for x in none ruin tw mix; do
  (for i in n500m10E n700m12E n1000m15E; do
     timeout 180 ./hgs_v1 ../../../instances/$i.txt --time 120 --seed 1 --xover $x --out abl_${x}_$i.txt > abl_${x}_$i.log 2>&1 &
   done; wait)
  for i in n500m10E n700m12E n1000m15E; do
    echo "$x $i $(python3 ../../../tools/check.py ../../../instances/$i.txt abl_${x}_$i.txt | sed 's/.*: //') $(grep -h '^generations' abl_${x}_$i.log | cut -d'|' -f1)"
  done
done

#!/bin/bash
# Runs LP-only window bounds one at a time. args: list of "inst:a:b"
cd "$(dirname "$0")/.."
W=../..
for spec in "$@"; do
  IFS=: read inst a b <<< "$spec"
  sol=$(ls $W/pool/$inst | sort -t_ -k2 -n | head -1)
  log=lbruns/${inst}_${a}_${b}.log
  ( ulimit -v 10000000; timeout 2700 stdbuf -oL -eL ./winlb $W/instances/$inst.txt $W/pool/$inst/$sol \
      --window $a:$b --time 2400 --no-mip --verbose > $log 2>&1 )
  echo "$inst [$a,$b) exit=$? $(grep -E '^window' $log | sed -E 's/.*ub ([0-9]+).*LP ([0-9.]+) .*/ub=\1 LP=\2/') $(grep -m1 -E '^Model status' $log)"
done

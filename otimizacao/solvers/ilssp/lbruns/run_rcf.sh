#!/bin/bash
# Runs window bounds one at a time: LP, then (if open) reduced cost fixing + MIP. args: "inst:a:b"...
cd "$(dirname "$0")/.."
W=../..
for spec in "$@"; do
  IFS=: read inst a b <<< "$spec"
  sol=$(ls $W/pool/$inst | sort -t_ -k2 -n | head -1)
  log=lbruns/${inst}_${a}_${b}_rcf.log
  ( ulimit -v 11000000; timeout 9000 stdbuf -oL -eL ./winlb $W/instances/$inst.txt $W/pool/$inst/$sol \
      --window $a:$b --time 4000 --rcfix --verbose > $log 2>&1 )
  echo "$inst [$a,$b) exit=$? $(grep -E '^ +rcfix' $log) $(grep -E '^window' $log | sed -E 's/.*ub ([0-9]+).*(LP [0-9.]+).*(dual [-0-9.e+]+).*\| (.*)$/ub=\1 \2 \3 \4/')"
done

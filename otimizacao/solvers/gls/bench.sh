#!/usr/bin/env bash
# usage: bench.sh <tag> <inst> <time> <seeds...> -- <extra solver args>
# runs one process per seed in parallel (caller must respect the 3-thread limit)
tag=$1; inst=$2; t=$3; shift 3
seeds=(); while [ $# -gt 0 ] && [ "$1" != "--" ]; do seeds+=("$1"); shift; done; shift
mkdir -p runs
for s in "${seeds[@]}"; do
  timeout $((${t%.*}+30)) ./gls ../../instances/$inst.txt --time $t --seed $s --out runs/${tag}_${inst}_s$s.txt --report 10 "$@" > runs/${tag}_${inst}_s$s.log 2>&1 &
done
wait
for s in "${seeds[@]}"; do printf "%s %s s%s: " $tag $inst $s; grep FINAL runs/${tag}_${inst}_s$s.log | awk '{print $2}'; done

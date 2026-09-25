#!/bin/bash
# usage: bench.sh <tag> <instance> <time> <extra args...>   (runs one config, prints FINAL line)
tag=$1; inst=$2; t=$3; shift 3
D=$(dirname "$0")
mkdir -p $D/runs
timeout $((${t%.*}+60)) $D/hgs $D/../../instances/$inst.txt --time $t --out $D/runs/$tag.txt "$@" > $D/runs/$tag.log 2>&1
python3 $D/../../tools/check.py $D/../../instances/$inst.txt $D/runs/$tag.txt | sed "s|^.*runs/||"
grep -E "^generations" $D/runs/$tag.log

#!/usr/bin/env bash
# usage: batch.sh <jobfile>   ; each line: tag inst time seed [extra args...]
# runs at most 3 jobs in parallel, prints "tag inst seed cost"
job() {
  tag=$1; inst=$2; t=$3; s=$4; shift 4
  timeout $((${t%.*}+30)) ./gls ../../instances/$inst.txt --time $t --seed $s --out runs/${tag}_${inst}_s$s.txt --report 1000 "$@" > runs/${tag}_${inst}_s$s.log 2>&1
  echo "$tag $inst s$s $(grep FINAL runs/${tag}_${inst}_s$s.log | awk "{print \$2}") $(grep -o "at [0-9.]*s" runs/${tag}_${inst}_s$s.log | head -1)"
}
export -f job
mkdir -p runs
xargs -P 3 -L 1 bash -c 'job "$@"' _ < "$1"

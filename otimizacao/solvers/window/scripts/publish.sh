#!/bin/bash
# usage: publish.sh <inst> <solution file>   -- validates with check.py, publishes atomically if better than pool best
set -e
WORK=/tmp/claude-1000/-home-ddscosta--rea-de-trabalho-Faculdade-APA-APA-Project--claude-code-/5aab076d-8aac-4e11-afe9-0cef821c0125/scratchpad/work
inst=$1; f=$2
cost=$(head -1 "$f")
python3 $WORK/tools/check.py $WORK/instances/$inst.txt "$f" >/dev/null || { echo "INVALID $f"; exit 1; }
best=$(ls $WORK/pool/$inst | grep -E "^${inst}_[0-9]+" | sed -E "s/^${inst}_([0-9]+).*/\1/" | sort -n | head -1)
if [ -n "$best" ] && [ "$cost" -ge "$best" ]; then echo "not better ($cost >= $best)"; exit 0; fi
cp "$f" $WORK/pool/$inst/.tmp_window_$$
mv $WORK/pool/$inst/.tmp_window_$$ $WORK/pool/$inst/${inst}_${cost}_window.txt
echo "published $inst $cost"

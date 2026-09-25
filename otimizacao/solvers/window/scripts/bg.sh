#!/bin/bash
# usage: bg.sh <logname> <timeout_s> <args...>   -- runs ./window in background with outer timeout
cd /tmp/claude-1000/-home-ddscosta--rea-de-trabalho-Faculdade-APA-APA-Project--claude-code-/5aab076d-8aac-4e11-afe9-0cef821c0125/scratchpad/work/solvers/window
log=$1; to=$2; shift 2
nohup timeout $to ./window "$@" > logs/$log.log 2>&1 &
echo "started $log pid $!"

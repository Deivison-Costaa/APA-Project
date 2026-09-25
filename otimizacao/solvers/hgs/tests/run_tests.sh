#!/bin/bash
# Correctness tests for the hgs solver:
#  1. self-test (incremental LS state/cost == full recomputation) on every instance;
#  2. the statement example: the known solution "1 2 5 / 3 4 6" costs 2800 and the solver
#     must find a solution that the independent checker accepts;
#  3. a short run per instance whose output is validated by tools/check.py.
set -euo pipefail
D=$(cd "$(dirname "$0")/.." && pwd)
W=$(cd "$D/../.." && pwd)
CHECK="python3 $W/tools/check.py"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
fail=0

for inst in exemplo n500m10E n700m12E n1000m15E; do
    if "$D/hgs" "$W/instances/$inst.txt" --selftest > "$TMP/st.log"; then
        echo "PASS selftest $inst"
    else
        echo "FAIL selftest $inst"; cat "$TMP/st.log"; fail=1
    fi
done

# known example solution evaluated by the solver's reader/writer path (warm start, 0.2 s)
printf '2800\n1 2 5\n3 4 6\n' > "$TMP/ex.txt"
$CHECK "$W/instances/exemplo.txt" "$TMP/ex.txt" > /dev/null && echo "PASS example reference cost 2800" || { echo "FAIL example reference"; fail=1; }
if "$D/hgs" "$W/instances/exemplo.txt" --polish --init "$TMP/ex.txt" | grep -q "^polish: 2800 "; then
    echo "PASS solver evaluates the example solution at 2800"
else
    echo "FAIL solver evaluation of the example solution"; fail=1
fi
"$D/hgs" "$W/instances/exemplo.txt" --time 0.3 --init "$TMP/ex.txt" --out "$TMP/ex_out.txt" --verbose 0 > /dev/null
if $CHECK "$W/instances/exemplo.txt" "$TMP/ex_out.txt" > /dev/null; then
    echo "PASS example run ($(head -1 "$TMP/ex_out.txt"))"
else
    echo "FAIL example run"; fail=1
fi

for inst in n500m10E n700m12E n1000m15E; do
    "$D/hgs" "$W/instances/$inst.txt" --time 3 --out "$TMP/$inst.txt" --verbose 0 > /dev/null
    if $CHECK "$W/instances/$inst.txt" "$TMP/$inst.txt" > /dev/null; then
        echo "PASS short run $inst ($(head -1 "$TMP/$inst.txt"))"
    else
        echo "FAIL short run $inst"; fail=1
    fi
done
exit $fail

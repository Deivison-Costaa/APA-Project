#!/usr/bin/env bash
# Self-test: internal costs must match the independent checker, and every
# applied move delta / cache is verified inside the solver (--selftest aborts
# on the first mismatch).
set -euo pipefail
D="$(cd "$(dirname "$0")/.." && pwd)"
W="$(cd "$D/../.." && pwd)"
OUT="$D/out/selftest"
mkdir -p "$OUT"
"$D/ilssp" "$W/instances/exemplo.txt" --time 1 --no-sp --selftest --quiet --out "$OUT/exemplo.txt" >/dev/null
python3 "$W/tools/check.py" "$W/instances/exemplo.txt" "$OUT/exemplo.txt"
# the statement example: runways "1 2 5 / 3 4 6" must cost 2800
printf '2800\n1 2 5\n3 4 6\n' > "$OUT/exemplo_given.txt"
log="$("$D/ilssp" "$W/instances/exemplo.txt" --time 0.2 --no-sp --init "$OUT/exemplo_given.txt" --out "$OUT/ex2.txt")"
grep -q "init cost 2800 (claimed 2800)" <<<"$log"
echo "example init cost 2800: OK"
for i in n500m10E n700m12E n1000m15E; do
  "$D/ilssp" "$W/instances/$i.txt" --time 4 --round 2 --sp-time 1 --selftest --quiet --out "$OUT/$i.txt" >/dev/null
  python3 "$W/tools/check.py" "$W/instances/$i.txt" "$OUT/$i.txt"
done
echo "SELFTEST PASSED"

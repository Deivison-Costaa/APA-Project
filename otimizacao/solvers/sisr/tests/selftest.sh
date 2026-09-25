#!/usr/bin/env bash
# Self-test: the debug build re-evaluates the whole solution from scratch after
# every move (--selfcheck) and aborts on any mismatch; every output is then
# validated by the independent checker (tools/check.py).
set -euo pipefail
DIR="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$(cd "$DIR/../.." && pwd)"
TMP="$(mktemp -d "$DIR/runs/selftest.XXXX")"
trap 'rm -rf "$TMP"' EXIT
check() { python3 "$WORK/tools/check.py" "$WORK/instances/$1.txt" "$2"; }

echo "== example instance (statement solution costs 2800)"
"$DIR/sisr_debug" "$WORK/instances/exemplo.txt" --time 1 --selfcheck --quiet --out "$TMP/ex.txt" >/dev/null
check exemplo "$TMP/ex.txt"

for inst in n500m10E n1000m15E; do
  echo "== $inst: cold, all move types, self-check every iteration"
  "$DIR/sisr_debug" "$WORK/instances/$inst.txt" --time 6 --global 0.4 --witers 2000 --selfcheck --quiet \
    --opt 0.5 --slice 0.3 --tail 0.5 --out "$TMP/$inst.txt" >/dev/null
  check "$inst" "$TMP/$inst.txt"
  echo "== $inst: warm start keeps the initial cost"
  first=$(head -n1 "$TMP/$inst.txt")
  got=$("$DIR/sisr" "$WORK/instances/$inst.txt" --time 0.5 --init "$TMP/$inst.txt" --quiet --out "$TMP/w.txt" |
        sed -n 's/^initial cost \([0-9]*\).*/\1/p')
  [[ "$got" == "$first" ]] || { echo "warm start cost $got != $first"; exit 1; }
  check "$inst" "$TMP/w.txt"
done
echo "ALL SELF-TESTS PASSED"

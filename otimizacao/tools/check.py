#!/usr/bin/env python3
"""Independent checker for Copa APA solutions.

usage: check.py <instance.txt> <solution.txt> [more solutions...]
Solution format: first line = claimed cost, then m lines with 1-indexed flights.
Exits 1 if any solution is infeasible or the claimed cost is wrong.
"""
import sys


def read_instance(path):
    tok = open(path).read().split()
    n, m = int(tok[0]), int(tok[1])
    v = list(map(int, tok[2:]))
    if len(v) != 3 * n + n * n:
        raise ValueError(f"bad instance size in {path}")
    return n, m, v[:n], v[n:2 * n], v[2 * n:3 * n], v[3 * n:]


def check(inst, sol_path):
    n, m, r, c, p, t = inst
    lines = open(sol_path).read().split("\n")
    claimed = int(lines[0].strip())
    runways = [list(map(int, ln.split())) for ln in lines[1:1 + m]]
    while len(runways) < m:
        runways.append([])
    extra = [ln for ln in lines[1 + m:] if ln.strip()]
    if extra:
        return False, f"{len(extra)} non-empty lines beyond m={m} runways"
    seen = [0] * n
    total = 0
    for rw in runways:
        prev, end = -1, 0
        for f1 in rw:
            f = f1 - 1
            if not 0 <= f < n:
                return False, f"flight out of range: {f1}"
            seen[f] += 1
            s = max(r[f], end + (t[prev * n + f] if prev >= 0 else 0))
            total += (s - r[f]) * p[f]
            end, prev = s + c[f], f
    missing = [i + 1 for i in range(n) if seen[i] == 0]
    dup = [i + 1 for i in range(n) if seen[i] > 1]
    if missing or dup:
        return False, f"missing={missing[:5]} dup={dup[:5]}"
    if total != claimed:
        return False, f"claimed {claimed} but real cost {total}"
    return True, f"OK cost={total}"


def main():
    inst = read_instance(sys.argv[1])
    ok_all = True
    for s in sys.argv[2:]:
        ok, msg = check(inst, s)
        ok_all &= ok
        print(f"{s}: {msg}")
    sys.exit(0 if ok_all else 1)


if __name__ == "__main__":
    main()

"""Compare several solutions: delay cost per release-time bin and shared arcs.

usage: tools_profile.py <instance> <bin width> <sol1> [sol2 ...]
"""
import sys


def read_instance(path):
    tok = open(path).read().split()
    n, m = int(tok[0]), int(tok[1])
    v = list(map(int, tok[2:]))
    return n, m, v[:n], v[n:2 * n], v[2 * n:3 * n], v[3 * n:]


def delays(inst, sol):
    n, m, r, c, p, t = inst
    lines = open(sol).read().split("\n")
    rws = [list(map(int, ln.split())) for ln in lines[1:1 + m]]
    d, arcs = {}, set()
    for rw in rws:
        prev, end = -1, 0
        for f1 in rw:
            f = f1 - 1
            s = max(r[f], end + (t[prev * n + f] if prev >= 0 else 0))
            if s > r[f]:
                d[f] = (s - r[f]) * p[f]
            arcs.add((prev, f))
            end, prev = s + c[f], f
    return d, arcs


def main():
    inst = read_instance(sys.argv[1])
    r = inst[2]
    width = int(sys.argv[2])
    sols = sys.argv[3:]
    res = [delays(inst, s) for s in sols]
    print("bin     " + " ".join("%8s" % s.split('/')[-1][:8] for s in sols))
    for b in range(0, max(r) + 1, width):
        row = [sum(v for f, v in d.items() if b <= r[f] < b + width) for d, _ in res]
        if any(row):
            print("%5d   " % b + " ".join("%8d" % x for x in row))
    print("total   " + " ".join("%8d" % sum(d.values()) for d, _ in res))
    a0 = res[0][1]
    print("arcs shared with first:", [len(a0 & a) for _, a in res], "of", len(a0))


if __name__ == "__main__":
    main()

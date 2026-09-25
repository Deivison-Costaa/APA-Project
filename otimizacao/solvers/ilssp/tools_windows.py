"""Print delay clusters of a solution: merged delay intervals [r_j, S_j) and their cost."""
import sys


def read_instance(path):
    tok = open(path).read().split()
    n, m = int(tok[0]), int(tok[1])
    v = list(map(int, tok[2:]))
    return n, m, v[:n], v[n:2 * n], v[2 * n:3 * n], v[3 * n:]


def main():
    n, m, r, c, p, t = read_instance(sys.argv[1])
    gap = int(sys.argv[3]) if len(sys.argv) > 3 else 100
    lines = open(sys.argv[2]).read().split("\n")
    iv = []
    for ln in lines[1:1 + m]:
        prev, end = -1, 0
        for f1 in map(int, ln.split()):
            f = f1 - 1
            s = max(r[f], end + (t[prev * n + f] if prev >= 0 else 0))
            if s > r[f]:
                iv.append((r[f], s, p[f] * (s - r[f])))
            end, prev = s + c[f], f
    iv.sort()
    cl = []
    for lo, hi, cost in iv:
        if cl and lo <= cl[-1][1] + gap:
            cl[-1][1] = max(cl[-1][1], hi); cl[-1][2] += cost; cl[-1][3] += 1
        else:
            cl.append([lo, hi, cost, 1])
    for lo, hi, cost, k in cl:
        print(f"cluster [{lo},{hi}) cost {cost} delayed {k}")


if __name__ == "__main__":
    main()

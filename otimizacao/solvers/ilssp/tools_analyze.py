import sys
def read_instance(path):
    tok = open(path).read().split()
    n, m = int(tok[0]), int(tok[1]); v = list(map(int, tok[2:]))
    return n, m, v[:n], v[n:2*n], v[2*n:3*n], v[3*n:]
inst, sol = sys.argv[1], sys.argv[2]
n, m, r, c, p, t = read_instance(inst)
lines = open(sol).read().split("\n")
rws = [list(map(int, l.split())) for l in lines[1:1+m]]
delayed = []
for k, rw in enumerate(rws):
    prev, end = -1, 0
    for q, f1 in enumerate(rw):
        f = f1-1
        s = max(r[f], end + (t[prev*n+f] if prev >= 0 else 0))
        if s > r[f]: delayed.append((r[f], k, q, f1, s-r[f], p[f], (s-r[f])*p[f]))
        end, prev = s + c[f], f
print("runway sizes", [len(x) for x in rws])
print("delayed flights:", len(delayed), "total", sum(d[-1] for d in delayed))
for d in sorted(delayed): print("  r=%d rw=%d pos=%d f=%d delay=%d p=%d cost=%d" % d)

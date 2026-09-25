# compare cost profiles of two solutions over time buckets
import sys
def rd(path):
    tok=open(path).read().split(); n,m=int(tok[0]),int(tok[1]); v=list(map(int,tok[2:]))
    return n,m,v[:n],v[n:2*n],v[2*n:3*n],v[3*n:]
n,m,r,c,p,t=rd(sys.argv[1])
def prof(path):
    lines=open(path).read().split("\n"); rws=[list(map(int,l.split())) for l in lines[1:1+m]]
    cost={}; succ={}
    for rw in rws:
        prev=-1;end=0
        for f1 in rw:
            f=f1-1; s=max(r[f],end+(t[prev*n+f] if prev>=0 else 0))
            cost[f]=(s-r[f])*p[f]; end=s+c[f]
            if prev>=0: succ[prev]=f
            prev=f
    return cost,succ
B=int(sys.argv[4]) if len(sys.argv)>4 else 100
ca,sa=prof(sys.argv[2]); cb,sb=prof(sys.argv[3])
mx=max(r)
print("bucket  costA  costB  diffArcsA")
for b0 in range(0,mx+1,B):
    fl=[f for f in range(n) if b0<=r[f]<b0+B]
    A=sum(ca[f] for f in fl); Bc=sum(cb[f] for f in fl)
    d=sum(1 for f in fl if sa.get(f,-1)!=sb.get(f,-1))
    print(f"{b0:5d} {A:6d} {Bc:6d} {d:4d}/{len(fl)}")
print("total", sum(ca.values()), sum(cb.values()))

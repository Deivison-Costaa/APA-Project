# list delayed flights (start, flight, delay, p, cost, runway) in a time range for a solution
import sys
tok=open(sys.argv[1]).read().split(); n,m=int(tok[0]),int(tok[1]); v=list(map(int,tok[2:]))
r,c,p,t=v[:n],v[n:2*n],v[2*n:3*n],v[3*n:]
lo,hi=int(sys.argv[3]),int(sys.argv[4])
lines=open(sys.argv[2]).read().split("\n"); rws=[list(map(int,l.split())) for l in lines[1:1+m]]
out=[]
for k,rw in enumerate(rws):
    prev=-1;end=0
    for f1 in rw:
        f=f1-1; s=max(r[f],end+(t[prev*n+f] if prev>=0 else 0))
        if lo<=s<hi and s>r[f]: out.append((s,f1,s-r[f],p[f],(s-r[f])*p[f],k, prev+1))
        end=s+c[f]; prev=f
out.sort()
for o in out: print("S=%d f=%d d=%d p=%d cost=%d rw=%d pred=%d"%o)
print("sum",sum(o[4] for o in out))

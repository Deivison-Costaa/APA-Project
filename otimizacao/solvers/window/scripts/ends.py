# runway end states (last flight with S<T, finish) for a solution at cut times
import sys
tok=open(sys.argv[1]).read().split(); n,m=int(tok[0]),int(tok[1]); v=list(map(int,tok[2:]))
r,c,p,t=v[:n],v[n:2*n],v[2*n:3*n],v[3*n:]
def ends(path,T):
    lines=open(path).read().split("\n"); rws=[list(map(int,l.split())) for l in lines[1:1+m]]
    res=[]
    for rw in rws:
        prev=-1;end=0;last=None
        for f1 in rw:
            f=f1-1; s=max(r[f],end+(t[prev*n+f] if prev>=0 else 0))
            if s>=T: break
            last=(f1,s+c[f]); end=s+c[f]; prev=f
        res.append(last)
    return sorted(res)
for T in map(int,sys.argv[4:]):
    a=ends(sys.argv[2],T); b=ends(sys.argv[3],T)
    print(T, "same" if a==b else "DIFF", a if a!=b else "", b if a!=b else "")

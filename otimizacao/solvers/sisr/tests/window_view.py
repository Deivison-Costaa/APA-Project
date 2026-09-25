import sys
tok=open(sys.argv[1]).read().split(); n,m=int(tok[0]),int(tok[1]); v=list(map(int,tok[2:]))
r,c,p,t=v[:n],v[n:2*n],v[2*n:3*n],v[3*n:]
lo,hi=int(sys.argv[3]),int(sys.argv[4])
L=open(sys.argv[2]).read().split('\n'); rws=[list(map(int,x.split())) for x in L[1:1+m]]
rows=[]
for k,rw in enumerate(rws):
    prev=-1; end=0; items=[]
    for f1 in rw:
        f=f1-1; tt=(t[prev*n+f] if prev>=0 else 0); s=max(r[f], end+tt); d=s-r[f]
        if lo<=s<=hi: items.append(f"{f+1}(r{r[f]} +{d} p{p[f]} t{tt})" if d else f"{f+1}(r{r[f]} t{tt})")
        end=s+c[f]; prev=f
    rows.append(' '.join(items))
for x in sorted(rows): print(x)

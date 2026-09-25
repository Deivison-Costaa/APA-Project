import sys
tok=open(sys.argv[1]).read().split(); n,m=int(tok[0]),int(tok[1]); v=list(map(int,tok[2:]))
r,c,p,t=v[:n],v[n:2*n],v[2*n:3*n],v[3*n:]
L=open(sys.argv[2]).read().split('\n'); rws=[list(map(int,x.split())) for x in L[1:1+m]]
ev=[]
for k,rw in enumerate(rws):
    prev=-1; end=0
    for f1 in rw:
        f=f1-1; s=max(r[f], end+(t[prev*n+f] if prev>=0 else 0)); d=s-r[f]
        if d>0: ev.append((r[f],k,f,d,p[f],d*p[f]))
        end=s+c[f]; prev=f
ev.sort()
# group into clusters by time gaps > 60
cl=[]; 
for e in ev:
    if cl and e[0]-cl[-1][-1][0] <= 60: cl[-1].append(e)
    else: cl.append([e])
for g in cl:
    print(f"t={g[0][0]:5d}-{g[-1][0]:5d} cost={sum(x[5] for x in g):5d} flights={len(g)} runways={sorted(set(x[1] for x in g))} delays={[x[3] for x in g]}")

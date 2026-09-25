import sys
tok=open(sys.argv[1]).read().split(); n,m=int(tok[0]),int(tok[1]); v=list(map(int,tok[2:]))
r,c,p,t=v[:n],v[n:2*n],v[2*n:3*n],v[3*n:]
for sp in sys.argv[2:]:
    L=open(sp).read().split('\n'); rws=[list(map(int,x.split())) for x in L[1:1+m]]
    delays=[]; tot=0; seg=[]
    for rw in rws:
        prev=-1; end=0
        for f1 in rw:
            f=f1-1; s=max(r[f], end+(t[prev*n+f] if prev>=0 else 0)); d=s-r[f]; tot+=d*p[f]
            if d>0: delays.append((d,p[f],d*p[f]))
            end=s+c[f]; prev=f
    delays.sort(key=lambda x:-x[2])
    print(sp.split('/')[-1], 'cost',tot,'delayed',len(delays),'sum delay',sum(d for d,_,_ in delays),'top10',[x[2] for x in delays[:10]], 'lens',[len(x) for x in rws])
    # busy fraction

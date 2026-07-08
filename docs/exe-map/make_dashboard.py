#!/usr/bin/env python3
"""Build misplacement.html - the destructors-removed misplacement dashboard.
Reads flags.json + scatter_methods.json; emits a self-contained multi-graph page
(scatter w/ flags, cluster-share histogram, conflated-unit bars, an RVA 'genome'
of where each file's functions sit, and a distance lollipop). Vanilla JS, no deps."""
import json
import statistics as st

import os as _os; DIR = _os.path.dirname(_os.path.abspath(__file__))
flags = json.load(open(DIR + "/flags.json"))
flagged = flags["flagged"]
methods = json.load(open(DIR + "/scatter_methods.json"))

flagstate = {f["file"]: (2 if f["conflated"] else 1) for f in flagged}
SCATTER = [{"f": r["file"], "n": r["n"], "g": r["frags"],
            "s": flagstate.get(r["file"], 0)} for r in methods]

CONFLATED = [{"file": f["file"].rsplit("/", 1)[-1], "cl": f["cluster_list"]}
             for f in sorted((x for x in flagged if x["conflated"]),
                             key=lambda x: -x["n"])[:16]]

def dom_home(f):
    t = {}
    for o in f["outliers"]:
        if o["home"]:
            t[o["home"]] = t.get(o["home"], 0) + 1
    return max(t.items(), key=lambda kv: kv[1])[0].rsplit("/", 1)[-1] if t else ""

GENOME = [{"file": f["file"].rsplit("/", 1)[-1], "lo": f["main_lo"], "hi": f["main_hi"],
           "rvas": f["rvas"], "nout": len(f["outliers"]), "home": dom_home(f),
           "conf": f["conflated"]} for f in flagged[:16]]

LOLLI = sorted(({"file": f["file"].rsplit("/", 1)[-1], "name": o["name"],
                 "dist": o["dist"], "home": (o["home"] or "?").rsplit("/", 1)[-1]}
                for f in flagged for o in f["outliers"] if o["cluster_n"] <= 2),
               key=lambda o: -o["dist"])[:22]

SHARES = [f["main_share"] for f in flagged] + [1.0] * (len(methods) - len(flagged))

tiles = {
    "flagged": len(flagged),
    "misplaced": sum(len(f["outliers"]) for f in flagged),
    "conflated": sum(1 for f in flagged if f["conflated"]),
    "share": round(st.median(SHARES), 2),
}

HTML = r"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GRUNTZ.EXE misplacement finder</title>
<style>
:root{
  --surface-1:#fcfcfb;--plane:#f9f9f7;--text-primary:#0b0b0b;--text-secondary:#52514e;
  --muted:#898781;--grid:#e1e0d9;--baseline:#c3c2b7;
  --blue:#2a78d6;--green:#1baf7a;--orange:#eb6834;--red:#e34948;--seq:#256abf;
  --ring:rgba(11,11,11,0.10);
}
:root[data-theme=dark]{
  --surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;--text-secondary:#c3c2b7;
  --muted:#898781;--grid:#2c2c2a;--baseline:#383835;
  --blue:#3987e5;--green:#199e70;--orange:#d95926;--red:#e66767;--seq:#184f95;
  --ring:rgba(255,255,255,0.10);
}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){
  --surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;--text-secondary:#c3c2b7;
  --muted:#898781;--grid:#2c2c2a;--baseline:#383835;
  --blue:#3987e5;--green:#199e70;--orange:#d95926;--red:#e66767;--seq:#184f95;
  --ring:rgba(255,255,255,0.10);
}}
*{box-sizing:border-box}
body{margin:0;background:var(--plane);color:var(--text-primary);
  font-family:system-ui,-apple-system,"Segoe UI",sans-serif;line-height:1.45;padding:28px 20px 60px}
.wrap{max-width:1040px;margin:0 auto}
h1{font-size:22px;margin:0 0 4px}.sub{color:var(--text-secondary);font-size:14px;margin:0 0 14px;max-width:80ch}
.bar-top{display:flex;gap:10px;align-items:center;margin:0 0 18px;flex-wrap:wrap}
.badge{font-size:12px;font-weight:600;border-radius:999px;padding:4px 12px;background:var(--red);color:#fff}
.xlink{font-size:12.5px;color:var(--blue);text-decoration:none;border:1px solid var(--ring);border-radius:6px;padding:4px 10px}
.xlink:hover{background:var(--surface-1)}
.toggle{margin-left:auto;font-size:12px;border:1px solid var(--ring);background:var(--surface-1);color:var(--text-secondary);border-radius:6px;padding:5px 10px;cursor:pointer}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin:0 0 24px}
.tile{background:var(--surface-1);border:1px solid var(--ring);border-radius:10px;padding:14px 16px}
.tile .v{font-size:28px;font-weight:650;letter-spacing:-0.02em}.tile .k{font-size:12px;color:var(--text-secondary);margin-top:2px}
.card{background:var(--surface-1);border:1px solid var(--ring);border-radius:12px;padding:18px 18px 10px;margin-bottom:22px}
.card h2{font-size:15px;margin:0 0 2px}.card .cap{font-size:12.5px;color:var(--text-secondary);margin:0 0 10px}
.legend{display:flex;gap:16px;flex-wrap:wrap;font-size:12px;color:var(--text-secondary);margin:2px 0 8px}
.sw{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px;vertical-align:middle}
svg{width:100%;height:auto;display:block;overflow:visible}
.tick{fill:var(--muted);font-size:11px}.tick.y{text-anchor:end}.tick.x{text-anchor:middle}
.axttl{fill:var(--text-secondary);font-size:12px}
.gridln{stroke:var(--grid);stroke-width:1}
.refln{stroke:var(--baseline);stroke-width:1.5;stroke-dasharray:5 4}.reftx{fill:var(--muted);font-size:11px}
.lbl{fill:var(--text-secondary);font-size:11px}.lblv{fill:var(--text-primary);font-size:11px}
#tip{position:fixed;pointer-events:none;opacity:0;transition:opacity .08s;background:var(--surface-1);
  border:1px solid var(--ring);border-radius:8px;padding:8px 10px;font-size:12px;box-shadow:0 4px 16px rgba(0,0,0,.18);max-width:340px;z-index:9}
#tip b{font-weight:650}#tip .m{color:var(--text-secondary)}
.note{font-size:12.5px;color:var(--text-secondary);max-width:80ch}.note code{background:var(--grid);padding:1px 5px;border-radius:4px}
</style></head><body><div class="wrap">
<div class="bar-top">
  <span class="badge">misplacement finder</span>
  <a class="xlink" href="scatter_methods.html">↔ scatter (no dtors)</a>
  <a class="xlink" href="scatter.html">↔ scatter (all fns)</a>
  <button class="toggle" onclick="tog()">◐ theme</button>
</div>
<h1>GRUNTZ.EXE &mdash; functions that might be in the wrong <code style="font-size:.9em">src/</code> file</h1>
<p class="sub">Destructors (the COMDAT-pooled members) are removed; what's left should sit in one tight per-TU
block. A function far from its file's main block is flagged &mdash; either a <b>mis-attribution</b> (belongs
in another file) or a <b>conflated TU</b> (one <code style="font-size:.9em">src/</code> name covering two real
TUs). __FLAGGED__ files flagged, __MISPLACED__ functions, __CONFLATED__ conflated.</p>

<div class="tiles">
  <div class="tile"><div class="v" style="color:var(--red)">__FLAGGED__</div><div class="k">files with outlier methods<br>(of __NFILES__ analysed)</div></div>
  <div class="tile"><div class="v">__MISPLACED__</div><div class="k">functions possibly in<br>the wrong file</div></div>
  <div class="tile"><div class="v">__CONFLATED__</div><div class="k">conflated units<br>(&ge;2 blocks of &ge;3 methods)</div></div>
  <div class="tile"><div class="v">__SHARE__</div><div class="k">median main-block share<br>(1.0 = all methods in one block)</div></div>
</div>

<div class="card">
  <h2>Fragments vs function count &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(one dot per file, flagged highlighted)</span></h2>
  <div class="legend">
    <span><span class="sw" style="background:var(--blue)"></span>clean</span>
    <span><span class="sw" style="background:var(--orange)"></span>flagged (outlier methods)</span>
    <span><span class="sw" style="background:var(--red)"></span>conflated (2+ blocks)</span></div>
  <svg id="scat" viewBox="0 0 1000 520"></svg>
</div>

<div class="card">
  <h2>Where each flagged file's functions actually sit &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(RVA &ldquo;genome&rdquo;, top 16 by severity)</span></h2>
  <p class="cap">Each row is one file across the whole <code>.text</code>. <b style="color:var(--blue)">Blue</b> ticks are in the file's main block (shaded);
  <b style="color:var(--red)">red</b> ticks are outliers stranded elsewhere. The arrow names where those outliers' RVA neighbourhood says they belong.</p>
  <svg id="genome" viewBox="0 0 1000 430"></svg>
</div>

<div class="card">
  <h2>Conflated units &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(one bar = one file, split into its RVA blocks)</span></h2>
  <p class="cap">Segment width = methods in that block, ordered by RVA. <b style="color:var(--green)">Green</b> = the main block; others are separate blocks that are probably distinct TUs to split out. Hover a segment.</p>
  <svg id="conf" viewBox="0 0 1000 430"></svg>
</div>

<div class="card">
  <h2>Farthest lone outliers &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(single stranded methods &rarr; suggested home)</span></h2>
  <p class="cap">Bar = distance from the file's main block (log). These are the highest-confidence single-function moves. Hover for the full name.</p>
  <svg id="lolli" viewBox="0 0 1000 560"></svg>
</div>

<div class="card">
  <h2>Main-block share &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(all analysed files)</span></h2>
  <p class="cap">Fraction of a file's methods that fall in its single largest block. A spike at 1.0 = most files are one clean block; the left tail = split/conflated files.</p>
  <svg id="share" viewBox="0 0 1000 300"></svg>
</div>

<p class="note">Heuristic, for review &mdash; not every flag is a bug. <code>Serialize</code> methods (like destructors) are
also COMDAT-pooled, so a lone <code>Serialize</code> outlier may be a benign exile. Clustering gap 0x4000; a
function's suggested home is the file owning the methods within 0x2000 of it. Data: <code>flags.json</code>.</p>
</div>

<div id="tip"></div>
<script>
const SCATTER=__SCATTER__, GENOME=__GENOME__, CONFLATED=__CONFDATA__, LOLLI=__LOLLI__,
      SHARES=__SHARES__, TLO=__TLO__, THI=__THI__;
const CS={0:'var(--blue)',1:'var(--orange)',2:'var(--red)'};
const tip=document.getElementById('tip');
function showTip(e,h){tip.innerHTML=h;tip.style.opacity=1;let x=e.clientX+14,y=e.clientY+14;
  if(x+340>innerWidth)x=e.clientX-14-Math.min(340,tip.offsetWidth);tip.style.left=x+'px';tip.style.top=y+'px';}
function hideTip(){tip.style.opacity=0;}
function mk(t,a){const e=document.createElementNS('http://www.w3.org/2000/svg',t);for(const k in a)e.setAttribute(k,a[k]);return e;}
const kb=b=>b<1024?b+' B':b<1048576?(b/1024).toFixed(0)+' KB':(b/1048576).toFixed(2)+' MB';
const hx=v=>'0x'+v.toString(16);

// 1) scatter n vs fragments, flagged coloured
(function(){const s=scat,W=1000,H=520,L=52,R=16,T=12,B=42,pw=W-L-R,ph=H-T-B;
  const max=Math.max(...SCATTER.map(d=>d.n),2),lg=Math.log10,lm=lg(max);
  const X=v=>L+lg(v)/lm*pw,Y=v=>T+ph-lg(v)/lm*ph;
  for(const t of [1,2,5,10,20,50,100,200,500].filter(t=>t<=max)){
    s.appendChild(mk('line',{x1:X(t),y1:T,x2:X(t),y2:T+ph,class:'gridln'}));
    s.appendChild(mk('line',{x1:L,y1:Y(t),x2:L+pw,y2:Y(t),class:'gridln'}));
    let a=mk('text',{x:X(t),y:T+ph+16,class:'tick x'});a.textContent=t;s.appendChild(a);
    let b=mk('text',{x:L-8,y:Y(t)+4,class:'tick y'});b.textContent=t;s.appendChild(b);}
  s.appendChild(mk('line',{x1:X(1),y1:Y(1),x2:X(max),y2:Y(max),class:'refln'}));
  let rl=mk('text',{x:X(max)-4,y:Y(max)+16,class:'reftx','text-anchor':'end'});rl.textContent='maximally scattered';s.appendChild(rl);
  let ax=mk('text',{x:L+pw/2,y:H-4,class:'axttl','text-anchor':'middle'});ax.textContent='functions in file (log)';s.appendChild(ax);
  let ay=mk('text',{x:-(T+ph/2),y:14,class:'axttl','text-anchor':'middle',transform:'rotate(-90)'});ay.textContent='fragments (log)';s.appendChild(ay);
  for(const d of SCATTER.filter(d=>d.n>=2).sort((a,b)=>a.s-b.s)){
    const r=Math.max(3,Math.min(8,Math.sqrt(d.n)));
    const c=mk('circle',{cx:X(d.n),cy:Y(d.g),r:r,fill:CS[d.s],'fill-opacity':d.s?.85:.5,stroke:'var(--surface-1)','stroke-width':.6});
    c.addEventListener('mousemove',e=>showTip(e,`<b>${d.f}</b><br><span class="m">${d.n} methods &rarr; ${d.g} fragments</span><br>${['clean','flagged','conflated'][d.s]}`));
    c.addEventListener('mouseleave',hideTip);s.appendChild(c);}
})();

// 2) genome strips
(function(){const s=genome,W=1000,L=150,RN=132,T=6,rowH=23,n=GENOME.length,ph=n*rowH,B=26;
  const sx=v=>L+(v-TLO)/(THI-TLO)*(W-L-RN);
  for(let g=0;g<=THI;g+=0x40000){s.appendChild(mk('line',{x1:sx(g),y1:T,x2:sx(g),y2:T+ph,class:'gridln'}));
    let t=mk('text',{x:sx(g),y:T+ph+15,class:'tick x'});t.textContent=hx(g);s.appendChild(t);}
  GENOME.forEach((f,i)=>{const y=T+i*rowH,mid=y+rowH/2;
    s.appendChild(mk('rect',{x:sx(f.lo),y:y+3,width:Math.max(1,sx(f.hi)-sx(f.lo)),height:rowH-8,rx:2,fill:'var(--blue)','fill-opacity':.14}));
    let lb=mk('text',{x:8,y:mid+3,class:'lblv'});lb.textContent=f.file;
    if(f.conf)lb.setAttribute('fill','var(--red)');s.appendChild(lb);
    for(const rv of f.rvas){const inm=rv>=f.lo&&rv<=f.hi;
      s.appendChild(mk('line',{x1:sx(rv),y1:y+3,x2:sx(rv),y2:y+rowH-5,stroke:inm?'var(--blue)':'var(--red)','stroke-width':inm?1.3:2,'stroke-opacity':inm?.55:.95}));}
    let nt=mk('text',{x:W-RN+8,y:mid+3,class:'lbl'});nt.textContent='→ '+(f.home||'?')+' ('+f.nout+')';s.appendChild(nt);
    const hit=mk('rect',{x:0,y:y,width:W,height:rowH,fill:'transparent'});
    hit.addEventListener('mousemove',e=>showTip(e,`<b>${f.file}</b><br><span class="m">main block ${hx(f.lo)}–${hx(f.hi)}; ${f.nout} outlier method(s)</span><br>outliers cluster near <b>${f.home||'?'}</b>`));
    hit.addEventListener('mouseleave',hideTip);s.appendChild(hit);});
})();

// 3) conflated segmented bars
(function(){const s=conf,W=1000,L=178,R=14,T=6,rowH=24,n=CONFLATED.length,ph=n*rowH;
  const maxTot=Math.max(...CONFLATED.map(f=>f.cl.reduce((a,c)=>a+c.n,0)));
  CONFLATED.forEach((f,i)=>{const y=T+i*rowH,tot=f.cl.reduce((a,c)=>a+c.n,0),bw=(W-L-R)*tot/maxTot;
    let lb=mk('text',{x:8,y:y+rowH/2+3,class:'lblv'});lb.textContent=f.file;s.appendChild(lb);
    const mn=Math.max(...f.cl.map(c=>c.n));let x=L;
    for(const c of f.cl){const w=bw*c.n/tot;
      const r=mk('rect',{x:x+1,y:y+3,width:Math.max(0,w-2),height:rowH-8,rx:2,fill:c.n===mn?'var(--green)':'var(--orange)'});
      r.addEventListener('mousemove',e=>showTip(e,`<b>${f.file}</b><br><span class="m">${c.n} methods @ ${hx(c.lo)}</span><br>${c.n===mn?'main block':'separate block → split?'}`));
      r.addEventListener('mouseleave',hideTip);s.appendChild(r);
      if(w>16){let t=mk('text',{x:x+w/2,y:y+rowH/2+3,class:'tick x',fill:'#fff'});t.textContent=c.n;s.appendChild(t);}
      x+=w;}});
})();

// 4) lollipop of farthest lone outliers
(function(){const s=lolli,W=1000,L=300,R=90,T=8,rowH=23,n=LOLLI.length,ph=n*rowH;
  const max=Math.max(...LOLLI.map(d=>d.dist)),lg=Math.log10,x0=L,xm=W-R;
  const X=v=>x0+lg(Math.max(v,1024))/lg(max)*(xm-x0);
  for(const g of [0x10000,0x40000,0x100000]){if(g>max)continue;
    s.appendChild(mk('line',{x1:X(g),y1:T,x2:X(g),y2:T+ph,class:'gridln'}));
    let t=mk('text',{x:X(g),y:T+ph+15,class:'tick x'});t.textContent=kb(g);s.appendChild(t);}
  LOLLI.forEach((d,i)=>{const y=T+i*rowH+rowH/2;
    let lb=mk('text',{x:8,y:y+3,class:'lbl'});lb.textContent=(d.file+' :: '+d.name).slice(0,44);s.appendChild(lb);
    s.appendChild(mk('line',{x1:x0,y1:y,x2:X(d.dist),y2:y,stroke:'var(--orange)','stroke-width':2}));
    const c=mk('circle',{cx:X(d.dist),cy:y,r:4,fill:'var(--orange)'});
    c.addEventListener('mousemove',e=>showTip(e,`<b>${d.name}</b><br><span class="m">in ${d.file}</span><br>${kb(d.dist)} from its main block<br>&rarr; likely home <b>${d.home}</b>`));
    c.addEventListener('mouseleave',hideTip);s.appendChild(c);
    let hm=mk('text',{x:X(d.dist)+8,y:y+3,class:'lbl'});hm.textContent='→ '+d.home;s.appendChild(hm);});
})();

// 5) main-share histogram
(function(){const s=share,W=1000,H=300,L=52,R=16,T=10,B=40,pw=W-L-R,ph=H-T-B,NB=10;
  const bins=Array(NB).fill(0);for(const v of SHARES){bins[Math.min(NB-1,Math.floor(v*NB))]++;}
  const ymax=Math.max(...bins,1);
  for(let g=0;g<=4;g++){const yv=Math.round(ymax*g/4),y=T+ph-yv/ymax*ph;
    s.appendChild(mk('line',{x1:L,y1:y,x2:L+pw,y2:y,class:'gridln'}));
    let t=mk('text',{x:L-8,y:y+4,class:'tick y'});t.textContent=yv;s.appendChild(t);}
  const bw=pw/NB;
  for(let i=0;i<NB;i++){const h=bins[i]/ymax*ph,x=L+i*bw+3,y=T+ph-h;
    const r=mk('rect',{x:x,y:y,width:bw-6,height:Math.max(0,h),rx:3,fill:'var(--seq)'});
    r.addEventListener('mousemove',e=>showTip(e,`share <b>${(i/NB).toFixed(1)}–${((i+1)/NB).toFixed(1)}</b><br><span class="m">${bins[i]} files</span>`));
    r.addEventListener('mouseleave',hideTip);s.appendChild(r);
    if(bins[i]){let t=mk('text',{x:x+(bw-6)/2,y:y-5,class:'tick x'});t.textContent=bins[i];s.appendChild(t);}
    let xt=mk('text',{x:L+i*bw+bw/2,y:T+ph+16,class:'tick x'});xt.textContent=((i+0.5)/NB).toFixed(1);s.appendChild(xt);}
  let ax=mk('text',{x:L+pw/2,y:H-3,class:'axttl','text-anchor':'middle'});ax.textContent='main-block share (methods in largest block ÷ file methods)';s.appendChild(ax);
})();

function tog(){const r=document.documentElement,dark=matchMedia('(prefers-color-scheme:dark)').matches;
  const cur=r.getAttribute('data-theme')||(dark?'dark':'light');r.setAttribute('data-theme',cur==='dark'?'light':'dark');}
</script></body></html>"""

for k, v in {"__SCATTER__": json.dumps(SCATTER, separators=(",", ":")),
             "__GENOME__": json.dumps(GENOME, separators=(",", ":")),
             "__CONFDATA__": json.dumps(CONFLATED, separators=(",", ":")),
             "__LOLLI__": json.dumps(LOLLI, separators=(",", ":")),
             "__SHARES__": json.dumps([round(x, 3) for x in SHARES]),
             "__TLO__": str(flags["text_lo"]), "__THI__": str(flags["text_hi"]),
             "__NFILES__": str(len(methods)),
             "__FLAGGED__": str(tiles["flagged"]), "__MISPLACED__": str(tiles["misplaced"]),
             "__CONFLATED__": str(tiles["conflated"]), "__SHARE__": str(tiles["share"])}.items():
    HTML = HTML.replace(k, v)

open(DIR + "/misplacement.html", "w").write(HTML)
print("wrote misplacement.html  (%d flagged, %d misplaced fns, %d conflated)"
      % (tiles["flagged"], tiles["misplaced"], tiles["conflated"]))

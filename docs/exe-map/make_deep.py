#!/usr/bin/env python3
"""Build deep.html - the in-depth .text map: zone genome strip + original-TU
interval track + link-order ribbon + mechanism verdicts. Reads deep_layout.json
(from deep_layout.py). Vanilla JS, self-contained, same tokens as the other
exe-map pages."""
import json

import os as _os; DIR = _os.path.dirname(_os.path.abspath(__file__))
D = json.load(open(DIR + "/deep_layout.json"))

ZONE = [{"r": z["rva"], "f": z["fam"], "c": z["coh"], "cc": z["cc"]}
        for z in D["zone"]]
IVALS = [{"lo": s["lo"], "hi": s["hi"], "n": s["n"],
          "u": dict(list(s["units"].items())[:8]),
          "m": 1 if len(s["core"]) > 1 else 0} for s in D["intervals"]]
ANCH = [{"p": a["path"].rsplit("\\", 1)[-1], "h": int(any(r["header"] for r in a["refs"])),
         "r": min(r["fn"] for r in a["refs"])} for a in D["anchors"]]
HOLES = [h for h in D["gaps"]["holes"] if h["size"] >= 4096]
RUNS = D["init_table"]["runs"]
VC = D["verdict_counts"]
multi = [s for s in D["intervals"] if len(s["core"]) > 1]
MERGE = [{"lo": s["lo"], "hi": s["hi"], "n": s["n"],
          "u": dict(sorted(s["core"].items(), key=lambda kv: -kv[1]))}
         for s in sorted(multi, key=lambda s: -s["n"])[:16]]
from collections import defaultdict
ucnt = defaultdict(list)
for s in D["intervals"]:
    for u, c in s["core"].items():
        ucnt[u].append((s["lo"], c))
SPLITS = sorted(({"u": u, "iv": v} for u, v in ucnt.items() if len(v) > 1),
                key=lambda x: -len(x["iv"]))
STRAYS = sum(len(s["strays"]) for s in D["intervals"])

tiles = {
    "__THUNKS__": "2,696", "__FRAGS__": str(D["init_table"]["live"]),
    "__NULLS__": str(D["init_table"]["null"]),
    "__NIVALS__": str(len(D["intervals"])), "__MERGES__": str(len(multi)),
    "__STRAYS__": str(STRAYS), "__MOVED__": str(VC.get("ILINK-MOVED", 0)),
    "__REHOME__": str(VC.get("REHOME-CANDIDATE", 0)),
    "__POOL__": str(VC.get("COMDAT-POOL-EXILE", 0)),
    "__USAGE__": str(VC.get("COMDAT-AT-USAGE", 0)),
}

HTML = r"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GRUNTZ.EXE deep layout map</title>
<style>
:root{
  --surface-1:#fcfcfb;--plane:#f9f9f7;--text-primary:#0b0b0b;--text-secondary:#52514e;
  --muted:#898781;--grid:#e1e0d9;--baseline:#c3c2b7;
  --blue:#2a78d6;--green:#1baf7a;--orange:#eb6834;--red:#e34948;--yellow:#eda100;
  --ring:rgba(11,11,11,0.10);
}
:root[data-theme=dark]{
  --surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;--text-secondary:#c3c2b7;
  --muted:#898781;--grid:#2c2c2a;--baseline:#383835;
  --blue:#3987e5;--green:#199e70;--orange:#d95926;--red:#e66767;--yellow:#c98500;
  --ring:rgba(255,255,255,0.10);
}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){
  --surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;--text-secondary:#c3c2b7;
  --muted:#898781;--grid:#2c2c2a;--baseline:#383835;
  --blue:#3987e5;--green:#199e70;--orange:#d95926;--red:#e66767;--yellow:#c98500;
  --ring:rgba(255,255,255,0.10);
}}
*{box-sizing:border-box}
body{margin:0;background:var(--plane);color:var(--text-primary);
  font-family:system-ui,-apple-system,"Segoe UI",sans-serif;line-height:1.45;padding:28px 20px 60px}
.wrap{max-width:1040px;margin:0 auto}
h1{font-size:22px;margin:0 0 4px}.sub{color:var(--text-secondary);font-size:14px;margin:0 0 14px;max-width:88ch}
.bar-top{display:flex;gap:10px;align-items:center;margin:0 0 18px;flex-wrap:wrap}
.badge{font-size:12px;font-weight:600;border-radius:999px;padding:4px 12px;background:var(--blue);color:#fff}
.xlink{font-size:12.5px;color:var(--blue);text-decoration:none;border:1px solid var(--ring);border-radius:6px;padding:4px 10px}
.xlink:hover{background:var(--surface-1)}
.toggle{margin-left:auto;font-size:12px;border:1px solid var(--ring);background:var(--surface-1);color:var(--text-secondary);border-radius:6px;padding:5px 10px;cursor:pointer}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;margin:0 0 24px}
.tile{background:var(--surface-1);border:1px solid var(--ring);border-radius:10px;padding:14px 16px}
.tile .v{font-size:26px;font-weight:650;letter-spacing:-0.02em}.tile .k{font-size:12px;color:var(--text-secondary);margin-top:2px}
.card{background:var(--surface-1);border:1px solid var(--ring);border-radius:12px;padding:18px 18px 12px;margin-bottom:22px}
.card h2{font-size:15px;margin:0 0 2px}.card .cap{font-size:12.5px;color:var(--text-secondary);margin:0 0 10px;max-width:88ch}
.legend{display:flex;gap:16px;flex-wrap:wrap;font-size:12px;color:var(--text-secondary);margin:2px 0 8px}
.sw{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px;vertical-align:middle}
svg{width:100%;height:auto;display:block;overflow:visible}
.tick{fill:var(--muted);font-size:11px}.tick.x{text-anchor:middle}
#tip{position:fixed;pointer-events:none;opacity:0;transition:opacity .08s;background:var(--surface-1);
  border:1px solid var(--ring);border-radius:8px;padding:8px 10px;font-size:12px;box-shadow:0 4px 16px rgba(0,0,0,.18);max-width:360px;z-index:9}
#tip b{font-weight:650}#tip .m{color:var(--text-secondary)}
.note{font-size:12.5px;color:var(--text-secondary);max-width:88ch}.note code,.cap code{background:var(--grid);padding:1px 5px;border-radius:4px}
.ribbon{display:flex;flex-wrap:wrap;gap:4px;font-size:11.5px;max-height:300px;overflow-y:auto;padding:2px}
.chip{border:1px solid var(--ring);border-radius:5px;padding:1px 7px;background:var(--plane);color:var(--text-secondary);white-space:nowrap}
.chip b{color:var(--text-primary);font-weight:600}
.chip.q{opacity:.45}
table{border-collapse:collapse;font-size:12.5px;width:100%}
th{text-align:left;color:var(--text-secondary);font-weight:600;padding:4px 10px 4px 0;border-bottom:1px solid var(--grid)}
td{padding:4px 10px 4px 0;border-bottom:1px solid var(--grid);vertical-align:top}
td code{background:var(--grid);padding:0 4px;border-radius:3px;font-size:11.5px}
details summary{cursor:pointer;font-size:12.5px;color:var(--text-secondary);margin:6px 0}
.vrow{display:flex;gap:10px;align-items:baseline;margin:6px 0}
.vtag{font-size:11.5px;font-weight:650;border-radius:5px;padding:2px 8px;color:#fff;white-space:nowrap}
.vn{font-weight:650}
.vdesc{font-size:12.5px;color:var(--text-secondary)}
</style></head><body><div class="wrap">
<div class="bar-top">
  <span class="badge">deep map</span>
  <a class="xlink" href="misplacement.html">&harr; misplacement finder</a>
  <a class="xlink" href="scatter_methods.html">&harr; scatter</a>
  <a class="xlink" href="homm2.html">&harr; HoMM2 baseline</a>
  <button class="toggle" onclick="tog()">&#9681; theme</button>
</div>
<h1>GRUNTZ.EXE &mdash; deep <code style="font-size:.9em">.text</code> layout &amp; the original-TU partition</h1>
<p class="sub">The retail EXE was linked <b>/INCREMENTAL</b> (ILT jmp-thunk forest, int3 contribution pads,
<code>.reloc</code> kept &mdash; the reason it is delinkable). The demo oracle proved function placements are
<b>first-link birth positions</b> (170/181 outliers sit identically in GruntDem.exe; only __MOVED__ true ilink moves
exist). So <code>.text</code> order is a faithful record of the ORIGINAL TU composition, the CRT init-table gives the
original obj <b>link order</b>, and the interval partition below reads the original file boundaries back out of the binary.
Actionable output: <code>TU_MIGRATION.md</code>.</p>

<div class="tiles">
  <div class="tile"><div class="v">__THUNKS__</div><div class="k">ILT jmp thunks at .text start</div></div>
  <div class="tile"><div class="v">__FRAGS__</div><div class="k">$E init fragments (link-order fossils; __NULLS__ slots zeroed)</div></div>
  <div class="tile"><div class="v">__NIVALS__</div><div class="k">original-TU intervals</div></div>
  <div class="tile"><div class="v" style="color:var(--orange)">__MERGES__</div><div class="k">merge groups (several src files = one original TU)</div></div>
  <div class="tile"><div class="v" style="color:var(--red)">__STRAYS__</div><div class="k">stray functions to re-home</div></div>
  <div class="tile"><div class="v">__MOVED__</div><div class="k">functions actually moved by ilink</div></div>
</div>

<div class="card">
  <h2>The genome strip</h2>
  <p class="cap">Top lane: dominant code family per 8&thinsp;KB bin. Middle: the __NIVALS__ reconstructed original-TU
  intervals (<b style="color:var(--orange)">orange</b> = merge group, <b style="color:var(--blue)">blue</b> = single-unit).
  Pins are <code>__FILE__</code> assert-string anchors (&#9650; = .cpp, &#9651; = header &rArr; inline-at-usage);
  hatched = int3 holes &ge;4&thinsp;KB. Hover everything.</p>
  <div class="legend">
    <span><span class="sw" style="background:var(--blue)"></span>game (src/Gruntz)</span>
    <span><span class="sw" style="background:var(--orange)"></span>engine (WAP32 modules)</span>
    <span><span class="sw" style="background:var(--green)"></span>CRT / MFC / zlib</span>
    <span><span class="sw" style="background:var(--yellow)"></span>ILT thunks</span>
    <span><span class="sw" style="background:var(--muted)"></span>unreconstructed</span>
    <span><span class="sw" style="background:repeating-linear-gradient(45deg,var(--baseline),var(--baseline) 2px,transparent 2px,transparent 5px)"></span>int3 hole</span>
  </div>
  <svg id="strip" viewBox="0 0 1000 150" role="img" aria-label="dominant code family and original-TU intervals across .text"></svg>
  <details><summary>table view (per-64KB summary)</summary><div id="ztable"></div></details>
</div>

<div class="card">
  <h2>Original link order &mdash; the init-table skeleton</h2>
  <p class="cap">Compressed unit sequence of the attributed $E initializer fragments, in CRT-table order
  (= original obj link order; fragments never grow, so ilink never moved them). Greyed chips are
  unattributed runs (unreconstructed neighborhoods). Hover a chip for its first fragment RVA.</p>
  <div class="ribbon" id="ribbon"></div>
</div>

<div class="card">
  <h2>Why functions sit outside their TU block &mdash; measured verdicts</h2>
  <div class="vrow"><span class="vtag" style="background:var(--red)">REHOME &middot; __REHOME__</span>
    <span class="vdesc">stable in both links, sitting in another TU's coherent run &mdash; the ORIGINAL source had them in that file; move ours to match (see TU_MIGRATION.md)</span></div>
  <div class="vrow"><span class="vtag" style="background:var(--baseline);color:var(--text-primary)">POOL EXILE &middot; __POOL__</span>
    <span class="vdesc">??1/??_G/??_E destructors, GetTypeTag, Serialize &mdash; COMDATs the linker keeps from whichever obj came first; benign, exclude from layout reasoning</span></div>
  <div class="vrow"><span class="vtag" style="background:var(--green)">AT USAGE &middot; __USAGE__</span>
    <span class="vdesc">header-defined inlines kept at the first REFERENCING obj in link order (proven by <code>incs\ddrawmgr.h</code> / <code>incs\netmgr.h</code> assert strings inside game blocks)</span></div>
  <div class="vrow"><span class="vtag" style="background:var(--blue)">ILINK MOVED &middot; __MOVED__</span>
    <span class="vdesc">at home in the demo link, relocated in retail &mdash; the ONLY true incremental-linker scatter in the whole EXE</span></div>
</div>

<div class="card">
  <h2>Largest merge groups (several of our units = one original TU)</h2>
  <div id="mtable"></div>
  <details><summary>conflated units (core presence in several intervals &rarr; split)</summary><div id="stable"></div></details>
  <p class="note">Full per-function instructions: <code>TU_MIGRATION.md</code> &middot; raw data: <code>deep_layout.json</code>.
  The 0x1396f0 engine-resource mega-interval is glued by our own coarse units &mdash; the DIRSURF / DDRAWMGR / DIRPAL
  <code>__FILE__</code> anchors mark three real files inside it; it needs per-function re-attribution before it splits.</p>
</div>

<div id="tip"></div>
<script>
const ZONE=__ZONE__,IVALS=__IVALS__,ANCH=__ANCH__,HOLES=__HOLES__,RUNS=__RUNS__,
      MERGE=__MERGE__,SPLITS=__SPLITS__,TLO=__TLO__,THI=__THI__;
const tip=document.getElementById('tip');
function showTip(e,html){tip.innerHTML=html;tip.style.opacity=1;
  const w=tip.offsetWidth,x=Math.min(e.clientX+14,innerWidth-w-8);
  tip.style.left=x+'px';tip.style.top=(e.clientY+14)+'px';}
function hideTip(){tip.style.opacity=0;}
function mk(t,a){const el=document.createElementNS('http://www.w3.org/2000/svg',t);
  for(const k in a)el.setAttribute(k,a[k]);return el;}
const hx=v=>'0x'+v.toString(16);
const FAMC={game:'var(--blue)',engine:'var(--orange)',library:'var(--green)',
            thunk:'var(--yellow)',unknown:'var(--muted)'};

(function(){ // genome strip
  const s=document.getElementById('strip'),W=1000,X=r=>(r-TLO)/(THI-TLO)*W;
  const defs=mk('defs',{});defs.innerHTML='<pattern id="hatch" width="5" height="5" patternTransform="rotate(45)" patternUnits="userSpaceOnUse"><rect width="5" height="5" fill="transparent"/><rect width="2" height="5" fill="var(--baseline)"/></pattern>';
  s.appendChild(defs);
  // family lane
  for(const z of ZONE){
    let dom='unknown',best=-1,tot=0;
    for(const f in z.f){tot+=z.f[f];if(z.f[f]>best){best=z.f[f];dom=f;}}
    if(!tot)continue;
    const x=X(z.r),w=Math.max(0.6,X(z.r+0x2000)-x-0.4);
    const r=mk('rect',{x:x,y:14,width:w,height:34,fill:FAMC[dom]||'var(--muted)',rx:1});
    const fam=Object.entries(z.f).map(([k,v])=>`${k} ${(v/1024).toFixed(1)}K`).join(' · ');
    const coh=z.c==null?'':`<br><span class="m">unit-coherence ${z.c}</span>`;
    r.addEventListener('mousemove',e=>showTip(e,`<b>${hx(z.r)}</b><br><span class="m">${fam}</span>${coh}<br><span class="m">${z.cc} pad bytes</span>`));
    r.addEventListener('mouseleave',hideTip);s.appendChild(r);
  }
  // holes
  for(const h of HOLES){
    const x=X(h.rva),w=Math.max(1.2,X(h.rva+h.size)-x);
    const r=mk('rect',{x:x,y:14,width:w,height:34,fill:'url(#hatch)'});
    r.addEventListener('mousemove',e=>showTip(e,`<b>int3 hole</b> ${hx(h.rva)}<br><span class="m">${(h.size/1024).toFixed(1)} KB of 0xCC</span>`));
    r.addEventListener('mouseleave',hideTip);s.appendChild(r);
  }
  // interval lane (alternate two rows to keep tiny ones hoverable)
  IVALS.forEach((v,i)=>{
    const x=X(v.lo),w=Math.max(1,X(v.hi)-x-0.3),y=58+(i%2)*16;
    const r=mk('rect',{x:x,y:y,width:w,height:13,rx:2,
      fill:v.m?'var(--orange)':'var(--blue)',opacity:v.m?0.95:0.55});
    const us=Object.entries(v.u).map(([k,c])=>`${k} (${c})`).join(', ');
    r.addEventListener('mousemove',e=>showTip(e,`<b>${hx(v.lo)}&ndash;${hx(v.hi)}</b> · ${v.n} fns${v.m?' · <b>merge group</b>':''}<br><span class="m">${us}</span>`));
    r.addEventListener('mouseleave',hideTip);s.appendChild(r);
  });
  // anchors
  for(const a of ANCH){
    const x=X(a.r);
    const p=mk('path',{d:`M ${x-4.5} 100 L ${x+4.5} 100 L ${x} 91 Z`,
      fill:a.h?'transparent':'var(--red)',stroke:'var(--red)','stroke-width':1.4});
    p.addEventListener('mousemove',e=>showTip(e,`<b>${a.p}</b><br><span class="m">__FILE__ anchor${a.h?' (header &rArr; inline-at-usage)':''} · ${hx(a.r)}</span>`));
    p.addEventListener('mouseleave',hideTip);s.appendChild(p);
  }
  // axis
  for(let r=0;r<=THI;r+=0x40000){
    const x=X(Math.max(r,TLO));
    s.appendChild(mk('line',{x1:x,y1:104,x2:x,y2:109,stroke:'var(--baseline)'}));
    const t=mk('text',{x:x,y:122,class:'tick x'});t.textContent=hx(r);s.appendChild(t);
  }
  const cap=mk('text',{x:0,y:12,class:'tick'});cap.textContent='code family';s.appendChild(cap);
  const cap2=mk('text',{x:0,y:56,class:'tick'});cap2.textContent='original-TU intervals';s.appendChild(cap2);
  // per-64KB table view
  const rows=[];
  for(let b=TLO&~0xffff;b<THI;b+=0x10000){
    const zs=ZONE.filter(z=>z.r>=b&&z.r<b+0x10000);
    const agg={},cc=zs.reduce((a,z)=>a+z.cc,0);
    zs.forEach(z=>{for(const f in z.f)agg[f]=(agg[f]||0)+z.f[f];});
    const fam=Object.entries(agg).sort((a,b2)=>b2[1]-a[1]).map(([k,v])=>`${k} ${(v/1024)|0}K`).join(', ');
    if(fam)rows.push(`<tr><td><code>${hx(b)}</code></td><td>${fam}</td><td>${(cc/1024).toFixed(1)}K</td></tr>`);
  }
  document.getElementById('ztable').innerHTML=
    `<table><tr><th>64KB</th><th>family bytes</th><th>int3 pad</th></tr>${rows.join('')}</table>`;
})();

(function(){ // ribbon
  const el=document.getElementById('ribbon');
  for(const r of RUNS){
    const c=document.createElement('span');
    c.className='chip'+(r.unit==='?'?' q':'');
    c.innerHTML=r.n>1?`<b>${r.unit}</b>&times;${r.n}`:`<b>${r.unit}</b>`;
    c.addEventListener('mousemove',e=>showTip(e,`<b>${r.unit}</b> · ${r.n} initializer${r.n>1?'s':''}<br><span class="m">first fragment ${hx(r.rva)}</span>`));
    c.addEventListener('mouseleave',hideTip);
    el.appendChild(c);
  }
})();

(function(){ // merge + split tables
  const m=MERGE.map(g=>`<tr><td><code>${hx(g.lo)}&ndash;${hx(g.hi)}</code></td><td>${g.n}</td>
    <td>${Object.entries(g.u).map(([k,c])=>`${k} (${c})`).join(', ')}</td></tr>`).join('');
  document.getElementById('mtable').innerHTML=
    `<table><tr><th>interval</th><th>fns</th><th>combine these units</th></tr>${m}</table>`;
  const sp=SPLITS.map(s=>`<tr><td>${s.u}</td><td>${s.iv.map(([lo,c])=>`<code>${hx(lo)}</code> (${c})`).join(', ')}</td></tr>`).join('');
  document.getElementById('stable').innerHTML=
    `<table><tr><th>unit</th><th>core intervals</th></tr>${sp}</table>`;
})();

function tog(){const r=document.documentElement,dark=matchMedia('(prefers-color-scheme:dark)').matches;
  const cur=r.getAttribute('data-theme')||(dark?'dark':'light');r.setAttribute('data-theme',cur==='dark'?'light':'dark');}
</script></body></html>"""

for k, v in {"__ZONE__": json.dumps(ZONE, separators=(",", ":")),
             "__IVALS__": json.dumps(IVALS, separators=(",", ":")),
             "__ANCH__": json.dumps(ANCH, separators=(",", ":")),
             "__HOLES__": json.dumps(HOLES, separators=(",", ":")),
             "__RUNS__": json.dumps(RUNS, separators=(",", ":")),
             "__MERGE__": json.dumps(MERGE, separators=(",", ":")),
             "__SPLITS__": json.dumps([{"u": s["u"], "iv": s["iv"]} for s in SPLITS],
                                      separators=(",", ":")),
             "__TLO__": str(D["text"]["lo"]), "__THI__": str(D["text"]["hi"]),
             **tiles}.items():
    HTML = HTML.replace(k, v)

open(DIR + "/deep.html", "w").write(HTML)
print("wrote deep.html  (%s intervals, %s merge groups, %s strays)"
      % (tiles["__NIVALS__"], tiles["__MERGES__"], tiles["__STRAYS__"]))

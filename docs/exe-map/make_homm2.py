#!/usr/bin/env python3
"""Build homm2.html - the ground-truth scatter baseline, overlaid against Gruntz.

HoMM2 (HEROES2W) has a real CodeView debug stream, so its file grouping is TRUTH;
its TUs are 100% contiguous (the linker doesn't scatter correctly-grouped, non-/Gy
code). Gruntz is our reconstructed grouping. Overlaying the two on one n-vs-fragments
plot shows how much of Gruntz's 'scatter' is the linker vs our grouping.

Reads the snapshot docs/exe-map/homm2_va.csv (produced by homm2_baseline.py) and
Gruntz's scatter_methods.json (dtors removed = the fairest grouping-quality view)."""
import csv
import json
import os
import statistics as st
from collections import defaultdict

DIR = os.path.dirname(os.path.abspath(__file__))

if not os.path.isfile(os.path.join(DIR, "homm2_va.csv")):
    print("homm2_va.csv absent (run homm2_baseline.py with homm2-decomp present); "
          "skipping homm2.html")
    raise SystemExit(0)


def homm2_rows():
    rows = []
    with open(os.path.join(DIR, "homm2_va.csv")) as f:
        for r in csv.DictReader(f):
            rows.append({"rva": int(r["rva"], 16), "unit": r["unit"],
                         "opt": r["opt"], "size": int(r["size"], 16)})
    rows.sort(key=lambda r: r["rva"])
    stats = defaultdict(lambda: {"n": 0, "frags": 0, "opt": ""})
    prev = None
    for r in rows:
        s = stats[r["unit"]]
        s["n"] += 1
        s["opt"] = r["opt"]
        if r["unit"] != prev:
            s["frags"] += 1
        prev = r["unit"]
    return [{"n": s["n"], "g": s["frags"], "r": round(s["frags"] / s["n"], 3),
             "opt": s["opt"], "f": u}
            for u, s in stats.items() if s["opt"]]  # real TUs only


HOMM2 = homm2_rows()
GRUNTZ = [{"n": r["n"], "g": r["frags"], "r": round(r["frag_ratio"], 3), "f": r["file"]}
          for r in json.load(open(os.path.join(DIR, "scatter_methods.json")))]


def pct_contig(rows, mn=5):
    sub = [r for r in rows if r["n"] >= mn]
    return round(100 * sum(1 for r in sub if r["g"] == 1) / len(sub)) if sub else 0


def med_run(rows, mn=5):
    sub = [r["n"] / r["g"] for r in rows if r["n"] >= mn]
    return round(st.median(sub), 1) if sub else 0


tiles = {
    "h_tus": len(HOMM2),
    "h_contig": pct_contig(HOMM2),
    "g_contig": pct_contig(GRUNTZ),
    "h_run": med_run(HOMM2),
    "g_run": med_run(GRUNTZ),
}

HTML = r"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HoMM2 vs Gruntz — linker scatter baseline</title>
<style>
:root{--surface-1:#fcfcfb;--plane:#f9f9f7;--text-primary:#0b0b0b;--text-secondary:#52514e;
  --muted:#898781;--grid:#e1e0d9;--baseline:#c3c2b7;--blue:#2a78d6;--orange:#eb6834;--ring:rgba(11,11,11,0.10);}
:root[data-theme=dark]{--surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;--text-secondary:#c3c2b7;
  --muted:#898781;--grid:#2c2c2a;--baseline:#383835;--blue:#3987e5;--orange:#d95926;--ring:rgba(255,255,255,0.10);}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){--surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;
  --text-secondary:#c3c2b7;--muted:#898781;--grid:#2c2c2a;--baseline:#383835;--blue:#3987e5;--orange:#d95926;--ring:rgba(255,255,255,0.10);}}
*{box-sizing:border-box}
body{margin:0;background:var(--plane);color:var(--text-primary);font-family:system-ui,-apple-system,"Segoe UI",sans-serif;line-height:1.45;padding:28px 20px 60px}
.wrap{max-width:1000px;margin:0 auto}
h1{font-size:22px;margin:0 0 4px}.sub{color:var(--text-secondary);font-size:14px;margin:0 0 14px;max-width:82ch}
.bar-top{display:flex;gap:10px;align-items:center;margin:0 0 18px;flex-wrap:wrap}
.badge{font-size:12px;font-weight:600;border-radius:999px;padding:4px 12px;background:var(--blue);color:#fff}
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
.axttl{fill:var(--text-secondary);font-size:12px}.gridln{stroke:var(--grid);stroke-width:1}
.refln{stroke:var(--baseline);stroke-width:1.5;stroke-dasharray:5 4}.reftx{fill:var(--muted);font-size:11px}
#tip{position:fixed;pointer-events:none;opacity:0;transition:opacity .08s;background:var(--surface-1);border:1px solid var(--ring);border-radius:8px;padding:8px 10px;font-size:12px;box-shadow:0 4px 16px rgba(0,0,0,.18);max-width:320px;z-index:9}
#tip b{font-weight:650}#tip .m{color:var(--text-secondary)}
.note{font-size:12.5px;color:var(--text-secondary);max-width:82ch}.note code{background:var(--grid);padding:1px 5px;border-radius:4px}
</style></head><body><div class="wrap">
<div class="bar-top">
  <span class="badge">HoMM2 baseline</span>
  <a class="xlink" href="scatter_methods.html">↔ Gruntz scatter</a>
  <a class="xlink" href="misplacement.html">↔ Gruntz misplacement</a>
  <button class="toggle" onclick="tog()">◐ theme</button>
</div>
<h1>How much does the linker actually scatter a file? &nbsp;<span style="font-weight:400;color:var(--text-secondary)">HoMM2 (truth) vs Gruntz (reconstructed)</span></h1>
<p class="sub"><b>HoMM2 (HEROES2W)</b> ships a real CodeView debug stream, so its function&rarr;file grouping is
<b>ground truth</b>. Its TUs are <b>100% contiguous</b> — the MSVC linker lays each whole-TU block down in
order and scatters nothing (that build uses no <code>/Gy</code>). <b>Gruntz</b> is our <i>reconstructed</i>
grouping (destructors removed). The gap between the two is what our grouping still gets wrong.</p>

<div class="tiles">
  <div class="tile"><div class="v" style="color:var(--blue)">__HCONTIG__%</div><div class="k">HoMM2 files contiguous<br>(&ge;5 fns, ground truth)</div></div>
  <div class="tile"><div class="v" style="color:var(--orange)">__GCONTIG__%</div><div class="k">Gruntz files contiguous<br>(&ge;5 fns, reconstructed)</div></div>
  <div class="tile"><div class="v">__HRUN__</div><div class="k">HoMM2 median contiguous run<br>(functions per block)</div></div>
  <div class="tile"><div class="v">__GRUN__</div><div class="k">Gruntz median contiguous run<br>(functions per block)</div></div>
</div>

<div class="card">
  <h2>Fragments vs function count &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(one dot per file, both games)</span></h2>
  <div class="legend">
    <span><span class="sw" style="background:var(--blue)"></span>HoMM2 (ground truth) — hugs the bottom (1 block)</span>
    <span><span class="sw" style="background:var(--orange)"></span>Gruntz (reconstructed) — climbs toward the diagonal</span></div>
  <p class="cap">On the dashed line = every function isolated (max scatter); at the bottom (fragments = 1) = one contiguous block. HoMM2 sits on the floor; Gruntz rises off it.</p>
  <svg id="scat" viewBox="0 0 1000 520"></svg>
</div>

<div class="card">
  <h2>Fragmentation-ratio distribution &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(% of each game's files, &ge;5 fns)</span></h2>
  <p class="cap">HoMM2 is spiked at the left (0.0–0.2 = near-contiguous). Gruntz spreads right (fragmented).</p>
  <svg id="hist" viewBox="0 0 1000 320"></svg>
</div>

<p class="note">Not a perfectly matched pair, and the reason is the <b>toolchain</b>. HoMM2 was built with <b>MSVC 4.2</b>,
whose <code>/O2</code> does <b>not</b> force function-level COMDATs, so each TU is one whole-<code>.text</code>
block the linker can't split (measured: only <b>39%</b> of its functions sit on a 16-byte boundary). Retail
Gruntz was built with <b>MSVC 5.0</b>, whose <code>/O2</code> <b>forces per-function COMDATs on</b> — proven by
byte-exact zlib COMDATs (<code>../zlib-matching.md</code>) and confirmed here: <b>100%</b> of Gruntz functions are
16-byte aligned, each in its own section. So Gruntz's functions are independently orderable and its special
members (destructors) get pooled away from their TU, whereas HoMM2's can't move at all. HoMM2 is therefore the
<i>no-COMDAT floor</i> — proof the linker doesn't scatter whole-TU code — not an exact model of Gruntz's build.
The Gruntz series has destructors removed to isolate grouping quality; the residual gap above HoMM2 is our
grouping errors. Data: <code>homm2_va.csv</code> (read-only snapshot) + <code>scatter_methods.json</code>.</p>
</div>

<div id="tip"></div>
<script>
const HOMM2=__HOMM2__, GRUNTZ=__GRUNTZ__;
const tip=document.getElementById('tip');
function showTip(e,h){tip.innerHTML=h;tip.style.opacity=1;let x=e.clientX+14,y=e.clientY+14;
  if(x+320>innerWidth)x=e.clientX-14-Math.min(320,tip.offsetWidth);tip.style.left=x+'px';tip.style.top=y+'px';}
function hideTip(){tip.style.opacity=0;}
function mk(t,a){const e=document.createElementNS('http://www.w3.org/2000/svg',t);for(const k in a)e.setAttribute(k,a[k]);return e;}

// overlaid scatter
(function(){const s=scat,W=1000,H=520,L=52,R=16,T=12,B=42,pw=W-L-R,ph=H-T-B;
  const max=Math.max(...HOMM2.map(d=>d.n),...GRUNTZ.map(d=>d.n),2),lg=Math.log10,lm=lg(max);
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
  function plot(data,color,game){for(const d of data.filter(d=>d.n>=2)){
    // jitter overlapping bottom dots a hair so the pile at frags=1 is visible
    const jy=(game==='HoMM2'?-1:1)*((d.n%3))*0.6;
    const c=mk('circle',{cx:X(d.n),cy:Y(d.g)+jy,r:Math.max(3,Math.min(8,Math.sqrt(d.n))),fill:color,'fill-opacity':.5,stroke:'var(--surface-1)','stroke-width':.6});
    c.addEventListener('mousemove',e=>showTip(e,`<b>${d.f}</b><br><span class="m">${game}: ${d.n} fns &rarr; ${d.g} fragment(s)</span>`));
    c.addEventListener('mouseleave',hideTip);s.appendChild(c);}}
  plot(GRUNTZ,'var(--orange)','Gruntz');   // draw Gruntz first, HoMM2 on top
  plot(HOMM2,'var(--blue)','HoMM2');
})();

// grouped histogram (% of each dataset per ratio bin)
(function(){const s=hist,W=1000,H=320,L=48,R=16,T=10,B=42,pw=W-L-R,ph=H-T-B,NB=10;
  const bins=g=>{const b=Array(NB).fill(0),d=g.filter(x=>x.n>=5);
    for(const x of d)b[Math.min(NB-1,Math.floor(x.r*NB))]++;return b.map(v=>d.length?100*v/d.length:0);};
  const H2=bins(HOMM2),GR=bins(GRUNTZ),ymax=Math.max(...H2,...GR,1);
  for(let g=0;g<=4;g++){const yv=Math.round(ymax*g/4),y=T+ph-yv/ymax*ph;
    s.appendChild(mk('line',{x1:L,y1:y,x2:L+pw,y2:y,class:'gridln'}));
    let t=mk('text',{x:L-8,y:y+4,class:'tick y'});t.textContent=yv+'%';s.appendChild(t);}
  const bw=pw/NB;
  for(let i=0;i<NB;i++){const x=L+i*bw;
    const pairs=[[H2[i],'var(--blue)','HoMM2'],[GR[i],'var(--orange)','Gruntz']];
    pairs.forEach(([v,col,nm],k)=>{const w=(bw-6)/2,bx=x+3+k*w,h=v/ymax*ph,by=T+ph-h;
      const r=mk('rect',{x:bx,y:by,width:w-1,height:Math.max(0,h),rx:2,fill:col,'fill-opacity':.9});
      r.addEventListener('mousemove',e=>showTip(e,`${nm} &middot; ratio <b>${(i/NB).toFixed(1)}–${((i+1)/NB).toFixed(1)}</b><br><span class="m">${v.toFixed(0)}% of files</span>`));
      r.addEventListener('mouseleave',hideTip);s.appendChild(r);});
    let xt=mk('text',{x:x+bw/2,y:T+ph+16,class:'tick x'});xt.textContent=((i+0.5)/NB).toFixed(1);s.appendChild(xt);}
  let ax=mk('text',{x:L+pw/2,y:H-4,class:'axttl','text-anchor':'middle'});ax.textContent='fragmentation ratio (1.0 = fully scattered)';s.appendChild(ax);
})();

function tog(){const r=document.documentElement,dark=matchMedia('(prefers-color-scheme:dark)').matches;
  const cur=r.getAttribute('data-theme')||(dark?'dark':'light');r.setAttribute('data-theme',cur==='dark'?'light':'dark');}
</script></body></html>"""

for k, v in {"__HOMM2__": json.dumps(HOMM2, separators=(",", ":")),
             "__GRUNTZ__": json.dumps(GRUNTZ, separators=(",", ":")),
             "__HCONTIG__": str(tiles["h_contig"]), "__GCONTIG__": str(tiles["g_contig"]),
             "__HRUN__": str(tiles["h_run"]), "__GRUN__": str(tiles["g_run"])}.items():
    HTML = HTML.replace(k, v)
open(os.path.join(DIR, "homm2.html"), "w").write(HTML)
print("wrote homm2.html  (HoMM2 %d TUs %d%% contiguous vs Gruntz %d%%; runs %.1f vs %.1f)"
      % (tiles["h_tus"], tiles["h_contig"], tiles["g_contig"], tiles["h_run"], tiles["g_run"]))

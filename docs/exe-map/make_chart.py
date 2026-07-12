#!/usr/bin/env python3
"""Build the scatterness charts from the scatter*.json datasets - self-contained
HTML (inline data + vanilla-JS SVG + hover tooltips, light/dark). Two views:
  scatter.html          all functions
  scatter_methods.html  pooled functions removed (dtors + CRT dynamic-init fragments)"""
import json
import statistics as st

import os as _os; DIR = _os.path.dirname(_os.path.abspath(__file__))

TEMPLATE = r"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GRUNTZ.EXE .text scatterness __TITLETAG__</title>
<style>
:root{
  --surface-1:#fcfcfb; --plane:#f9f9f7; --text-primary:#0b0b0b; --text-secondary:#52514e;
  --muted:#898781; --grid:#e1e0d9; --baseline:#c3c2b7; --series-1:#2a78d6;
  --seq-500:#256abf; --ring:rgba(11,11,11,0.10); --accent:#1baf7a;
}
:root[data-theme=dark]{
  --surface-1:#1a1a19; --plane:#0d0d0d; --text-primary:#fff; --text-secondary:#c3c2b7;
  --muted:#898781; --grid:#2c2c2a; --baseline:#383835; --series-1:#3987e5;
  --seq-500:#184f95; --ring:rgba(255,255,255,0.10); --accent:#199e70;
}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){
  --surface-1:#1a1a19; --plane:#0d0d0d; --text-primary:#fff; --text-secondary:#c3c2b7;
  --muted:#898781; --grid:#2c2c2a; --baseline:#383835; --series-1:#3987e5;
  --seq-500:#184f95; --ring:rgba(255,255,255,0.10); --accent:#199e70;
}}
*{box-sizing:border-box}
body{margin:0;background:var(--plane);color:var(--text-primary);
  font-family:system-ui,-apple-system,"Segoe UI",sans-serif;line-height:1.45;padding:28px 20px 60px}
.wrap{max-width:1000px;margin:0 auto}
h1{font-size:22px;margin:0 0 4px} .sub{color:var(--text-secondary);font-size:14px;margin:0 0 14px;max-width:74ch}
.bar-top{display:flex;gap:10px;align-items:center;margin:0 0 18px;flex-wrap:wrap}
.badge{font-size:12px;font-weight:600;border-radius:999px;padding:4px 12px;background:var(--accent);color:#fff}
.xlink{font-size:12.5px;color:var(--series-1);text-decoration:none;border:1px solid var(--ring);
  border-radius:6px;padding:4px 10px} .xlink:hover{background:var(--surface-1)}
.toggle{margin-left:auto;font-size:12px;border:1px solid var(--ring);background:var(--surface-1);
  color:var(--text-secondary);border-radius:6px;padding:5px 10px;cursor:pointer}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin:0 0 26px}
.tile{background:var(--surface-1);border:1px solid var(--ring);border-radius:10px;padding:14px 16px}
.tile .v{font-size:28px;font-weight:650;letter-spacing:-0.02em}
.tile .k{font-size:12px;color:var(--text-secondary);margin-top:2px}
.card{background:var(--surface-1);border:1px solid var(--ring);border-radius:12px;padding:18px 18px 8px;margin-bottom:22px}
.card h2{font-size:15px;margin:0 0 2px} .card .cap{font-size:12.5px;color:var(--text-secondary);margin:0 0 10px}
svg{width:100%;height:auto;display:block;overflow:visible}
.tick{fill:var(--muted);font-size:11px} .tick.y{text-anchor:end} .tick.x{text-anchor:middle}
.axttl{fill:var(--text-secondary);font-size:12px}
.dot{fill:var(--series-1);fill-opacity:.55;stroke:var(--surface-1);stroke-width:.6}
.dot:hover{fill-opacity:.95;stroke-width:1.2}
.refln{stroke:var(--baseline);stroke-width:1.5;stroke-dasharray:5 4}
.reftx{fill:var(--muted);font-size:11px}
.bar{fill:var(--seq-500)} .bar:hover{fill:var(--series-1)}
.gridln{stroke:var(--grid);stroke-width:1}
#tip{position:fixed;pointer-events:none;opacity:0;transition:opacity .08s;background:var(--surface-1);
  border:1px solid var(--ring);border-radius:8px;padding:8px 10px;font-size:12px;
  box-shadow:0 4px 16px rgba(0,0,0,.18);max-width:320px;z-index:9}
#tip b{font-weight:650} #tip .m{color:var(--text-secondary)}
.note{font-size:12.5px;color:var(--text-secondary);max-width:78ch}
.note code{background:var(--grid);padding:1px 5px;border-radius:4px}
</style></head><body><div class="wrap">
<div class="bar-top">
  <span class="badge">__BADGE__</span>
  <a class="xlink" href="__OTHER_HREF__">↔ __OTHER_LABEL__</a>
  <a class="xlink" href="misplacement.html">↔ misplacement finder</a>
  <a class="xlink" href="homm2.html">↔ HoMM2 baseline</a>
  <button class="toggle" onclick="tog()">◐ theme</button>
</div>
<h1>GRUNTZ.EXE &mdash; how scattered is each source file across <code style="font-size:.9em">.text</code>?</h1>
<p class="sub">A source file breaks into contiguous <b>fragments</b>; <b>1 fragment</b> = one solid block,
<b>fragments = function count</b> = every function isolated. __VARIANT_SUB__ Based on __FILES__ files
(the <code style="font-size:.9em">src/Stub/</code> backlog is excluded).</p>

<div class="tiles">
  <div class="tile"><div class="v">__MEDRATIO__</div><div class="k">median fragmentation ratio<br>(files &ge;5 fns; 1.0 = fully scattered)</div></div>
  <div class="tile"><div class="v">__MEDCLUSTER__</div><div class="k">median contiguous run<br>(functions per fragment, &ge;5-fn files)</div></div>
  <div class="tile"><div class="v">__MEDSPREAD__&times;</div><div class="k">median spread<br>(RVA span &divide; own bytes)</div></div>
  <div class="tile"><div class="v">__CONTIG__%</div><div class="k">of &ge;5-fn files are<br>perfectly contiguous</div></div>
</div>

<div class="card">
  <h2>Fragments vs function count &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(log&ndash;log, one dot per file)</span></h2>
  <p class="cap">A file can't have more fragments than functions, so every dot is on or below the diagonal.
  <b>On the dashed line</b> = every function isolated (max scatter). <b>Near the bottom</b> = contiguous. Hover a dot.</p>
  <svg id="scat" viewBox="0 0 1000 560"></svg>
</div>

<div class="card">
  <h2>Distribution of fragmentation ratio &nbsp;<span style="font-weight:400;color:var(--text-secondary)">(files &ge;5 fns)</span></h2>
  <p class="cap">How many files fall in each scatter band. Left-heavy = mostly contiguous; right-heavy = mostly fragmented.</p>
  <svg id="hist" viewBox="0 0 1000 320"></svg>
</div>

<p class="note">__NOTE__</p>
</div>

<div id="tip"></div>
<script>
const DATA=__DATA__;
const tip=document.getElementById('tip');
function showTip(e,html){tip.innerHTML=html;tip.style.opacity=1;
  let x=e.clientX+14,y=e.clientY+14;
  if(x+320>innerWidth)x=e.clientX-14-Math.min(320,tip.offsetWidth);
  tip.style.left=x+'px';tip.style.top=y+'px';}
function hideTip(){tip.style.opacity=0;}
function svg(t,a){const e=document.createElementNS('http://www.w3.org/2000/svg',t);
  for(const k in a)e.setAttribute(k,a[k]);return e;}
const fmtB=b=>b<1024?b+' B':(b/1024).toFixed(1)+' KB';

(function(){
  const s=document.getElementById('scat'),W=1000,H=560,L=54,R=18,T=14,B=44;
  const pw=W-L-R,ph=H-T-B;
  const pts=DATA.filter(d=>d.n>=2);
  const max=Math.max(...DATA.map(d=>d.n),2);
  const lg=v=>Math.log10(v), lmax=lg(max);
  const X=v=>L+lg(v)/lmax*pw, Y=v=>T+ph-lg(v)/lmax*ph;
  const ticks=[1,2,5,10,20,50,100,200,500].filter(t=>t<=max);
  for(const t of ticks){
    s.appendChild(svg('line',{x1:X(t),y1:T,x2:X(t),y2:T+ph,class:'gridln'}));
    s.appendChild(svg('line',{x1:L,y1:Y(t),x2:L+pw,y2:Y(t),class:'gridln'}));
    let tx=svg('text',{x:X(t),y:T+ph+16,class:'tick x'});tx.textContent=t;s.appendChild(tx);
    let ty=svg('text',{x:L-8,y:Y(t)+4,class:'tick y'});ty.textContent=t;s.appendChild(ty);
  }
  s.appendChild(svg('line',{x1:X(1),y1:Y(1),x2:X(max),y2:Y(max),class:'refln'}));
  let rl=svg('text',{x:X(max)-4,y:Y(max)+16,class:'reftx','text-anchor':'end'});
  rl.textContent='maximally scattered  (1 function / fragment)';s.appendChild(rl);
  let axx=svg('text',{x:L+pw/2,y:H-4,class:'axttl','text-anchor':'middle'});
  axx.textContent='functions in file  (log)';s.appendChild(axx);
  let axy=svg('text',{x:-(T+ph/2),y:15,class:'axttl','text-anchor':'middle',transform:'rotate(-90)'});
  axy.textContent='contiguous fragments  (log)';s.appendChild(axy);
  for(const d of pts){
    const rad=Math.max(3,Math.min(9,Math.sqrt(d.b)/22));
    const c=svg('circle',{cx:X(d.n),cy:Y(d.g),r:rad,class:'dot'});
    c.addEventListener('mousemove',e=>showTip(e,
      `<b>${d.f}</b><br><span class="m">${d.n} fns &rarr; ${d.g} fragments</span><br>`+
      `avg run <b>${d.c}</b> fns &middot; spread <b>${d.s}&times;</b> &middot; ${fmtB(d.b)}`));
    c.addEventListener('mouseleave',hideTip);
    s.appendChild(c);
  }
})();

(function(){
  const s=document.getElementById('hist'),W=1000,H=320,L=54,R=18,T=10,B=42;
  const pw=W-L-R,ph=H-T-B;
  const vals=DATA.filter(d=>d.n>=5).map(d=>d.r);
  const NB=10,bins=Array(NB).fill(0);
  for(const v of vals){let i=Math.min(NB-1,Math.floor(v*NB));bins[i]++;}
  const ymax=Math.max(...bins,1);
  for(let g=0;g<=4;g++){const yv=Math.round(ymax*g/4),y=T+ph-yv/ymax*ph;
    s.appendChild(svg('line',{x1:L,y1:y,x2:L+pw,y2:y,class:'gridln'}));
    let ty=svg('text',{x:L-8,y:y+4,class:'tick y'});ty.textContent=yv;s.appendChild(ty);}
  const bw=pw/NB;
  for(let i=0;i<NB;i++){
    const h=bins[i]/ymax*ph,x=L+i*bw+3,y=T+ph-h;
    const r=svg('rect',{x:x,y:y,width:bw-6,height:Math.max(0,h),rx:3,class:'bar'});
    const lo=(i/NB).toFixed(1),hi=((i+1)/NB).toFixed(1);
    r.addEventListener('mousemove',e=>showTip(e,`ratio <b>${lo}&ndash;${hi}</b><br><span class="m">${bins[i]} files</span>`));
    r.addEventListener('mouseleave',hideTip);
    s.appendChild(r);
    if(bins[i]){let t=svg('text',{x:x+(bw-6)/2,y:y-5,class:'tick x'});t.textContent=bins[i];s.appendChild(t);}
    let xt=svg('text',{x:L+i*bw+bw/2,y:T+ph+16,class:'tick x'});xt.textContent=((i+0.5)/NB).toFixed(2);s.appendChild(xt);
  }
  let axx=svg('text',{x:L+pw/2,y:H-4,class:'axttl','text-anchor':'middle'});
  axx.textContent='fragmentation ratio  (fragments ÷ functions;  1.0 = fully scattered)';s.appendChild(axx);
  let axy=svg('text',{x:-(T+ph/2),y:15,class:'axttl','text-anchor':'middle',transform:'rotate(-90)'});
  axy.textContent='files';s.appendChild(axy);
})();

function tog(){const r=document.documentElement;
  const dark=matchMedia('(prefers-color-scheme:dark)').matches;
  const cur=r.getAttribute('data-theme')|| (dark?'dark':'light');
  r.setAttribute('data-theme',cur==='dark'?'light':'dark');}
</script></body></html>"""


def build(src_json, out_html, badge, titletag, other_href, other_label, variant_sub, note):
    rows = json.load(open(DIR + "/" + src_json))
    pts = [{"f": r["file"], "n": r["n"], "g": r["frags"], "r": round(r["frag_ratio"], 3),
            "c": round(r["avg_cluster"], 2), "s": round(r["spread"], 1), "b": r["bytes"]}
           for r in rows]
    big = [r for r in rows if r["n"] >= 5]
    fr = [r["frag_ratio"] for r in big]
    repl = {
        "__DATA__": json.dumps(pts, separators=(",", ":")),
        "__FILES__": str(len(rows)),
        "__MEDRATIO__": str(round(st.median(fr), 2)),
        "__MEDCLUSTER__": str(round(st.median([r["avg_cluster"] for r in big]), 1)),
        "__MEDSPREAD__": str(round(st.median([r["spread"] for r in big]))),
        "__CONTIG__": str(round(100 * sum(1 for r in big if r["frags"] == 1) / len(big))),
        "__BADGE__": badge, "__TITLETAG__": titletag,
        "__OTHER_HREF__": other_href, "__OTHER_LABEL__": other_label,
        "__VARIANT_SUB__": variant_sub, "__NOTE__": note,
    }
    html = TEMPLATE
    for k, v in repl.items():
        html = html.replace(k, v)
    open(DIR + "/" + out_html, "w").write(html)
    print(f"wrote {out_html}  ({len(rows)} files, median frag-ratio(>=5) "
          f"{repl['__MEDRATIO__']}, contiguous {repl['__CONTIG__']}%)")


NOTE_COMMON = ('Fragmentation is a property of the <b>retail link layout</b>, not the reconstruction: '
               'MSVC/link concatenates each TU\'s COMDATs in source order, so a TU is <i>mostly</i> one '
               'contiguous block. Data grouping is our <code>src/</code> files (kept close to the original '
               'TUs). Source: <code>scatter*.json</code>.')

build("scatter.json", "scatter.html",
      "all functions", "(all functions)",
      "scatter_core.html", "hide irreducible /Gy COMDAT scatter",
      "Here <b>all</b> functions are counted &mdash; including the compiler COMDATs the retail /Gy link "
      "pools away from the TU block (ctors/dtors/operators/init-fragments). Toggle to the core view to "
      "hide them.",
      "Destructors + ctors + operators + init fragments here still count. Under <code>/Gy</code> MSVC emits "
      "every function as a COMDAT and the linker pools the special-members (deleting <code>??_G</code>/<code>??_E</code> "
      "dtors, <code>??0</code> ctors, <code>??4</code>&hellip; operators) into shared low-address runs away from "
      "the TU block &mdash; so they inflate the scatter. The <b>core</b> view removes exactly that set. " + NOTE_COMMON)

build("scatter_core.json", "scatter_core.html",
      "core body (irreducible scatter hidden)", "(core body)",
      "scatter.html", "show all functions (affirm what's hidden)",
      "This view hides <b>all irreducible /Gy scatter</b> &mdash; every MSVC special-name COMDAT "
      "(<code>??0</code> ctors, <code>??1</code>/<code>??_G</code>/<code>??_E</code> dtors, <code>??4</code>&hellip; "
      "operators), CRT init-fragments (<code>_$E</code>/<code>InitStr</code>) and pooled vtable-slot virtuals. "
      "What remains is the genuine per-TU body.",
      "The hidden set is exactly the functions our build CANNOT relocate: the retail <code>/Gy</code> linker places "
      "each COMDAT by first-use / init order, and re-emitting one in its TU would trip the duplicate-RVA guard "
      "(see <code>docs/exe-map/interleaved-comdats.md</code>). Flip to <b>all functions</b> to affirm every hidden dot "
      "is one of those. Once the contiguity drain evicts every foreign mislabel + splits every over-merged TU, this "
      "line <b>flatlines at 1 fragment</b> &mdash; each TU's body is one contiguous block (as in non-<code>/Gy</code> HoMM2). " + NOTE_COMMON)

build("scatter_methods.json", "scatter_methods.html",
      "destructors removed", "(no destructors)",
      "scatter.html", "view with all functions",
      "<b>Destructors are removed</b> here (base <code>??1</code> + deleting <code>??_G</code>/<code>??_E</code>); "
      "constructors are kept, since they cluster with the methods.",
      "This view drops the destructors &mdash; the COMDAT-pooled members that MSVC exiles from the TU block "
      "(measured dtor median distance-to-sibling 0x7b0, p90 ~645&nbsp;KB, vs 0xa0 / 93%-within-8KB for methods). "
      "For the full irreducible-scatter view see <b>core body</b>. " + NOTE_COMMON)

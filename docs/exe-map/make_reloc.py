#!/usr/bin/env python3
"""Build reloc.html - the reloc-fidelity dashboard (the metric objdiff can't see).
Regenerates reloc_fidelity.json live (via `reloc_fidelity --json`) so the page can
never go stale, then renders. Self-contained vanilla JS, same tokens as the other
exe-map pages. If the live regen fails (no delink data), falls back to the committed
JSON so build_site.py still completes."""
import json
import os as _os
import subprocess as _sp
import sys as _sys

DIR = _os.path.dirname(_os.path.abspath(__file__))
JSON = DIR + "/reloc_fidelity.json"

# Refresh the JSON from the live analysis so reloc.html always reflects current src/
# (make_reloc used to read a committed snapshot -> the dashboard silently went stale).
try:
    _out = _sp.run([_sys.executable, "-m", "gruntz.analysis.reloc_fidelity", "--json"],
                   capture_output=True, text=True, check=True).stdout
    json.loads(_out)  # validate before overwriting
    open(JSON, "w").write(_out)
except Exception as _e:  # noqa: BLE001 - degrade to the committed snapshot
    print(f"  (reloc live-regen failed: {_e}; using committed reloc_fidelity.json)")

D = json.load(open(JSON))
UNITS = [u for u in D["units"] if u["misbound"] + u["unbound"] > 0][:40]
WORST = D["worst"]
pct = 100 * D["faithful"] // max(D["n"], 1)

tiles = {
    "__PCT__": str(pct), "__FAITHFUL__": f'{D["faithful"]:,}', "__N__": f'{D["n"]:,}',
    "__CALL__": f'{D["call_bad"]:,}', "__DATA__": f'{D["data_bad"]:,}',
    "__MIS__": f'{D["misbound"]:,}', "__UNB__": f'{D["unbound"]:,}',
    "__SITES__": f'{D["sites"]:,}',
}

HTML = r"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GRUNTZ.EXE reloc fidelity</title>
<style>
:root{--surface-1:#fcfcfb;--plane:#f9f9f7;--text-primary:#0b0b0b;--text-secondary:#52514e;
  --muted:#898781;--grid:#e1e0d9;--baseline:#c3c2b7;--blue:#2a78d6;--green:#1baf7a;
  --orange:#eb6834;--red:#e34948;--yellow:#eda100;--ring:rgba(11,11,11,0.10);}
:root[data-theme=dark]{--surface-1:#1a1a19;--plane:#0d0d0d;--text-primary:#fff;
  --text-secondary:#c3c2b7;--muted:#898781;--grid:#2c2c2a;--baseline:#383835;
  --blue:#3987e5;--green:#199e70;--orange:#d95926;--red:#e66767;--yellow:#c98500;
  --ring:rgba(255,255,255,0.10);}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){--surface-1:#1a1a19;
  --plane:#0d0d0d;--text-primary:#fff;--text-secondary:#c3c2b7;--muted:#898781;
  --grid:#2c2c2a;--baseline:#383835;--blue:#3987e5;--green:#199e70;--orange:#d95926;
  --red:#e66767;--yellow:#c98500;--ring:rgba(255,255,255,0.10);}}
*{box-sizing:border-box}
body{margin:0;background:var(--plane);color:var(--text-primary);
  font-family:system-ui,-apple-system,"Segoe UI",sans-serif;line-height:1.45;padding:28px 20px 60px}
.wrap{max-width:1040px;margin:0 auto}
h1{font-size:22px;margin:0 0 4px}.sub{color:var(--text-secondary);font-size:14px;margin:0 0 14px;max-width:90ch}
.bar-top{display:flex;gap:10px;align-items:center;margin:0 0 18px;flex-wrap:wrap}
.badge{font-size:12px;font-weight:600;border-radius:999px;padding:4px 12px;background:var(--red);color:#fff}
.xlink{font-size:12.5px;color:var(--blue);text-decoration:none;border:1px solid var(--ring);border-radius:6px;padding:4px 10px}
.xlink:hover{background:var(--surface-1)}
.toggle{margin-left:auto;font-size:12px;border:1px solid var(--ring);background:var(--surface-1);color:var(--text-secondary);border-radius:6px;padding:5px 10px;cursor:pointer}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin:0 0 24px}
.tile{background:var(--surface-1);border:1px solid var(--ring);border-radius:10px;padding:14px 16px}
.tile .v{font-size:28px;font-weight:650;letter-spacing:-0.02em}.tile .k{font-size:12px;color:var(--text-secondary);margin-top:2px}
.card{background:var(--surface-1);border:1px solid var(--ring);border-radius:12px;padding:18px 18px 12px;margin-bottom:22px}
.card h2{font-size:15px;margin:0 0 2px}.card .cap{font-size:12.5px;color:var(--text-secondary);margin:0 0 12px;max-width:90ch}
.legend{display:flex;gap:16px;flex-wrap:wrap;font-size:12px;color:var(--text-secondary);margin:2px 0 10px}
.sw{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px;vertical-align:middle}
.row{display:grid;grid-template-columns:150px 1fr 90px;gap:10px;align-items:center;margin:3px 0;font-size:12.5px}
.rlabel{text-align:right;color:var(--text-secondary);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.rbar{height:15px;background:var(--grid);border-radius:3px;overflow:hidden;display:flex}
.rbar .c{background:var(--orange);height:100%}.rbar .d{background:var(--blue);height:100%}
.rnum{font-variant-numeric:tabular-nums;color:var(--text-secondary)}
table{border-collapse:collapse;font-size:12.5px;width:100%}
th{text-align:left;color:var(--text-secondary);font-weight:600;padding:4px 10px 4px 0;border-bottom:1px solid var(--grid)}
td{padding:4px 10px 4px 0;border-bottom:1px solid var(--grid)}
td code{background:var(--grid);padding:0 4px;border-radius:3px;font-size:11.5px}
.note{font-size:12.5px;color:var(--text-secondary);max-width:90ch}.note code,.cap code{background:var(--grid);padding:1px 5px;border-radius:4px}
</style></head><body><div class="wrap">
<div class="bar-top">
  <span class="badge">reloc fidelity</span>
  <a class="xlink" href="deep.html">&harr; deep map</a>
  <a class="xlink" href="misplacement.html">&harr; misplacement</a>
  <a class="xlink" href="scatter_methods.html">&harr; scatter</a>
  <button class="toggle" onclick="tog()">&#9681; theme</button>
</div>
<h1>GRUNTZ.EXE &mdash; reloc fidelity <span style="font-weight:400;color:var(--text-secondary);font-size:15px">(the metric objdiff can't see)</span></h1>
<p class="sub">A relocation is byte-masked in match scoring: at a fixup site objdiff compares nothing, so a reference
bound to <b>NO</b> retail address (a declared-only fake, usually in a fake view) or bound to the <b>WRONG</b>
address still scores <b>100%</b>. This tool aligns each byte-exact function's relocations against retail's
authoritative targets. Faithful = zero misbound, zero unbound. Live campaign metric &mdash; drive to zero.</p>

<div class="tiles">
  <div class="tile"><div class="v">__PCT__%</div><div class="k">faithful functions<br>(__FAITHFUL__ of __N__ byte-exact)</div></div>
  <div class="tile"><div class="v" style="color:var(--orange)">__CALL__</div><div class="k">CALL-target defects<br>(fake/misbound functions, rel32)</div></div>
  <div class="tile"><div class="v" style="color:var(--blue)">__DATA__</div><div class="k">DATA defects<br>(globals/vtables, DIR32)</div></div>
  <div class="tile"><div class="v" style="color:var(--red)">__MIS__</div><div class="k">MISBOUND sites<br>(wrong rva &mdash; toxic)</div></div>
  <div class="tile"><div class="v">__UNB__</div><div class="k">UNBOUND sites<br>(fake decl; retail names the target)</div></div>
</div>

<div class="card">
  <h2>Worst units</h2>
  <p class="cap">Defective reloc sites per unit, split <b style="color:var(--orange)">CALL</b> (fake/misbound
  functions) vs <b style="color:var(--blue)">DATA</b> (globals, vtables). CALL defects are the ones tied
  tightest to fake views &mdash; a view exists to declare a phantom method. Bars share one scale.</p>
  <div class="legend">
    <span><span class="sw" style="background:var(--orange)"></span>CALL (function) defects</span>
    <span><span class="sw" style="background:var(--blue)"></span>DATA (global/vtable) defects</span>
  </div>
  <div id="units"></div>
</div>

<div class="card">
  <h2>Worst functions</h2>
  <p class="cap">Individual byte-exact functions hiding the most reloc defects &mdash; each scores a perfect
  100% in objdiff.</p>
  <div id="worst"></div>
</div>

<p class="note">Source: <code>python -m gruntz.analysis.reloc_fidelity</code> (<code>--worklist</code> for the
per-site fix CSV: each defect's <code>retail_rva</code> names the real target). DIR32 data + rel32 call
targets; scoped to the byte-exact set where offsets provably align. Regenerate with
<code>reloc_fidelity --json &gt; reloc_fidelity.json</code> then <code>make_reloc.py</code>.</p>

<script>
const UNITS=__UNITS__, WORST=__WORST__;
const mx=Math.max(1,...UNITS.map(u=>u.call_bad+u.data_bad));
document.getElementById('units').innerHTML=UNITS.map(u=>{
  const cw=100*u.call_bad/mx, dw=100*u.data_bad/mx;
  return `<div class="row"><div class="rlabel">${u.unit}</div>
    <div class="rbar"><div class="c" style="width:${cw}%"></div><div class="d" style="width:${dw}%"></div></div>
    <div class="rnum">${u.call_bad}C / ${u.data_bad}D</div></div>`;
}).join('');
document.getElementById('worst').innerHTML=
  `<table><tr><th>unit</th><th>function</th><th>CALL</th><th>DATA</th><th>misbound</th><th>unbound</th></tr>`+
  WORST.map(w=>`<tr><td>${w.unit}</td><td><code>${w.name}</code></td><td>${w.call_bad}</td>
    <td>${w.data_bad}</td><td>${w.misbound}</td><td>${w.unbound}</td></tr>`).join('')+`</table>`;
function tog(){const r=document.documentElement,dark=matchMedia('(prefers-color-scheme:dark)').matches;
  const cur=r.getAttribute('data-theme')||(dark?'dark':'light');r.setAttribute('data-theme',cur==='dark'?'light':'dark');}
</script></body></html>"""

for k, v in {"__UNITS__": json.dumps(UNITS, separators=(",", ":")),
             "__WORST__": json.dumps(WORST, separators=(",", ":")), **tiles}.items():
    HTML = HTML.replace(k, v)
open(DIR + "/reloc.html", "w").write(HTML)
print("wrote reloc.html  (%d%% faithful, %d call / %d data defects, %d units)"
      % (pct, D["call_bad"], D["data_bad"], len(UNITS)))

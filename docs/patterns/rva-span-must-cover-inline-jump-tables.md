# A hard 0% on a jump-table function is a SPAN bug, not a codegen wall

**Tags:** `cpp:switch` | `asm:jmp` | `topic:scoring-artifact` `topic:tooling`
**Confidence:** 9/10

## Symptom

A function with a dense `switch` scores a flat **0%** in `report.json` no matter how
correct the body is, while its neighbours in the same unit score normally. The
`@early-stop` note on it usually blames "the jump-table data region can't pair" or
"the jumptable-data-overlap scoring artifact".

**The trap that hides it:** objdiff omits `fuzzy_match_percent` entirely when the value
is `0.0` (serde default-skip). So in `report.json` a genuine 0 and a *never scored*
function are indistinguishable, and both render as `0.00` in `gruntz status`. Listing
the functions with **no** `fuzzy_match_percent` key is the way to find the family:

```python
[ (f['size'], u['name'], f['name'])
  for u in json.load(open('build/objdiff/report.json'))['units']
  for f in u.get('functions', []) if 'fuzzy_match_percent' not in f ]
```

## Cause

objdiff sizes a symbol by **next-symbol-start**. Our side is a `/Gy` COMDAT whose
section covers the code **plus** the inline switch index LUT and jump table. The
delinked target side is carved to whatever the `RVA()` annotation says — and the
annotation is normally the *code* length (what Ghidra reports, i.e. up to the `ret`).
The two sides then disagree on the symbol length and the comparison collapses to 0.

## Fix

Grow the `RVA()` span to the **true retail extent**: from the function start to the
start of the next real function, so the carve covers the tables the code jumps into.

```cpp
// before - 0x95 is the code length, function scores a flat 0
RVA(0x000bf7c0, 0x95)
i32 CNetSession::DispatchMsg(CNetCtrlMsg* m, i32 arg2) { ... }

// after - 0x1b0 also covers the 258-byte index LUT + the 5-entry jump table
RVA(0x000bf7c0, 0x1b0)
```

Verify two things before you commit it:

1. **It must not overlap the next annotated function.** `next_start - rva` is the
   maximum legal span; use exactly that when the gap is all table.
2. **Measure, do not assume which value.** On `CMapMgr::ComputeCellFlags` (0x77790)
   all three plausible values behaved differently: `0x37d` (code) = unscored,
   `0x450` (our base COMDAT) = 24.5%, `0x630` (true retail extent, ending exactly
   where `SetCell` starts) = **43.0%**. The true extent was both the honest annotation
   and the best-scoring one, but the base-COMDAT value is a real alternative when our
   tables and retail's differ in size.

## The span is MEASURED, not assumed — growing it can make things WORSE

"Extend to the next function's start" is not the rule; it is one of two candidates.
It is right only when the gap between the code and the next function holds **nothing but
this function's own tables** (`DispatchMsg`, `ComputeCellFlags`). When the gap holds more,
carrying it in makes the two sides carve *different* content and the score collapses.

Measured 2026-07-29, all three in `tileswitchlogic`, all three reverted:

| function | base COMDAT | extent-to-next | verdict |
|---|---|---|---|
| `CTileActionEvent::SetActionCode` | 0xf0 → **66.6** | 0x140 → 47.2 | COMDAT wins |
| `CTileActionEvent::Process` | 0x35e → **58.9** | 0x540 → 47.4 | COMDAT wins |
| `CTileActionEvent::MorphByTool` | 0x350 → **92.1** | 0x440 → 53.9 | COMDAT wins |

So: compile both candidates and take the number. The defect these three were found by —
an annotated span whose last bytes are not a `ret`/`jmp` terminator — is also RARE (21 of
3700 swept, almost all benign trailing padding), so a non-terminator tail is a hint to
measure, not evidence of a bug.

## The cheap pre-check: does OUR table start at retail's table offset?

The two candidates above are worth compiling only when the *code* half is already the
right length, and that is decidable from the objects in one look — no build needed:

* retail's table start = the `switchdataD_...` **relocation target** in
  `gruntz sema disasm <rva>` (it is listed in the relocation header);
* our table start = the offset of the first `$Lnnnnn` label in the function's COMDAT,
  `llvm-objdump -d build/objdiff/base/<unit>.obj`.

If those two offsets are EQUAL, the code is byte-length-correct and growing the span
lands the tables on top of each other — expect a jump straight to (or near) 100. If they
differ, fix the body first; the span change alone will not converge. Measured
2026-07-29 in one unit: `CDDSurface::Refresh` and `CDDSurface::BlitIntoDesc` both had
matching offsets (0x1bc, 0x188) and both went to **100.00 EXACT**;
`CGruntzMgr::BuildMoviePath`'s base table sat at 0x1c8 against retail's 0x1cc, which was
a real body defect (four missing jump-table entries — its `switch` case values were
wrong), and only after fixing that did the wider span pay.

Corollary: a function with `switchdata` relocs is worth auditing even at a HIGH score.
`CGruntzMgr::ReportWorldStatus` sat at 96.81 with a code-only span and went to 99.98 on
the span alone. Sweep a unit for the family with:

```sh
for r in $(awk -F, '$3=="<unit>" && $5=="func" {print $1}' build/gen/symbol_names.csv); do
  gruntz sema disasm $r 2>/dev/null | grep -q switchdata && echo $r
done
```

## What this does NOT fix (measured, so you can stop early)

A body that is genuinely **shorter than retail** scores 0 for a different reason, and
no span helps. Check with `python -m gruntz.audit.base_size --all` first: if the
compiled length is far below the annotated one, the body is under-reconstructed.
Reverted after testing, all still 0 at every span: `CPlay::Render` (-276 bytes),
`CGrunt::StepArrivalDrop` (-253), `CFaderMgr::Add` (-546), and `CGameObject::Play`
(-15, tried at both its exact code+table extent 0x190 and next-function extent 0x1d0).

## Evidence

2026-07-29, the whole family swept in one pass (every 0-scoring function in the tree
cross-checked against base COMDAT size vs annotated size):

| function | rva | span | before | after |
|---|---|---|---|---|
| `CNetSession::DispatchMsg` | 0xbf7c0 | 0x95 → 0x1b0 | 0 | **100.00 EXACT** |
| `CMulti::HandleControlMsg` | 0xba1a0 | 0x83 → 0x1a0 | 0 | 96.66 |
| `CMapMgr::ComputeCellFlags` | 0x77790 | 0x37d → 0x630 | 0 | 42.95 |
| `CActionOptionsMenuBar::Refresh` | 0x9330 | 0x136 → 0x190 | 0 | 32.94 |

The first two had `@early-stop` notes asserting the code bytes were already
byte-identical (proven with `llvm-objdump -dr`). They were right — the bytes were never
the problem, the span was. Precedent: `CTileActionEvent::MorphByTool` @0x113420 already
carried `RVA(..., 0x350)` for the same reason.

2026-07-29, second sweep (`ddsurface` / `gruntzmgr`) — note these were NOT 0-scorers, so
the "list the functions with no `fuzzy_match_percent` key" recipe above misses them
entirely; the `switchdata`-reloc sweep is what finds them:

| function | rva | span | before | after |
|---|---|---|---|---|
| `CDDSurface::Refresh` | 0x13e140 | 0x133 → 0x1a0 | 5.40 | **100.00 EXACT** |
| `CDDSurface::BlitIntoDesc` | 0x13e2e0 | 0x188 → 0x1f0 | 50.00 | **100.00 EXACT** |
| `CGruntzMgr::ReportWorldStatus` | 0x90ac0 | 0x1bb → 0x1cc | 96.81 | 99.98 |
| `CGruntzMgr::BuildMoviePath` | 0x8ff30 | 0x1ca → 0x20c | 88.06 | 96.74 |

`Refresh` is the sharpest case: its `@early-stop` note said writing the switch "with a
`case 8` makes MSVC emit the same jump-table structure, but ... objdiff scores that 0%",
and the body had been rewritten as an if/else chain SPECIFICALLY to score. It measured
5.40 with the jump table and a code-only span, and 100.00 EXACT with the jump table and
the true extent. Writing the wrong shape to protect a number cost 34 points.

Related: [[jumptable-data-overlap]], [[switch-jumptable-separate-comdat]].

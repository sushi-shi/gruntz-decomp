# A retail `call ??0CRect@@QAE@HHHH@Z` proves the TU compiled with MFC inlines OFF

tags: cpp:header cpp:local | asm:call asm:lea | topic:codegen-idiom topic:tooling

symptoms: a function that builds a `CRect(0, 0, w, h)` plateaus 15-20 points below
  its neighbours; the retail disasm has `push h / push w / push 0 / push 0 /
  lea ecx,[esp+N] / call <thunk>` where our recompile emits four inline `mov`s

confidence: 9/10

## The test — one grep, no guessing

`??0CRect@@QAE@HHHH@Z` lives at **0x00029ac0** (32 bytes, `ret 0x10`), reached through
the ILT thunk **0x000034a4**. MFC defines that constructor in `afxwin1.inl` behind
`_AFX_ENABLE_INLINES`, so a call to the LIB copy can only happen in a TU that was
compiled with the inlines suppressed. Therefore:

```
gruntz sema xref 0x00029ac0 --raw     # every unit that compiled without MFC inlines
gruntz sema disasm <rva> --lite | grep -c '0x34a4\|0x29ac0'
```

Any unit in that caller list whose source includes `<Mfc.h>`/`<MfcWin.h>` but **not**
`<MfcNoInline.h>` is mis-configured. The fix is one include:

```cpp
#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>
```

(`<Mfc.h>` must precede it — `_AFX_ENABLE_INLINES` is *defined* by `<afx.h>`, so
undefining it earlier is a silent no-op.)

## Evidence

Measured 2026-08-07 on the CGrunt/Battlez family. Adding the include alone:

| unit | function | before | after |
|---|---|---|---|
| gruntcombat | `CGrunt::PathScan` @0x57db0 | 69.71 | **85.16** |
| grunt | `CGrunt::XferName` @0x5d210 | 78.68 | 80.00 |
| grunt | `CGrunt::StepGruntMovement` @0x4c170 | 64.91 | 65.74 |
| grunttilescan | `CBattlezMapConfig::ScanRegion` @0x32ce0 | 67.47 | 69.56 |

`CGrunt::LoadAnimNameTable` (100% EXACT, same TU as XferName) stayed EXACT — the
switch only changes TUs that actually instantiate an MFC inline body.

Overall project fuzzy rose 86.28 -> 86.40 across nine units (`grunt`, `gruntcombat`,
`grunttilescan`, `battlezmapconfig`, `battlezunitstep`, `gruntstatestep`,
`gruntbricklayerstep`, `gruntgoosuckerstep`, `gruntdiggerstep`, `gruntphasestep`).

## Why it is worth more than the ctor itself

`_AFX_ENABLE_INLINES` is a whole-TU switch: it also un-inlines `CString`, `CPoint`,
`CSize` and the `CObject`/`CObList` accessors. So one wrongly-inlined TU misprices
EVERY MFC expression in it, and the residue reads as diffuse regalloc noise rather
than a header bug. The `CRect(int,int,int,int)` call is just the cheapest detector
because it is the one MFC inline that is big enough to be obvious in a diff.

## Corollary for reading the disasm

Inside such a TU, an INLINE four-store rect construction is therefore **not** a
`CRect` — it is a plain `RECT` (or a `RECT` built by a project-local inline). And a
`CRect ra(...)` whose value is immediately overwritten is a source bug: retail's
`SCAN_RECT_BOUNDS`-style clip block constructs TWO live `CRect`s and copies one into a
third plain `RECT` (`RECT full = CRect(0,0,w,h);` — temporary + copy-init), which is
what produces two ctor calls plus a field-by-field copy.

## The corollary, measured (2026-08-07)

`CBattlezMapConfig::StepRowUnits` @0x267c0 confirmed it end-to-end. `insn_seq --multiset`
read `??0CRect@@QAE@HHHH@Z base=9 tgt=15` - retail builds SIX more CRects than we did. Every
clip block spells the second rect as a **temporary + copy-init**, not a struct assignment:

```cpp
static_cast<RECT*>(new (&h1) CRect(0, 0, hb->m_width, hb->m_height));
RECT hc = CRect(0, 0, hb->m_width, hb->m_height);   // NOT `RECT hc; hc = h1;`
if (!IntersectRect(&hb->m_bounds, &hc, &h1)) { hb->m_bounds = hc; }
```

The tell in the disasm is that the copy reads through the ctor's RETURN value
(`call <CRect>; mov ecx,[eax]; mov [hc],ecx; mov edx,[eax+4]; ...`) - a struct assignment
would read the named source slot instead. One block additionally builds its first rect with
four inline stores (a plain `RECT`, four `mov`s, no call), which is what makes the retail
frame `sub esp,0x12c` against our `0x11c`: **the frame-size delta counts the rects you are
missing**, 0x10 per RECT. Fixing both took StepRowUnits 84.00 -> 85.40 with `sub esp` and the
whole 270-entry reloc sequence exactly matching.

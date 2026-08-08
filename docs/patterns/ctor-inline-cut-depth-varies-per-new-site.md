# cl5 cuts a ctor chain OUT-OF-LINE at a depth that varies PER `new`-SITE
tags: cpp:ctor cpp:new cpp:inline | asm:call asm:eh topic:wall
symptoms: base has NO `/GX` EH frame where retail does (`push -1 / push <handler> / mov fs:0,esp`), a smaller `sub esp,N`, and retail has a `call <ILT thunk>` at a `new T` site where the base stamps the base-class fields inline; retail also sets an EH state (`mov [esp+N],1/2/3`) around each such `new`
confidence: 9/10 (three distinct cut depths measured in two functions; every OOL body identified by address)
variants: ool-ctor-device (docs/patterns/ INDEX), phantom-method-reconstruction.md

`new CSBI_MenuItem` runs a four-deep ctor chain
`CSBI_MenuItem -> CSBI_Image -> CSBI_RectOnly -> CStatusBarItem`, all four defined in-class
in headers. cl5 inlines a PREFIX of that chain and emits a `call` for the rest, and **the
cut depth is not a property of the declaration** - the same ctor is called at one site and
inlined at another **inside a single function**:

| site | retail | cut after |
|---|---|---|
| `BuildStatusBarTabs` @0xffde0, the three `new CSBI_RectOnly` | 5 inline stores, no call | nothing (whole chain inlined) |
| `BuildStatusBarTabs`, all five `new CSBI_MenuItem` | `call 0x22c0` -> `??0CStatusBarItem@@QAE@XZ` @0x1005d0 | depth 3 |
| `BuildGameMenu` @0x101580, sites 1-3 | `call 0x1e88` -> **`??0CSBI_RectOnly@@QAE@XZ` @0x101fa0** (27 B, unnamed in our tree) | depth 2 |
| `BuildGameMenu`, site 4 | `call 0x22c0` | depth 3 |
| `BuildTabzDialog` @0x10a340, `new CSBI_Image` | `call 0x22c0` | depth 2 |

The consequence is structural, not cosmetic: an out-of-line ctor **can throw**, so cl must
be able to `operator delete` the half-constructed object. That forces the whole function
into a `/GX` EH frame with a state variable (`mov [esp+N],1`, `2`, `3` at successive
`new`s, reset to `-1` once each ctor returns), and grows the frame. A fully-inlined chain
contains no call, cannot throw, and gets no frame at all. So one inliner decision moves the
prologue, the epilogue, the frame size and the register allocation of the entire function.

## What does NOT control it

- **Declaration form.** All four ctors are in-class inline on both sides; `??0CSBI_RectOnly`
  is inlined at some sites and called at others in the same object file.
- **`#pragma inline_depth(3)`** reproduces `BuildStatusBarTabs` (71.58 -> 78.21, EH frame and
  all five calls appear) but is a global setting and therefore cannot express a per-site cut;
  it drives `BuildGameMenu` the wrong way (72.33 -> 66.93). It is a fitted artifact - do not
  land it.
- **Per-site typed locals** instead of one reused `CStatusBarItem* it` - byte-identical
  (71.58 either way). Worth doing for cleanliness, not for the match.

## Consequences for the worklist

Any function whose `eh 0->1` differs in
`python -m gruntz.audit.insn_count`-style prologue comparison and which contains a `new` of
a deeply-derived class is in this family. Until the heuristic is understood these are
`@early-stop` walls, and the OOL bodies retail emitted (`??0CSBI_RectOnly@@QAE@XZ` @0x101fa0
is still unnamed) need `RVA_COMPGEN` pins in whichever TU emits the COMDAT.

## MOSTLY BROKEN 2026-08-09: the split is per-CLASS at both ends of the chain

The `??0CLoadable` result (`ob1-inline-budget-divergence.md` § RESOLVED) says to run
`sema xref` on the callee one level UP and read each `new`'s `??_7` stamp before calling
a cut a budget wall. Doing that here changes the verdict for most of the sites.

`sema xref 0x001005d0` lists exactly four callers of `??0CStatusBarItem@@QAE@XZ`, all
`CStatusBarMgr` builders. Pairing every `push <size>; call ??2@YAPAXI@Z` in those four
with the ctor call that follows it and with the `??_7` the site stamps gives 38 sites:

| most-derived class (by `??_7`) | none | `call ??0CStatusBarItem` | `call ??0CSBI_RectOnly` |
|---|---|---|---|
| `CSBI_RectOnly` / `CSBI_GruntMachine` / `CSBI_StatzTabGruntBar` (depth 2) | **6** | 0 | 0 |
| `CSBI_Image` (depth 3) | 4 | **10** | 0 |
| `CSBI_MenuItem` (depth 4) | 6 | **11** | 5 |
| `CSBI_ImageSet` (depth 4) | 6 | **9** | 6 |
| `CSBI_ImageSetAni` / `CSBI_WarlordHead` / `CSBI_StatzTabArrow` / `CSBI_WellGoo` | 0 | 0 | **8** |

The two ends are unanimous or near-unanimous, and both are expressible with the
tagged-sibling recipe of `two-shapes-need-two-entities.md`, because retail carries BOTH
`??0CStatusBarItem` (23 B) and `??0CSBI_RectOnly` (27 B, the base ctor folded in) as real
standalone COMDATs - which by that pattern's probe table means retail's source had an
out-of-line definition for each, plus an inline sibling for the expansions:

```cpp
class CStatusBarItem {
    CStatusBarItem();                       // out-of-line, 0x1005d0
    enum ENoSeed { NO_SEED };
    CStatusBarItem(ENoSeed) { ...same 4 stores... }
};
class CSBI_RectOnly : public CStatusBarItem {
    CSBI_RectOnly();                        // out-of-line, 0x101fa0 (base folded in)
    enum EInlineSelf { INLINE_SELF };
    CSBI_RectOnly(EInlineSelf) : CStatusBarItem(CStatusBarItem::NO_SEED) { ... }
    enum EBaseCall { BASE_CALL };
    CSBI_RectOnly(EBaseCall) { ... }        // calls the out-of-line base
};
class CSBI_Image : public CSBI_RectOnly {
    CSBI_Image() : CSBI_RectOnly(BASE_CALL) { ... }
    enum ECallRectOnly { CALL_RECTONLY };
    CSBI_Image(ECallRectOnly) : CSBI_RectOnly() { ... }   // calls 0x101fa0
};
```

`new CSBI_RectOnly` takes `INLINE_SELF`; the depth-2 siblings seed their base with
`CStatusBarItem::NO_SEED`; `CSBI_MenuItem`/`CSBI_ImageSet`/`CSBI_WellGoo` and everything
below take a `CALL_RECTONLY` chain where the census says so.

Measured: `BuildStatusBarTabs` 71.58 -> **78.21**, which is exactly the number the banned
`#pragma inline_depth(3)` scored - so that pragma was not a lever, it was a *symptom* of a
missing entity. `BuildTabzDialog` 79.68 -> 84.98, `LoadTabSprites` 76.12 -> 82.41,
`CAniPlayer::RenderCel` 75.64 -> 98.48, `CSBI_Image::Render` 74.04 -> 100.00, and
`??0CSBI_RectOnly@@QAE@XZ` goes from unclaimed to 100.00 EXACT (+1 denominator).

**What is left is a real per-site residue and it is small.** `CSBI_MenuItem` and
`CSBI_ImageSet` genuinely take all three shapes; the majority is modelled and the rest is
budget. `BuildGameMenu` 72.33 -> 66.93 is where that concentrates (its seven MenuItem sites
are 3 `CSBI_RectOnly` / 3 `CStatusBarItem` / 1 fully inline).

**Method, reusable:** don't eyeball the cut depth. Pair `operator new` size, the following
ctor call and the `??_7` reloc per site over EVERY caller the xref lists, then tabulate by
class. A class that is unanimous is an entity you are missing; a class that is split is the
residue.

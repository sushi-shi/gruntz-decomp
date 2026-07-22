# Multi-level /GX destructor — model the WHOLE base chain as a local polymorphic hierarchy (RTTI auto-namer names every ??_7)
tags: cpp:dtor cpp:eh cpp:virtual | asm:mov asm:call | topic:codegen-idiom topic:eh
symptoms: a most-derived ~Dtor that re-stamps N base vtables (`mov [esi],&g_vtbl_*` x N) and calls N teardown helpers, with push -1/mov fs:0,esp + a descending [esp+0x10]=K..0/-1 trylevel chain; the manual-vptr model is frameless (~34-43%) because no base SUBOBJECT is non-trivial
confidence: 9/10
variants: inline-base-dtor-folds-into-leaves.md, eh-dtor-needs-base-subobject.md, eh-dtor-model-members-as-destructible.md

The deep SBI dtor chain (`CSBI_StatzTabArrow : CSBI_ImageSetAni : CSBI_ImageSet :
CSBI_Image : CSBI_RectOnly : CStatusBarItem`, up to 6 levels) is the multi-level
case of `inline-base-dtor-folds-into-leaves`. Define the ENTIRE chain locally with
a non-trivial **inline virtual** dtor per base level (each calls its reloc-masked
member-teardown helper) and one out-of-line RVA-keyed dtor for the leaf; MSVC folds
every base teardown into the leaf and — because each base subobject is now
non-trivial — emits the full `/GX` frame + the per-level vptr re-stamps. The
catalog auto-namer (`config/vtable_names.csv`, `gruntz.core.vtable_scan`) names
every `??_7CSBI_*@@6B@` on the target, so the stamps reloc-mask with NO manual
`RVA_COMPGEN` pin and NO `DATA(&g_vtbl_*)` extern.

```cpp
struct CStatusBarItem { virtual ~CStatusBarItem(); /*+10 virtuals*/ void DtorStatus(); };
inline CStatusBarItem::~CStatusBarItem() { DtorStatus(); }            // reloc-masked helper
struct CSBI_RectOnly : CStatusBarItem { virtual ~CSBI_RectOnly(); void DtorRect(); };
inline CSBI_RectOnly::~CSBI_RectOnly() { DtorRect(); }
struct CSBI_Image : CSBI_RectOnly { virtual ~CSBI_Image(); virtual void Imf1(); void DtorImage(); };
// ... up the chain ...  leaf is the only OUT-OF-LINE (RVA-keyed) dtor:
RVA(0x00100870, 0x6a) CSBI_Image::~CSBI_Image() { DtorImage(); }
```
```asm
100870: push -1 / push <handler> / mov fs:0,esp        ; /GX frame (from non-trivial bases)
        mov [esi],offset ??_7CSBI_Image@@6B@ / call DtorImage
        mov [esi],offset ??_7CSBI_RectOnly@@6B@ / call DtorRect    ; base dtors folded in
        mov [esi],offset ??_7CStatusBarItem@@6B@ / call DtorStatus
```
STEERABLE — the per-level teardown call is reloc-masked, so one uniform chain matches
every leaf regardless of which engine fn each level calls. Evidence: 9 SBI leaf dtors
(`~CSBI_Image` 0x100870 … `~CSBI_StatzTabArrow` 0x1048f0, `~CSBI_WellGoo` 0x104bb0)
all 43%→100%. Resolves the `eh-dtor-needs-base-subobject` wall for the RTTI-catalog case.

**Dual-dtor TU → SPLIT, one TU per leaf.** When TWO leaves on the SAME chain are
RVA-keyed in the same source (e.g. `~CSBI_RectOnly` 0x100700 AND `~CSBI_ImageSet`
0x102000, both over `…CSBI_RectOnly : CStatusBarItem`), they CANNOT share a TU: the
shared mid-chain base (`CSBI_RectOnly`) must be the **out-of-line RVA-keyed leaf** in
one and an **inline base** (folded into the deeper leaf) in the other — one class
cannot be both in a single TU. Give each leaf its own TU (`SBI_RectOnlyDtorEh.cpp` =
2-level chain, `SBI_ImageSetEh.cpp` = 4-level chain), each defining its full chain
locally with the bases below it `inline` and only its own leaf out-of-line. Matching-
neutral (RVA-keyed); the dtor mangling flips `??1…@@QAE@XZ`→`@@UAE@XZ` (now virtual) —
expected, auto-derived. Drop the old manual-stamp model's `g_vtbl_*` DATA externs from
the source: unreferenced externs aren't in the base obj so they don't name anything,
and the catalog auto-namer names every `??_7…@@6B@` the new chains emit. Evidence:
0x100700 34%→100%, 0x102000 43%→100% (the dual-dtor TU the 9-leaf pass left parked).

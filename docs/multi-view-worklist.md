# Multi-view worklist (NO-MULTIPLE-VIEWS sweep)

Fan-out worklist for the user's hard rule: **if the same physical memory is read
from the same address via C-casts to DIFFERENT types at CONSISTENT offsets, those
views ARE one class → collide into ONE canonical class** (fold the fields at their
offsets as real members, re-home the methods, delete the view structs). Cast +
offset == member access at `/O2`, so a merge is matching-neutral (verify per-fn,
not just the headline).

## HARD RULES (user directive)

- **No class/struct definitions in `.cpp` files — ALL class/struct defs live in
  headers** (`include/`). A `.cpp`-local class is a reconstruction hack, NOT code the
  original devs wrote.
- **No local views.** A per-TU inline re-declaration of a class (a "view") is the
  multi-view anti-pattern itself — fold it into the ONE shared header class.
- Genuine data-record structs that a single TU builds/reads (on-stack file headers,
  packed source descriptors) still belong in a header — put them in a dedicated,
  narrowly-included header if adding them to the class's main header perturbs other
  TUs (an MSVC cross-function codegen leak; see `include/Image/CFileImageRecords.h`).
- These override the "keep byte-exact byte-exact" preference: recover the devs' one
  true shape even at the cost of a hair of match% (correctness-not-artifacts).

Regenerate the raw signals:

```
# same class name with a real body in 2+ files
for f in $(git ls-files 'include/**.h' 'src/**.cpp'); do \
  grep -oP '^\s*(class|struct)\s+\K[A-Za-z_][A-Za-z0-9_]*(?=\s*(:.*)?\{)' "$f" | sort -u | sed "s#\$# $f#"; \
done | awk '{print $1}' | sort | uniq -c | sort -rn | awk '$1>=2'

# placeholder-view name classes (Host/Self/View/Owner/Root/Peer/Holder + RVA-hex suffix)
grep -rhoP '^\s*(class|struct)\s+\K[A-Za-z_][A-Za-z0-9_]*(?=\s*(:.*)?\{)' include src \
  | sort -u | grep -E 'Host$|Self$|View$|Owner$|Root$|Peer$|Holder$|_[0-9a-f]{4,6}$'
```

## EXCLUDED (do NOT merge)

- **Forward declarations** (`class X;` no body) — not a view.
- **Nested `struct Vtbl`** (14×) — each is a distinct COM/dispatch vtable, not a
  field-view.
- **Required foreign-vtable pointer-only DISPATCH interfaces** — hold NO data, only
  vtable slots so a `mov eax,[p]; call [eax+slot]` lands right for a vtable `cl`
  can't emit. Keep: `CFileImageVtblView`/`IDirectDrawSurfaceZ`/`CFileImageHeldSurface`
  (surface 0x5ef7f0 + DDraw COM), `CPoolItemAVtbl` (0x5ef7f0), and any `*Vtbl`.
- **`ClassUnknown_N`** single-fn shells — per match-queue-priorities, skip.
- **Genuinely-DIFFERENT same-name classes** → those need RENAME, not merge; flagged
  `[VERIFY: may be REQUIRED split]` below (e.g. the MFC/Win32 `CGameMgr`/`CGruntzMgr`
  0x24556c dual-view is a REQUIRED split per campaign memory — do NOT force-merge).

---

## CATEGORY 1 — Cross-module FIELD-VIEW collisions (verified; the flagged cluster)

### CFileImage  [MERGED THIS RUN — see Deliverable B]
Canonical: `CFileImage` (Image module, `include/Image/Image.h`).
Folded views: the 2nd `class CFileImage` in `include/Image/CFileImage.h` (deleted),
`CFileImageInfo` (palette/format-config view at +0x538/+0x53c/+0x93c == CFileImage's
own palette fields → it is a 2nd CFileImage instance passed as `info`), and the
held-surface field (unified to one `IDirectDrawSurfaceZ* m_8` at +0x08).
Dual-named-@-one-RVA reconciled: SaveBmp/Save24 @0x1443b0, SaveTga/Save8 @0x144900,
DecodeRun/DecodeBmpData @0x143cf0, Decode/DecodePcxData2 @0x144b30.

### CDDSurface  [DEFER — RISKY cross-module rename; fan-out]
Same physical class as `CFileImage` — the DIRSURF.CPP DirectDraw surface. Proof:
`Lock @0x13e6d0` is defined ONCE and shared; the layout matches (m_8@+0x08,
m_c@+0x0c, DDSURFACEDESC scratch @+0x10 with height/width/pitch @+0x18/+0x1c/+0x20,
lPitch @+0x34, bit-depth @+0x64, state @+0x7c, +0x94 owned array, raw-bpp @+0xa8).
Three defs: `include/DDrawMgr/CDDSurface.h`, `include/Stub/discovered.h`,
`src/Gruntz/CDDSurfaceDtor.cpp`. **~19 COM-thunk methods currently named
`CDDSurface::` (Lock/Flip/Blt/BltEx/BltFast/GetColorKey/SetColorKey/SetPalette/
Refresh) are BYTE-EXACT (100%) in `cdirectdrawmgr`** while the sibling
decoders/blitters/save path are named `CFileImage::` (also same object).

Why deferred, not done here: this class is compiled in ≥3 TUs, each emitting its
own destructor body at a DISTINCT RVA — `??1CFileImage@@UAE` (0x141350, virtual),
`??1CFileImageSurface@@UAE` (0x142360, derived), `??1CDDSurface@@QAE` (0x142a40,
non-virtual). Those dtor symbols CANNOT collapse to one name (name collision in the
PDB), so a literal single-name merge is impossible — the multi-copy dtor is a
**REQUIRED name-split** (like the CGameRegistry/CGruntzMgr 0x24556c split). Renaming
the 8+ byte-exact `CDDSurface::` COM thunks to `CFileImage::` risks regressing them
and is reloc-masked-neutral at best.
Fan-out task: (a) unify the LAYOUT to ONE shared header (`CDDSurface.h` ↔ `Image.h`
CFileImage already agree offset-for-offset — make one include the other or a shared
base), (b) pick ONE method-name family for the shared thunks and update callers
(reloc-masked, verify each byte-exact fn stays byte-exact), (c) keep the 3 dtor
copies distinctly named. Module: DDrawMgr + Gruntz + Image. ~40 methods, ~30 casts.

---

## CATEGORY 2 — Same-name class re-declared INLINE in N TUs (recover ONE shared header)

Anti-pattern (CLAUDE.md): "Never re-declare a class inline per-TU — that is a
different shape in each TU and diverges; recover the single shared header." Each
inline redecl is a view. `(hdr N)` = how many of the N defs already live in
`include/`; the rest are per-TU stragglers to fold into the shared header (or, if
`hdr 0`, CREATE the shared header). Count = # files with a body.

Strongest (no shared header at all — `hdr 0`):
- `CGameReg` ×10 (hdr 0) — Gruntz. Biggest. 10 TUs each hand-roll it. → create `include/Gruntz/CGameReg.h`.
- `MgrSettings` ×7 (hdr 0) — Gruntz.
- `CAnimSink` ×6 (hdr 0) — Gruntz.
- `LogicFnTable` ×5 (hdr 0) — Gruntz (grunt-AI family — matcher-6 territory).
- `CDDrawSubMgrLeafScan` ×5 (hdr 0) — Gruntz.
- `NameVec` ×4, `CTypeKeyColl` ×4, `CTypeColl` ×4, `CStepList` ×4, `CScanGrid` ×4,
  `CScanCell` ×4 (all hdr 0) — Gruntz.
- `CString` ×4 (hdr 0) — **MFC class; should come from `<Mfc.h>`, not be hand-declared**
  (real-MFC-headers convention). Delete the 4 hand decls, include `<Mfc.h>`.
- ×3 hdr 0: `MgrSub30`, `MgrSub24`, `MgrGrid`, `Inner`, `CTypeNameEntryView`,
  `CStepList2`, `CStatusBarHolder`, `CScanRectInit`, `CScanNode324`, `CSBI_SideTab`,
  `CPlayLevelLoad`.

Partial adoption (shared header exists; fold the stragglers):
- `WwdGameReg` ×6 (hdr 1), `CPlay` ×6 (hdr 2), `CGrunt` ×6 (hdr 2, grunt-AI/matcher-6),
  `CSerialSub34` ×5 (hdr 2), `CMovingLogic` ×5 (hdr 3), `CDDrawWorkerMgr` ×5 (hdr 1),
  `CDDrawSurfaceMgr` ×5 (hdr 1), `CContainerErr` ×5 (hdr 3), `MinervaInner` ×4 (hdr 1),
  `CSymParser` ×4 (hdr 1), `CStatusBarItem` ×4 (hdr 2), `CSpriteRefTable` ×4 (hdr 1),
  `CSpotLight` ×4 (hdr 1), `CSBI_RectOnly` ×4 (hdr 1), `CSBI_ImageSet` ×4 (hdr 2),
  `CGruntTileMgr` ×4 (hdr 1), `CDDrawWorkerRegistry` ×4 (hdr 1), `CImageSet` ×3 (hdr 2),
  `Font` ×5 (hdr 1), `CDirectDrawMgr` ×3 (hdr 1), `CNetMgr` ×3 (hdr 1),
  `DirectSoundMgr` ×3 (hdr 2), `CSeverusWorker` ×3 (hdr 3), `CWwdGrid` ×3 (hdr 1),
  `CSymTab` ×3 (hdr 1), `CLogicTypeBuilder` ×3 (hdr 1), `CObjList` ×3 (hdr 3),
  `CUserLogic` ×3 (hdr 3), `CUserBase` ×3 (hdr 3), `CGruntzSoundZ`/`CGruntzSoundInnerZ`/
  `CGruntzSingleCommand` ×3 (hdr 2).

`[VERIFY: may be REQUIRED split]` (do NOT blind-merge; confirm one layout first):
- `CGameMgr` ×4 (Net/NetMgr.h + Wap32/Wap32.h + 2 src) — MFC/Win32 dual-view per memory.
- `CGruntzMgr` ×5 (CSoundFxEmitter.h + GruntzMgr.h + 3 src) — 0x24556c dual-view per memory.

## CATEGORY 3 — Placeholder-view ALIASES (Host/Self/View/Owner/Root/Peer + RVA-suffix)

**474 placeholder view classes** matching the name pattern — each is (almost always)
a cast-shell aliasing a real class's `this`/pointer to reach its fields, to be folded
into that real class. Mechanical, high-volume; fan out **per family prefix**. Fold
`X{Host,Self,View,Owner,Root,Peer}` and `B_<rva>` / `<name>_<rva>` shells into the
one class they alias (the class owning that RVA / those offsets), delete the shell,
drop the cast. Biggest family prefixes (view-name count):

    Menu 11   Sbi 7   World 6   Wwd 5   Mi 5   Booty 5   Tele 4   Proj 4
    Net 4   Grunt 4   Dbg 4   Chat 4   Attract 4   Ani 4   Statz 3   Snd 3
    Sb 3   Rez 3   Plane 3   Palette 3   Img 3   Game 3   Credits 3   Combat 3

Worked examples of the shape (each `[views] -> canonical`):
- `CAttract{Host,Owner,Self}` -> the attract-loop class (Gruntz/CAttract.cpp).
- `CChatBox{DcHost,Owner,RegRoot,TextHost}` -> the chat-box class (Gruntz/ChatBox.cpp).
- `CCredits{DrawRoot,DrawView}`, `CCreditz{ImageRoot,Owner}` -> the credits class.
- `CBooty{AnimSelf,FlushView,MusicHost,OwnerView}` -> the booty class.
- `Action{Host,Peer,Sub}_0d7220` -> the class whose method is at 0x0d7220.
- `Ail{Host_138490,Mgr_1384f0,Seq_138c20,Slot_1384f0}` -> the AIL audio classes
  (some already re-homed to ailseq — see ApiCallers memory).
- `B_<rva>` (`B_13dee0`, `B_1413c0`, `B_151d20`, ...) -> the class owning `<rva>`.

Caveat: a `*Vtbl`-only shell (no data members, only virtuals) is a KEEP (dispatch
interface), not a fold — check the body before folding.

---

## Recommended fan-out order

1. CDDSurface layout-unify + method-name reconcile (Category 1; 1 focused matcher).
2. `CString` → `<Mfc.h>` (trivial, high count) + the `hdr 0` Category-2 families
   (create shared headers) — 1 matcher per ~3 families.
3. Category-3 per-prefix (Menu, Sbi, World, Chat, Attract, Booty, Credits, ...) —
   1 matcher per family; skip grunt-AI (`CGrunt*`, `LogicFnTable`) if matcher-6 holds it.
4. `[VERIFY]` splits (CGameMgr/CGruntzMgr) last — confirm layout before any change.

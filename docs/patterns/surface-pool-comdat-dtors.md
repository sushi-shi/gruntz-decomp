# One surface hierarchy, five byte-identical COMDAT dtors under five names
tags: cpp:dtor cpp:vtable cpp:comdat | asm:mov asm:call | topic:wall topic:identity topic:scoring-artifact
symptoms: five `??1C...@@UAE@XZ` at different RVAs with IDENTICAL code bytes (`mov [esi],0x5ef7f0; call FreeSurfaces(0x13e4d0); lea ecx,[esi+0x94]; call 0x1b4f3e`); `?g_poolItemVtbl@@3PAXA` RTTI-less vtable; same class modeled as CDDSurface / CFileImage / CPoolItemBase in different TUs
confidence: 9/10

## The shape

The WAP32 0xc0-byte surface object (vtable `0x5ef7f0`, RTTI-less, delinker-named
`?g_poolItemVtbl@@3PAXA`) is a **1-base + 4-derived** hierarchy. Each vtable's
slot-0 `??_G` scalar-deleting dtor calls the non-deleting dtor 0x20 bytes later
(`??_G` is `push esi; mov esi,ecx; call <~>; test [esp+8],1; je; push esi; call
operator delete; ...; ret 4`), so the true OWNER of each `~` is fixed by which
vtable references its `??_G`, NOT by the byte pattern:

| vtable (VA) | ??_G      | ~ (non-del dtor) | owner (this campaign)          | owning TU |
| ----------- | --------- | ---------------- | ------------------------------ | --------- |
| 0x5ef7f0 base | 0x141330 | 0x141350         | CFileImage / CDDSurface / CPoolItemBase | image |
| 0x5efa58 a58  | 0x142340 | 0x142360         | CFileImageSurface / CPoolItemA | image |
| 0x5efa88 a88  | 0x142800 | 0x142820         | CPoolItemA88                   | ddrawptrcollections |
| 0x5efab8 ab8  | 0x142a20 | 0x142a40         | CPoolItemAB8                   | ddrawptrcollections |
| 0x5efae8 ae8  | 0x142d20 | 0x142d40         | CPoolItemAE8                   | ddrawptrcollections |

All FIVE non-deleting dtors are **byte-identical** because the derived vptr stamp
(`mov [esi],&<derived vtable>`) folds as a dead store — the inlined base
`~CPoolItemBase` immediately re-stamps `0x5ef7f0`, leaving one vptr store whose
DIR32 operand is reloc-masked. So identical code bytes do NOT mean "copies of one
class": they are five DISTINCT classes in one hierarchy.

## Why they land in different TUs (COMDAT selection, not folding)

MSVC 5.0 emits each class's inline dtor as a COMDAT in every TU that `new`s /
destroys that class. The linker keeps ONE copy per symbol (COMDAT *selection* —
the default) and discards the rest; it does NOT fold *different* symbols with
identical bytes (that is `/OPT:ICF`, off by default). Both the image TU and the
ddraw pool `new` base + a58 items, so those two dtor COMDATs exist in both — the
linker kept the IMAGE copies (0x141350 / 0x142360). a88/ab8/ae8 are `new`'d only
by the pool, so their dtors live only in the ddraw region.

## Reconstruction rule

- Give each derived class an out-of-line `~T() {}` with the RVA the vtable's
  `??_G` points at (a88→0x142820, ab8→0x142a40, ae8→0x142d40). Getting the wrong
  RVA still matches (bytes are identical + reloc-masked) but MISLABELS the class —
  a semantic bug the vtable dump refutes. Confirm ownership with the raw
  `.rdata` vtable dump (slot 0 → `??_G` → `~`), never the byte pattern.
- The base + a58 dtors are the image TU's kept COMDATs; in the ddraw TU those two
  classes are declared-only (their COMDAT was the discarded copy) so no duplicate
  RVA is claimed.

## The cross-module identity (documented, merge deferred)

The base (0x5ef7f0) is modeled under THREE names — `CDDSurface` (<DDrawMgr/
DDSurface.h>, the widely-included surface-op view), `CFileImage` (<Image/Image.h>,
the fuller BMP/PCX/PID-loader view), `CPoolItemBase` (DDrawPtrCollections.cpp, the
pool view) — and a58 under two (`CFileImageSurface` / `CPoolItemA`). Same physical
class; RTTI-less so no ground-truth name. A full name-merge would force the +0x94
`CByteArray` member onto ~30 pointer-only `CDDSurface` includers (the MFC ripple)
and reconcile CDDSurface's placeholder vtable (`v00..v20`) into the real slot roles
(dtor/Refresh/Init1/BlitSurf/FreeSurfaces) that a few TUs call — a large deliberate
sweep, not a byte-match blocker. Until then the identity is documented on each view.

## Pool-B sibling (unrelated hierarchy)

The pool's +0x498 list holds `CDDPalette` (DIRPAL.CPP, 0x38-byte RezAlloc'd, vtable-
less), NOT a surface. Proven by method-RVA equality: the pool's Init/Init2/Init3/
Teardown ARE CDDPalette's CreateRGB(0x1474d0)/LoadFromFile(0x147410)/
CreateFromTrailing(0x147840)/Destroy(0x147530); Create@0x143040 `new`s + returns
`CDDPalette*` (mangled `PAUCDDPalette` ⇒ struct). No shared base with the surface
family — pool-A = surfaces, pool-B = palettes.

## Same mangled name at multiple RVAs: pipeline-TOLERATED, model-INVALID

The step-0 experiment (two units emitting `??1CFileImageSurface@@UAE@XZ` at two
RVAs) proved the PIPELINE copes: `symbol_names.csv` carries `(unit, name)` rows,
the delink is RVA-driven, objdiff pairs per-unit — nothing explodes. Treat that
strictly as a **refactor safety property** (a transitional duplicate mid-rename
won't break the build), NEVER as a modeling license:

- **Linker theorem:** MSVC5 keeps ONE COMDAT copy per symbol name. N similar
  retail bodies at N RVAs ⇒ the original link had N DISTINCT names ⇒ N distinct
  classes/functions. A reconstruction that *needs* one name at two retail RVAs
  contradicts the binary — it is a mis-model (exactly how this family's
  "5 copies of one dtor" premise was disproven).
- **Tooling cost:** duplicate names break name→RVA injectivity (sema reverse
  lookups, Ghidra navigation, xref-by-name become ambiguous).

Rule: a persistent duplicate mangled fn name across units is a defect to
resolve (find the distinct real identities), not a state to keep.

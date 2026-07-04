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
| 0x5ef7f0 base | 0x141330 | 0x141350         | CDDSurface (UNIFIED)           | image |
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

## The cross-module identity (MERGED — the base is one class `CDDSurface`)

The base (0x5ef7f0) was modeled under THREE names — `CDDSurface` (<DDrawMgr/
DDSurface.h>, the widely-included surface-op view), `CFileImage` (<Image/Image.h>,
the fuller BMP/PCX/PID-loader view), `CPoolItemBase` (DDrawPtrCollections.cpp, the
pool view). Same physical class; RTTI-less so no ground-truth name, but EVERY method
references `C:\Proj\DDrawMgr\DIRSURF.CPP`, so it is one DDrawMgr class from DIRSURF.CPP
and `CDDSurface` (= DirectDraw Surface = DIRSURF) is the evidenced name. The three
views are now UNIFIED into one `class CDDSurface` in <DDrawMgr/DDSurface.h>:
- DDSurface.h absorbs Image.h's full model (all method decls + layout + the 9-slot
  polymorphic vtable dtor/Refresh/Init1/BlitSurf/FreeSurfaces/IsValid/v18/RestoreLost/
  v20), and pulls <Mfc.h> for the real +0x94 member. That member is a **CPtrArray**
  (`m_elements`), NOT a CByteArray — the FreeSurfaces disasm (0x13e4d0) walks
  `[+0x98][i*4]` and `delete`s each element via its slot-0 vdtor, i.e. an array of
  polymorphic OBJECT POINTERS; the "CByteArray" FLIRT name was a misattribution.
- Name-preserving field merge (semantic wins): m_pos (pool POSITION @+0x04), m_8/m_c
  (held surfaces), m_lockBits (was m_34), m_dontOwn (was m_7c), m_bitDepth (was m_a8).
- Image.h drops CFileImage (keeps CRezImage + the sibling helpers); all Image-module
  bodies are now `CDDSurface::`. DDrawPtrCollections.cpp drops the local CPoolItemBase;
  CPoolItemA/A88/AB8/AE8 derive from `CDDSurface`.
- The dtor split: the image TU keeps the out-of-line `~CDDSurface` @0x141350 (its kept
  COMDAT); the ddraw TU defines `~CDDSurface` inline (before the derived classes) so
  each derived dtor still inlines the teardown. The build delinks per-unit, so the two
  definitions do not collide. The a58 pair (CFileImageSurface / CPoolItemA) stays two
  classes for the same reason (a SEPARATE unification, not yet done).
- MFC ripple: the Win32 deref-consumers include DDSurface.h (with <Mfc.h>) FIRST;
  pointer-only TUs that need only IDirectDrawSurface (e.g. LightFxRender) stay off it.
Net matching impact was +0.01% fuzzy (one -1 exact = fileimageloadbyext's pre-existing
scheduler coin-flip, re-triggered as its @early-stop comment predicts).

## Pool-B sibling (unrelated hierarchy)

The pool's +0x498 list holds `CDDPalette` (DIRPAL.CPP, 0x38-byte RezAlloc'd, vtable-
less), NOT a surface. Proven by method-RVA equality: the pool's Init/Init2/Init3/
Teardown ARE CDDPalette's CreateRGB(0x1474d0)/LoadFromFile(0x147410)/
CreateFromTrailing(0x147840)/Destroy(0x147530); Create@0x143040 `new`s + returns
`CDDPalette*` (mangled `PAUCDDPalette` ⇒ struct). No shared base with the surface
family — pool-A = surfaces, pool-B = palettes.

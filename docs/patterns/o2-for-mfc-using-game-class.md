# A game class that USES MFC types (CString members, stack CFileIO) is still /O2 — not /O1
tags: cpp:mfc | asm:mov asm:and | topic:flags topic:codegen-idiom
symptoms: a TU full of CString/CFileIO usage plateaus in the 40s-90s% under /O1 (mfc profile); `and $0,mem` for `x=0`, tail-merged early-return epilogues, jmp-to-shared-ret in tiny accessors
confidence: 9/10
variants: o1-for-mfc-derived.md

The `/O1` rule (see o1-for-mfc-derived.md) is about **NAFXCW library code** — the actual `CFile`/
`CObject`-clone implementations MFC itself shipped /O1. A GAME-LOGIC class that merely *embeds* MFC
types (`CString` members, constructs a `CFileIO` on its stack, calls `AfxThrow*`) is still **engine
code → /O2** (the project-locked default). Provenance is the toolchain split, NOT "does it touch
MFC types." If an MFC-using TU plateaus under the `mfc` (/O1) profile, try the `eh` (/O2 /GX)
profile before assuming a per-function wall.

```toml
# config/units.toml — an MFC-using game class wants /O2 + the EH frame, not /O1
[[unit]]
unit = "savegame"
source = "src/Io/SaveGame.cpp"
flags = "eh"   # NOT "mfc"
```
```asm
; /O1 (wrong here): size-optimized                 ; /O2 (retail): speed-optimized
and  dword ptr [esi+8], 0                           mov  dword ptr [esi+8], 0
xor  eax,eax / jmp <shared ret>   (tail-merge)      xor  eax,eax / pop esi / ret  (inline per site)
```
STEERABLE (per-TU `flags`). Evidence: CSaveGame (the save-slot manager that owns two CString
members + builds stack CFileIO temps) went **1/18 → 13/18 byte-exact** flipping `mfc`→`eh`; the /O1
tells were `and $0,mem`, tail-merged `return 0` epilogues, and jmp-to-shared-ret in GetSlot/leaf
accessors — all gone under /O2.

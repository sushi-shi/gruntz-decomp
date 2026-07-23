# ILT: retail is linked `/INCREMENTAL`, so a vtable slot holds a `jmp` thunk, not the body
tags: asm:jmp | topic:delinker topic:scoring-artifact topic:reloc-fidelity
symptoms: a vtable slot's delinked reloc names an unrelated class (`~CTriggerMgr`, `GroupAllScored`, `SetupImage`, `WaitForOtherPlayers`), or `thunk_FUN_...`; an `RVA_COMPGEN`/`RVA()` pin whose address is `0x1005..0x44a8` and whose body is only 5 bytes; `.rdata` section stuck below 100% with correct-looking code
confidence: 10/10

Retail `GRUNTZ.EXE` was linked **`/INCREMENTAL`**. link.exe therefore emits an
**incremental-link thunk table (ILT)** at the very start of `.text` and routes
*function-address* references through it, so a body can move on a relink without
rewriting every reference. On GRUNTZ.EXE the band is exact and structural:

```
.text rva 0x1000: cc cc cc cc cc          <- 5 bytes of INT3 pad
      rva 0x1005: e9 46 68 06 00          <- ILT entry 0   (jmp rel32)
      rva 0x100a: e9 11 75 0e 00          <- ILT entry 1
      ...                                    2695 entries, 5 bytes each
      rva 0x44a3: e9 88 d2 00 00          <- last entry
      rva 0x44a8: cc cc cc cc ...         <- INT3 pad terminates the band
```

So a vtable slot does **not** hold its method's address. `??_7CGuardPoint@@6B@`
slot 1 holds `0x00401a32`; those bytes are `e9 39 e9 00 00` = `jmp 0x010370`, and
`0x010370` is the real `?SerializeMove@CGuardPoint@@UAEHPAVCFileMemBase@@HHH@Z`.

**The thunk is a pure link-time artifact.** `cl` cannot name a symbol that does not
exist until link, so *every* `DIR32` in the original object named the **body**. This
is a theorem, not a guess — check any base obj: the vtable `.rdata` reloc is always
against the method symbol. It is the same shape as an IAT slot standing in for an
import.

Two consequences, both load-bearing:

- **Delinking must resolve through the thunk.** Otherwise the slot is named after
  whatever symbol sits at the thunk address. Ghidra propagates the *target's* name
  onto its thunks (1387 of the 1584 it carves in the band get "real-looking" names),
  so the delinked reloc reads `~CTriggerMgr` / `GroupAllScored` — garbage from an
  unrelated class, and the `.rdata` section can never match. `nix/patches/vostok-ilt-thunk-resolution.patch`
  detects the band structurally and hops to the destination (only when a function
  symbol sits exactly there, so an unnamed destination is left alone).

- **Never pin a symbol to a slot's stored VALUE.** Reading a vtable slot and binding
  `RVA_COMPGEN`/`RVA()` to what it contains pins the symbol onto a 5-byte `jmp`,
  claiming the thunk IS the function. `src/Gruntz/UserLogic.cpp` had exactly this for
  14 `CUserLogic`/`CUserBase` virtuals (`0x1361`, `0x242d`, …); the real bodies are the
  contiguous `0x87d0..0x89f0` cluster of tiny defaults (`ret`, `mov eax,1; ret`).
  **Pin the thunk's destination.**

## Recognizing it

- An address in `0x1005..0x44a8` that is 5 bytes long is an ILT entry, never a body.
- `gruntz sema xref` prints these as `(thunk-band)` when chasing callers.
- The delinker announces the band: `[delink] incremental-link thunk table: 2695 entries`.

## Reverse use: collapse invented helper declarations

An unresolved source call can carry any invented name and signature while its
retail call site still resolves to an ILT entry. Before preserving such a
declaration, chase that entry to its body and compare the real mangled
signature. Several differently named placeholders may be one existing
function.

The Grunt voice gates demonstrated this directly: calls spelled `CueVisible`,
`GruntPointVisible`, and `BoardTest` all target ILT `0x00001127`, which jumps to
the already exact
`CGameLevel::PointInBounds(const LevelCoordRect*, i32, i32)` at `0x0006b330`.
Replacing the placeholders at all call sites removed two declared-only
functions and one misleading extern without adding a reconstruction target.
The odd `GruntPointVisible(y, x, rect)` spelling had only preserved the cdecl
push order; restoring `PointInBounds(rect, x, y)` emits the same pushes with the
real semantics.

The same audit applies when a call lands inside a larger function rather than
in the ILT band. A named destination at an internal shared tail is not evidence
for a standalone helper. Chase the tail's calls and receiver instead. The
Grunt entrance-animation tails both resolve to
`CWwdGameObjectA::ApplyLookupSprite`; replacing the invented
`EntranceApplyFrame` declaration with the real member call removed another
declared-only function.

A thin wrapper that is only `call <member>; ret` also preserves the callee's
return register. If its caller consumes EAX, a `void` reconstruction of that
member is disproved even when the source-inferred name looked plausible.
`CGrunt::TileSwitch` exposed `CGrunt::StepArrivalDrop` this way: restoring the
real `i32` return type removed the fake `GruntTileSwitchImpl` alias.

## Trap: FID false-positives on the destinations

The ILT-reached bodies are often 1-6 byte defaults (`c3`, `33 c0 c3`, `b8 01 00 00 00 c3`).
Those byte-match dozens of CRT stubs, so FID/FLIRT anchors them as library rows at LOW
confidence — `__fpclear` was claimed at **four** distinct RVAs at once, which is self-
refuting (one CRT function cannot be at four addresses). The vtable evidence arbitrates:
`0x88d0` is reached from **69 vtable slots, always slot 6** (`??_7CUserLogic@@6B@`,
`??_7CActionArea@@6B@`, …). A CRT routine cannot be slot 6 of 69 game vtables — the rows
were pruned from `config/library_labels.csv`.

## Related

- `??_E` vs `??_G` — a *separate*, composing mismatch on the same slots: cl emits the
  vector deleting dtor `??_E` as a COFF **weak external** whose aux record links to the
  scalar deleting dtor `??_G` (`llvm-readobj --symbols` → `AuxWeakExternal { Linked: ??_G... }`).
  The linker collapses them to one address, so the delinker can only ever spell it `??_G`
  while the base obj says `??_E`. The distinction is destroyed by linking and is NOT
  recoverable from the image — a genuine scoring artifact, not a thunk gap.

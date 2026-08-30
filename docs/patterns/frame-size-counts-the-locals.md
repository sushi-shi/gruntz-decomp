# `sub esp,N` is a LOCAL COUNT the masked diff hides - sweep the band for it

tags: cpp:local cpp:array | asm:sub asm:mov | topic:codegen-idiom topic:audit
symptoms: a function in the high 90s whose `gruntz walls diagnose --asm` and `gruntz walls diagnose`
  are both clean and whose instruction count matches, yet it is not 100. The
  prologue's `sub esp,N` differs by a multiple of 4 and EVERY `[esp+K]`
  displacement after it is shifted by the same amount - which `--diff` shows as a
  long run of one-operand mismatches that reads like regalloc noise.
confidence: 9/10

`sub esp,N` is the size of the local frame, so a difference is a hard statement
about **how many bytes of locals the function declares** - independent of every
other signal. It is the one invariant `--diff` (address-masked) and `--blocks`
(shape-only) are both blind to, and `insn_count` cannot see either, because
allocating a local costs ZERO instructions.

Three source facts commonly move it:

1. **An array's size.** `CFecFile`-style buffers are the usual case:
   `DrawSaveGameMenu` @0xe3f40 had `sub esp,0x20` against retail's `0x24`, and the
   4 bytes could only be the name buffer - `char name[0x20]` -> `char name[0x24]`
   took it to **100.00 EXACT**. Note the size argument passed to the API
   (`GetDlgItemTextA(..., 0x20)`) is the *limit*, NOT the buffer size; do not read
   the buffer's extent off the call.
2. **The number of distinct ADDRESS-TAKEN scalars.** A scalar that only ever lives
   in a register costs no slot (measured: adding `i32 shown = slot + 1;` did not
   move the frame at all), so the count is exactly the locals whose address is
   taken - `&x` passed to a read/lookup/out-param. `SoundStream::ParseWave`
   @0x137b70 declared five (`riffTag/riffSize/waveTag` + a loop-local
   `chunkId/chunkSize` pair) against retail's three: the dev **reused the header
   pair as the chunk pair**. 99.82 -> 99.97 and the frame matched.
3. **The declared scope of an address-taken local.** cl 5.0 packs stack homes by
   source scope, not only by machine live range. A loop-local aggregate may permit
   an earlier temporary to reuse a dead incoming parameter home, while the same
   real local declared at function scope prevents that reuse. In
   `CFecFile::AddFile` @0x17b950, moving the existing `MSG msg` declaration from
   the copy loop to function scope took the frame from `0x38` to retail's `0x3c`
   and made the function plus all of its EH helpers exact. See
   [function-scope-address-taken-local-blocks-param-home-reuse.md](function-scope-address-taken-local-blocks-param-home-reuse.md).

## The sweep

Frame size is cheap to check in bulk and needs no per-function reading:

```python
from gruntz.core import branches as B
bo, to = B.obj_paths(unit)
db, dt = B.decode(bo), B.decode(to)          # {symbol: [(off, mnemonic, operands)]}
# first `sub $N, %esp` in the first ~16 instructions of each side
```

First run over the 90-100 band: **26 mismatches**, i.e. 26 functions with a
provable local-count bug that no other tool reports. Two were opened, both were
real. Prefer this over re-reading a `--diff` that "looks like regalloc".

## What the full sweep found (2026-08-08, 21 open entries worked)

**Every mismatch opened was a real modelling fact - none was noise.** Four closed
outright and the rest each named a concrete mechanism. The population splits into
five causes, and only the first three are source-steerable today:

1. **A member POD read field-by-field instead of through its by-value accessor**
   ([byvalue-size-accessor-temp.md](byvalue-size-accessor-temp.md)) - the largest
   family. `CPlay::ResetViewport` 95.55 -> **100 EXACT**,
   `CMulti::WaitForOtherPlayers` 94.42 -> **100 EXACT**,
   `CPlay::SaveUnderAndDrawCursor` 90.00 -> 99.99, `CState::InputVirtual` 96.18 -> 99.54.
2. **An `i64` clamped in place** ([i64-clamp-homes-the-whole-quad.md](i64-clamp-homes-the-whole-quad.md))
   - `CTriggerMgr::HitTestApply` 91.33 -> 98.80, frame 8 -> retail's ZERO.
3. **A value hoisted into a local that retail re-reads at each use** -
   `CTeleporter::Update` 94.19 -> 98.91 (the `TeleporterKind kind` hoist was the whole
   0xc-vs-0x8 delta, and it also forced a `mov ebp,1` compare register).
4. **The dead-PARAMETER-home coalesce** - retail homes a local in a dead parameter's
   slot and cl does not (or vice versa). Seen on `CFecFile::AddFile`,
   `CMulti::LeaveState` + `CPlay::LeaveState`, `CWarpStoneFly::Init`,
   `FontRenderer::LayoutWrapped`, `CPlay::SaveUnderAndDrawCursor`'s DDSCAPS. It is **NOT a
   compiler flag** (/Oa /Ow /Ox /Ob2 /Og /Gy /Oi- /Ot /G4 /G5 /Gf /GF /Op /Gd all leave
   the frame unchanged) and it is **NOT** scope tricks, decl order or CRect/CPoint
   spellings - **it is CONTROL FLOW**: a call plus an early `return` ahead of the
   local's definition suppresses it, and inverting the guard to wrap the body restores
   it. See [early-return-kills-the-param-home-coalesce.md](early-return-kills-the-param-home-coalesce.md)
   - both `LeaveState` copies went 92.75 -> **100.00 EXACT** on that one change.
5. **A promoted zero/one register** ([redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md))
   - `CBootyState::ShowSecretBonusMessage` (extra `push ebx`, `cmp eax,ebp` for every
   null test), `CGiantRockLogic::BuildRockBreakInGameText`.

A mismatch of 8+ bytes was twice a *reconstruction* gap rather than a local-count
bug (`CGruntzMgr::HandleCommand` is 16 B AND 264 instructions short) - check the
instruction delta before assuming the frame is the whole story.

**Blind scope tricks are not a lever.** Block-scoping locals merely to move
`sub esp,N` either does nothing or overshoots (`ParseWave` reached `sub esp,0x8`
against retail's `0xc` that way, and `CSBI_GruntMachine::SerializeFields` lost
4 points). Scope becomes evidence only after the stack map names the roles:
which real local is address-taken, which temporary occupies an incoming
parameter home, and which declaration boundary permits or prevents that reuse.
`CFecFile::AddFile` is the positive control; otherwise find the variable the dev
actually reused or the array whose size is wrong.

Slot *assignment* within the frame is a separate, weaker signal: it follows
first-USE order, not declaration order (swapping declarations in `ParseWave`
changed nothing).

related: masked-diff-hides-branch-target.md, compensating-error-signatures.md

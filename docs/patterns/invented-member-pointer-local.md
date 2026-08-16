# A `Mgr* m = m_member;` local retail never had: one extra callee-save push, every register rotated
tags: cpp:local cpp:member | asm:push asm:mov | topic:codegen-idiom topic:regalloc
symptoms: base pushes one MORE callee-saved register than the target; base loads `[this+N]` once
into that register and uses it everywhere; retail re-issues `mov reg,[this+N]` at every use;
block topology identical, instruction counts drift by 1-3 per block, 80-93% plateau
confidence: 9/10

A long method that touches one member object a dozen times reads much better with a local:

```cpp
void CMulti::PumpB() {
    CDDrawSurfaceMgr* mgr = m_world;          // <-- NOT in retail
    ...
    mgr->m_level->VisitVisible(mgr->m_drawTarget->m_backPair, mgr->m_childGroup);
```

cl 5.0 gives that local a callee-saved home (`push ebx` appears purely to free `edi` for it) and
every later `[this+N]` load disappears. Retail re-reads the member at each use, so the recompile is
2-3 instructions SHORTER per block, one push LONGER at entry, and every register after the
prologue is rotated. The diff reads like regalloc noise; it is not.

**Tell:** `gruntz walls diagnose <rva> --asm` shows one more callee-save push at entry, and the
first hunk of `gruntz walls diagnose --asm` shows a prologue `mov <callee-saved>,[esi+N]` in base that
the target does not have, followed by `[<callee-saved>+k]` where the target has `mov eax,[esi+N]` /
`[eax+k]`. Sieve the source for `^\s+T\*\s+\w+\s*=\s*m_\w+;` in the functions the sieve names.

**Fix:** delete the local, spell `m_member->` at every use. `CMulti::PumpB` 0xb6e90 **83.38 ->
92.36** on that edit alone; the `push ebx` and the whole register rotation went with it.

Not every such local is invented — retail keeps `bx`/`by`/array-base locals in registers all the
time. The push count decides: retail hoisting the member itself into a callee-saved register means
the local IS real.

## Corollary: a store to an address-taken local both survives DSE and blocks CSE

The same function's residue was a `RECT rc` filled for a `SetRect` call. Retail stores `rc.top`
TWICE with the same value and loads `g_gameReg->m_modeSize.cx` twice for two uses in one call:

```asm
mov edx,[eax+0x90]   ; cy
mov ecx,[eax+0x8c]   ; cx  (#1)
mov [esp+0x10],edx   ; rc.top = cy      <-- store 1
mov edx,[eax+0x8c]   ; cx  (#2, NOT CSEd with #1)
mov eax,[eax+0x90]   ; cy  (#2)
...
mov [esp+0x18],eax   ; rc.top = cy      <-- store 2, same slot
```

`&rc` escapes into `SetRect`, so cl can neither delete the dead first store nor prove the global's
field unchanged across it. A duplicated store to the same escaped slot is therefore REAL SOURCE,
not a scheduling artifact — transcribe it. Both stores plus two named `left`/`right` locals took
PumpB 92.36 -> **98.32**; moving the second store between the two `cx` reads instead made it worse
(97.66), so place it where the byte order says, then stop.

related: [redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md)
(the same "one spurious local rotates everything" failure, for an `= 0` local),
[shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
(the other reason the entry push count differs)

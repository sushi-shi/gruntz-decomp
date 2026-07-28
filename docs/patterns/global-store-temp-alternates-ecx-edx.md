# `g_ptr->member = v` address temps alternate ecx/edx - it is a PHASE, not a coin flip

**Tags:** `cpp:global` `cpp:assign` | `asm:mov` | `topic:wall` `topic:regalloc`
**Confidence:** 8/10

## Symptom

One instruction pair differs and it is only the scratch register:

```
base:    mov edx,[g_gameReg]  ...  mov [edx+0x100],eax
target:  mov ecx,[g_gameReg]  ...  mov [ecx+0x100],eax
```

Everything else in the function - including OTHER reloads of the same global, some
of which use edx in retail too - is byte-identical. It looks like an unsteerable
regalloc coin flip. It is deterministic, but the lever is not local.

## Cause

For a store through a pointer loaded from a global (`g->m_x = <value already in eax>`),
cl5 takes the address temp from a 2-register pool and **alternates**: the 1st such
store in the function gets ecx, the 2nd edx, the 3rd ecx, ... Thiscall receivers
(`mov ecx,[g]; call ?Set...`) are forced to ecx and do NOT advance the alternation.

So the register on any one store is a function of HOW MANY such stores precede it -
a phase. Proven in a 40-line micro-replica (`cl /O2 /MT /GX`) that reproduces the
production register sequence exactly:

| stores present | 0x118 | 0x100 | 0x124 |
|---|---|---|---|
| as reconstructed         | ecx | **edx** | ecx |
| delete the 0x118 store   |  -  | **ecx** | ecx |
| insert a 3rd store first | ecx / edx | **ecx** | edx |

## What this means when you hit it

If retail's phase differs from yours, retail's source has one MORE (or one fewer)
pool temp before the differing store, in a form that is otherwise byte-identical -
i.e. you are missing a construct, not a register hint. There is no local spelling
that flips one store: temps for the RHS, a named local for the pointer, flattening
the enclosing `if` nest, and dropping later blocks all leave the phase alone (a named
local is actively worse - it re-colors several sites and adds a callee-save push).

Do not grind it as a coin flip: either find the missing pool temp upstream, or park
it. `ReadMenuOptionsDialog` (0x36a30) and `CPlay::ApplyGameOptions` (0x36be0) both sit
on the same phase break.

## Evidence

`src/Gruntz/VideoConfig.cpp` @0x036a30 (99.90%) and @0x036be0. Micro-replica A/B
under the production flags; the replica emits our exact `ecx / edx / edx / ecx`
sequence for the 0x118 store, the 0x100 store, the `m_sound` deref and the 0x124
store, so the reconstruction and the replica agree and retail is the outlier.

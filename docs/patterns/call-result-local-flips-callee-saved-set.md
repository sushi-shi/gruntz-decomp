# Binding a CALL RESULT to a named local flips the whole callee-saved register SET

**Tags:** `cpp:local` `cpp:call` `cpp:member` | `asm:push` `asm:pop` `asm:mov` | `topic:codegen-idiom` `topic:regalloc`
**Confidence:** 9/10

## Symptom

A function is byte-identical to retail *after canonicalising two register names*, and
the diff is dominated by the **prologue push set** — not just which value sits in which
register, but **which callee-saved registers the function saves at all**:

```
base:    push ebx | push esi | mov ebx,ecx        ; this -> ebx, node -> esi
target:  push esi | push edi | mov edi,ecx        ; this -> edi, node -> esi
```

Everything after that is the same instruction stream with `ebx`↔`edi` (or
`edi`↔`ebx`) exchanged, including the epilogue `pop` order. It reads as an
unsteerable regalloc tie-break, and it was filed as one here for months.

## Cause

The last statement was

```cpp
node->m_listPosition = list.AddTail(node);      // call result stored straight into a member
```

cl5 treats the call's return value as a **dead-on-arrival temp of the store**: it has no
live range of its own, so it never enters the colouring and the whole allocation is
decided by the remaining values. Giving the result a **named local** creates a real live
range, the colouring runs with one more candidate, and cl lands on retail's register set:

```cpp
POSITION pos = list.AddTail(node);
node->m_listPosition = pos;
```

The same lever works on a TAIL CALL — `return F(...)` vs `T r = F(...); return r;` —
where the return value is likewise a temp of the `ret`.

**This is the callee-saved SET, not a scratch register.** It is the strong form of
[`one-use-local-is-a-regalloc-knob`](one-use-local-is-a-regalloc-knob.md) (which covers
scratch/eax-vs-edx choices); here the push/pop list itself changes.

## Fix

Split `<lvalue> = <call>(...)` into `T v = <call>(...); <lvalue> = v;` — and split
`return <call>(...)` into `T r = <call>(...); return r;`. Then MEASURE: the lever runs
both ways and the wrong direction costs as much as the right one gains.

## Evidence (2026-07-28, `src/Image/ImagePool.cpp`, unit `imagepool`)

**Nine call sites, one line each, EIGHT functions flipped to 100 EXACT in one build.**
The five surface factories and the three palette factories all ended
`node->m_listPosition = m_surfaces.AddTail(node);` / `= m_palettes.AddTail(node);`:

| function | before → after |
|---|---|
| `CImagePool::CreatePaletteFromEntries` @0x1754f0 | 99.02 → **100.00 EXACT** |
| `CImagePool::CreatePaletteFromRgb` @0x175570 | 99.02 → **100.00 EXACT** |
| `CImagePool::LoadPaletteFromData` @0x175680 | 99.11 → **100.00 EXACT** |
| `CImagePool::CreateSurface` @0x174fe0 | 96.07 → **100.00 EXACT** |
| `CImagePool::CreateSurfaceFromPixels` @0x1750e0 | 96.15 → **100.00 EXACT** |
| `CImagePool::LoadSurfaceFromData` @0x1751f0 | 95.98 → **100.00 EXACT** |
| `CImagePool::LoadSurfaceFromResource` @0x1752f0 | 95.98 → **100.00 EXACT** |
| `CImagePool::ConvertSurface` @0x1753f0 | 95.88 → **100.00 EXACT** |

The palette trio had `this` in ebx where retail has edi; the surface five had the node
in ebx and the zero constant in edi where retail has them the other way round. Both
were filed `@early-stop` as "a regalloc tie-break, not source-steerable", with a
documented failed search (a `CPtrList*` local for the list, a `CImagePool* self = this`
copy, a pre-initialised `node = 0`, and modelling the seed as a real ctor). None of
those touch the *result* of the call, which is the value that was missing a live range.

The tail-call form, same file: `CRezImage::DecodeResData` @0x175e00
(`return DecodeBlit(...)` → `i32 r = DecodeBlit(...); return r;`) **96.73 → 99.62**,
its `buf`/`bitcount` two-register role swap gone in one edit.

The sibling `CImagePool::LoadPaletteFromFile` @0x1755f0 was ALWAYS 100 with the un-split
spelling — its extra leading `g_hResModule = m_resourceModuleHandle;` already gives
`this` an early use, which pins the same set. That asymmetry inside one file is the
tell: **when one member of a copy-paste family matches and the rest do not, look for the
statement the matching one has extra, then ask which value it gives a live range to.**

## Cost

The added local perturbs cl's per-TU state: `CRezImage::FillRectAt` @0x176da0 (700 lines
further down, untouched) went 100 → 66.44 on the same build and no spelling of *its* body
recovers it (all 24 field-assignment orders measured; see
[`preceding-function-state-recolors-later-comdat`](preceding-function-state-recolors-later-comdat.md)).
Net +7 EXACT for the file. Take the trade.

## Related

- [`one-use-local-is-a-regalloc-knob.md`](one-use-local-is-a-regalloc-knob.md) — the
  scratch-register form of the same knob; try both directions.
- [`named-local-keeps-deref-base-in-own-register.md`](named-local-keeps-deref-base-in-own-register.md)
  — the same idea applied to a deref BASE rather than a call result.
- [`delete-expression-vs-operator-delete-scratch-reg.md`](delete-expression-vs-operator-delete-scratch-reg.md)

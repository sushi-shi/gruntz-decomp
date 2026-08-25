# A `volatile` receiver can force a store order, but does not prove volatile source

- confidence: 10/10
- tags: `cpp:member` `cpp:struct` `cpp:local` | `asm:mov` | `topic:negative-control` `topic:wall`

## Retraction

The original version called the access-site `volatile` spelling a recovered
source idiom because it made the two example functions exact. That conclusion
was too strong. `volatile` imposes an externally observable access-order
constraint for which there is no program evidence here; it is a compiler
steering device, not a reconstruction of the object. Commit `1fbdd8236` removed
both qualifiers and retained the lower, honest result. The experiment remains
useful as a negative control: it proves that the residual is only store
scheduling, not that the retail source was volatile.

## Symptom

A tiny leaf setter (10-15 instructions, correct instruction MULTISET, correct size)
plateaus in the low 70s because exactly one store floats two slots away from where
retail puts it:

```
retail                              recompile
  mov [ecx+0x334],eax   counter       mov [ecx+0x334],eax
  mov eax,[esp+0xc]     load arg3     mov eax,[esp+0xc]
  mov [ecx+0x330],edx   state         mov [ecx+0x340],eax    <- interval jumps the queue
  mov [ecx+0x340],eax   interval      xor eax,eax
  xor eax,eax                         mov [ecx+0x330],edx    <- state sinks
  mov [ecx+0x344],eax                 mov [ecx+0x344],eax
```

Retail's order is exactly the order the four assignments are written in. Yours is not,
and **no source spelling changes it**.

## How to be sure before reaching for this

This idiom is only the answer once the local space is genuinely exhausted, because it is
indistinguishable from a scheduling wall until then. The measurements that establish it:

- **All statement orders collapse.** 96 cells (all 24 permutations x i64-vs-lo/hi spelling
  of both 64-bit members) produced exactly TWO distinct outputs. cl canonicalizes the
  store order of independent same-object members, so source order is not a lever - which
  also means retail's order cannot have come from a different source order.
- **All receiver shapes collapse.** Direct member, `T& r = m_x`, `T* r = &m_x`, and a
  `static inline` helper taking the row by reference or pointer are byte-identical (a
  plain `static` helper is NOT - cl5 /Ob1 leaves it a real call and the function drops to
  0.00).
- **All union-member selections collapse.** 192 cells crossing `m_counter`/`m_value`,
  `m_interval`/`m_intervalLo+Hi`, `m_last`/`m_lastLo+Hi` and the orders: same two outputs.
- **It is not TU state.** Perturbing the PRECEDING function (statement swap, extra local,
  an inserted `static` function) moves the score by 0.00, so `--state-trials` is the wrong
  lever by its own classification rule.

## Rejected workaround

Binding the receiver through a `volatile` reference forces cl to emit the stores
in source order and happens to reproduce retail's bytes:

```cpp
volatile CSbiHlRow& r = m_leftMachine;
r.m_counter = y0;
r.m_state = IDX(x0);
r.m_interval = static_cast<u32>(z);
r.m_last = g_frameTime;
```

`volatile T&`, `volatile T*` and the volatile-qualified inline helper all give the
same byte-exact result. None establishes that the object was actually volatile.
Do not retain any of them without independent evidence of volatile storage or
observable concurrent/device access.

## Why a `volatile` member is not the answer either

`volatile CSbiHlRow m_leftMachine;` is not legal C++ when the type has a user-declared
constructor - constructors cannot be cv-qualified, so the containing class could never
construct it. `CSbiHlRow::CSbiHlRow` is a real retail function (0x000c86d0), so the
declaration must be plain. With no independent access-site evidence, neither volatile
model is justified.

## Measurement

`CStatusBarMgr::SetLeftRezMachineAnimation` 0x1066f0 and `SetRightRezMachineAnimation` 0x106740 are byte-identical
twins. Both remain at 71.83 through roughly 400 ordinary source-shape cells and
become **100.00 EXACT** with the access-site qualifier. Their plain bodies have
the same stores, offsets, values, size, and straight-line control flow as retail;
the residue is one scheduling swap. The exact volatile result is therefore a
compiler control, while the retained non-volatile source is the evidence-backed
model.

## Related

- [In the 40-65% band, check the PROLOGUE's frame size before anything else](frame-size-mismatch-dominates-the-40-65-band.md) - the other "one modelling error reads as N points" family.
- [Two if/else arms retail emits in FULL: give each arm its own local](identical-arms-need-distinct-locals.md) - the other place where twins carry the signal.

# A store run retail emits in SOURCE order, that cl reorders no matter how you spell it, is a `volatile` receiver

- confidence: 10/10
- tags: `cpp:member` `cpp:struct` `cpp:local` | `asm:mov` | `topic:codegen-idiom` `topic:wall-broken`

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

## Fix

Bind the receiver through a `volatile` reference. Volatile stores cannot be reordered
with respect to each other, so cl emits them in source order; non-volatile loads are
still free to move, which is what lets retail hoist the argument load into the middle of
the run:

```cpp
volatile CSbiHlRow& r = m_machineA;
r.m_counter = y0;
r.m_state = IDX(x0);
r.m_interval = static_cast<u32>(z);
r.m_last = g_frameTime;
```

`volatile T&`, `volatile T*` and the volatile-qualified inline helper all give the SAME
byte-exact result, so pick the one that reads best. A volatile qualifier on just the one
store that moved does NOT work - the whole receiver has to be volatile, which is the
honest reading: the object is volatile, not the assignment.

## Why it cannot be a `volatile` MEMBER

`volatile CSbiHlRow m_machineA;` is not legal C++ when the type has a user-declared
constructor - constructors cannot be cv-qualified, so the containing class could never
construct it. `CSbiHlRow::CSbiHlRow` is a real retail function (0x000c86d0), so the
declaration must be plain and the volatility applied at the access site.

## Evidence

`CStatusBarMgr::SetHudRectA` 0x1066f0 and `SetHudRectB` 0x106740 - byte-identical twins,
both stuck at 71.83 through ~400 measured cells - go to **100.00 EXACT** together, size
59 each, the moment the receiver is volatile. Tree exact 3304 -> 3309 (the twins plus
three ripple gains in the same TU). Twins are the tell: two independent instances of the
same deterministic mis-order is a source-shape fact, never scheduler noise.

## Related

- [In the 40-65% band, check the PROLOGUE's frame size before anything else](frame-size-mismatch-dominates-the-40-65-band.md) - the other "one modelling error reads as N points" family.
- [Two if/else arms retail emits in FULL: give each arm its own local](identical-arms-need-distinct-locals.md) - the other place where twins carry the signal.

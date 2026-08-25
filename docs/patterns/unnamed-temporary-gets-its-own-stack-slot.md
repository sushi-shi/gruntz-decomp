# A class temporary gets its OWN stack slot only when it is UNNAMED

**Tags:** `cpp:local` `cpp:call` | `asm:lea` `asm:mov` | `topic:codegen-idiom` `topic:wall`
**Confidence:** 9/10

## Symptom

Retail's frame is bigger than yours by an exact multiple of 4, and retail's stack map
shows one slot per call site for a by-value class return (typically `CString`), where
your recompile shares ONE slot across all of them. Every `[esp+N]` operand then shifts
and the function cannot align at all — a hard 0% is normal.

The giveaway is at the call site, not in the frame:

```
target:  call ?FindAnimationKey@...     ; returns CString by value
         mov  edi,[eax]           ; m_pchData read straight off the return buffer
base:    call ?FindAnimationKey@...
         mov  edi,[esp+0x10]      ; stored, then reloaded from the temp's slot
```

## Cause

cl 5.0 allocates a distinct temporary slot **per call site** for an unnamed class
return value. Bind that return to a **named local** and it becomes an ordinary local
whose live range ends at its last use — and cl then colours all of them onto one slot,
because the lifetimes are disjoint (`FindAnimationKey` → inline `strcpy` → `~CString`, no
throwing call spanning them).

So the coalescing is not a compiler mood: it is what a named local *asks for*.

## Fix

Consume the temporary in place.

```cpp
// before - one shared slot, and `mov edi,[esp+N]`
CString animationName = registry->FindAnimationKey(m_animMoving);
strcpy(nameBuffer, static_cast<const char*>(animationName));

// after - its own slot, and `mov edi,[eax]`
strcpy(nameBuffer, static_cast<const char*>(registry->FindAnimationKey(m_animMoving)));
```

## What this does NOT mean

It is not "always inline the temporary". The choice is a per-function measurement:

- `CWarlord::SerializeDispatch` (0x43670) has **eleven** such call sites and retail gives
  each its own slot — unnamed is right, and it is the whole reason the function can
  align at all.
- `CInGameIcon::SerializeDispatch` (0x98c90) has **two**, and retail SHARES one slot —
  named locals are right there. Inlining both cost it a 4-byte frame growth and 2.4%.

Read the frame, count retail's slots, then pick. `mov edi,[eax]` vs `mov edi,[esp+N]`
tells you per site.

## Evidence

2026-07-29. `CWarlord::SerializeDispatch` (0x43670, 3104 B) was stubbed at 0.19% behind an
explicit dead-end note: *"our MSVC5 /O2 /GX COALESCES those eleven destructible
temporaries into one slot ... No source spelling defeats the coalescing (tried: unnamed
temporary, eleven distinct named locals, function-scope buffers)"*, and a complete body
had been written, built, and reverted for scoring below the stub. Written with unnamed
temporaries it built at **91.8% first try**, and the frame matched 0x130 exactly:
2 dwords + 0x80 body buffer + ten temp slots + 0x80 chain header.

The rule had been measured independently an hour earlier on `CInGameIcon::SerializeDispatch`
in the opposite direction — two unnamed temporaries there grew the frame by 4 over two
named locals, which is the same rule seen from the other side.

Four further spellings took the same function 91.8 → 99.52, all worth knowing:
shared `goto fail` exits (+0.7), hoisting a member pointer cl5 reloads because it cannot
disprove store aliasing (+0.8), the branch sense that puts the hot arm on the fallthrough
(+6.2), and taking a map's address into a local so the out-slot's zero store lands after
the argument pushes (+0.02).

Related: [[dup-exit-means-a-shared-goto-label]], [[frame-dword-count-merges-duplicate-temps]].

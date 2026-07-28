# `[base+index]` SIB operand roles follow LOCAL DECLARATION ORDER

**Tags:** `cpp:local` `cpp:loop` | `asm:mov` | `topic:codegen-idiom`
**Confidence:** 8/10

## Symptom

A byte/word loop over `p[i]` differs from retail only in which register is the SIB
*base* and which is the *index* - same address, different encoding, one differing
byte per site:

```
base:    mov al,byte ptr [esi+edi]      ; base = esi (the INDEX variable)
target:  mov al,byte ptr [edi+esi]      ; base = edi (the POINTER)
```

Both operands are already in the same registers as retail; only the roles are swapped.

## Cause

MSVC5 builds the address expression from the two values in the order they became
live, which for two function-scope locals is their DECLARATION order. Declaring the
pointer first makes the pointer the index; declaring the counter first makes the
pointer the base (retail's shape here).

## Fix

Swap the two declarations.

```cpp
// before - cl emits [pos + names]
const char* names = blockBase + pd->imageSetsOffset;
i32 pos = 0;

// after - cl emits [names + pos], matching retail
i32 pos = 0;
const char* names = blockBase + pd->imageSetsOffset;
```

## What does NOT work (measured, so you can stop early)

The declaration order is the ONLY lever. On `CVariantSlot::Find` (0x16e1d0) and the
three `CHashBase` bucket helpers these all produced byte-identical output:

- reversing the source operand order (`lo + hi` for `hi + lo`, `idx + arr` for
  `arr + idx`, even the legal `idx[arr]` spelling) - cl canonicalizes the addition;
- binding the address to a named local (`CHashSlot* b = &m_buckets[idx]; b->…`);
- re-reading the member instead of the local index;
- routing the subscript through an inline accessor (`Bucket(idx)->…`), so the index
  becomes an inline formal materialised at the call.

**Corollary: with only ONE local in play there is no lever.** `MonoClear` (0x184db0)
indexes a GLOBAL buffer by its single loop counter; giving the buffer a local turns the
loop into `rep stos` (a much bigger change), so its one SIB byte is parked.

## Evidence

- `CDDrawWorkerHost::Read` (0x161640): three `mov al,[edi+esi]` sites in the image-set
  name tokenizer, all fixed by the swap. (In the same function the index also had to be
  `i32`, not `u32` - a `u32` loop cursor kept a different form.)
- `CVariantSlot::Find` (0x16e1d0), 2026-07-28: the binary search's `lea eax,[esi+edi]`
  (`(hi + lo) / 2`). Declaring `lo` before `hi` - **not** writing `lo + hi` - flipped the
  SIB byte; 99.69 -> **100 EXACT**.
- Still open in the same family, both with the pointer coming from a MEMBER rather than a
  local: `CHashBase::Insert` 0x184a70 / `Remove` 0x184ab0 / `Last` 0x184b10 (one SIB byte
  each; note Insert/Remove and Last want OPPOSITE roles, so it is not a global convention).

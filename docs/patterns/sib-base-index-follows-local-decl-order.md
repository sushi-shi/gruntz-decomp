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
- Still open in the same family, all with the pointer coming from a MEMBER rather than a
  local: `CHashBase::Insert` 0x184a70 / `Remove` 0x184ab0 / `Lookup` 0x184b40 and
  `CHashElement::Prev` 0x184900 (one SIB byte each; note Insert/Remove and Last want
  OPPOSITE roles, so it is not a global convention). `Last` 0x184b10 has since gone EXACT.
- 2026-08-05 re-audit of the three open ones adds a FOURTH and FIFTH non-lever. `MonoClear`
  (0x184db0): `i + g_monoBuffer` (cl canonicalizes the addition), `&g_monoBuffer[i]`
  (subscript+address-of builds the same tree), and a `(u8*)` cast on the base all emit the
  identical SIB byte; giving the buffer a local declared AFTER the counter - the exact form
  the rule above prescribes - is still the `rep stosd` collapse, so the corollary holds.
  `MonoNewline` (0x184d50) carries the SAME inversion at all three of its sites, so it is
  one defect, not four. And the TU-STATE parity probe that flips a canonical `imul`
  ([`commutative-operand-order-is-canonical.md`](commutative-operand-order-is-canonical.md))
  leaves all four SIB sites untouched - the SIB role is NOT that mechanism.
- `CHashElement::Prev` (0x184900), 2026-07-29, adds a THIRD non-lever to the list above:
  the address there is strength-reduced into a loop-preheader `lea` rather than re-formed
  per iteration, and neither declaration order flips it (counter-first and pointer-first
  give the identical byte). Routing the subscript through the member (`coll->m_buckets[i]`
  with no `b` local) DOES flip the roles - but simultaneously recolours `m_buckets` from
  eax into ecx, so it trades one wrong byte for four. The pointer-from-a-member sub-family
  still has no lever.

## Where the lever does NOT reach

Both operands must be function-scope LOCALS. When one of them is a member load the
declaration order has nothing to move, and the roles are fixed by the order the two
values become live. `CRezImage::DecodeBmpData` 0x175e00 is the control: its single
differing byte is `lea esi,[esi+edx+0x400]` against retail's `lea esi,[edx+esi+0x400]`,
where the index is `ih->biSize` read straight from the header. Seven A/Bs are
byte-identical at 99.62 - three operand orders in the address expression, a named
local for the size, a parenthesised regrouping, and three declaration-order
permutations of the function's own locals.

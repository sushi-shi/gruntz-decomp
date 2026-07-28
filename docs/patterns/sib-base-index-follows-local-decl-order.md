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

## Evidence

`CDDrawWorkerHost::Read` (0x161640): three `mov al,[edi+esi]` sites in the image-set
name tokenizer, all fixed by the swap. (In the same function the index also had to be
`i32`, not `u32` - a `u32` loop cursor kept a different form.)

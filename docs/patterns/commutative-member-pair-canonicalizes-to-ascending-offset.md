# A literal and a structurally-spelled constant are DIFFERENT TREES - `offsetof` flipped an operand order a literal could not

**Tags:** `cpp:expr` `cpp:member` `cpp:struct` | `asm:mov` `asm:add` `asm:imul` | `topic:codegen-idiom` `topic:correctness`
**Confidence:** 10/10 (one EXACT flip, four refuted spellings before it, siblings held)

## Symptom

A tiny accessor sits one byte short of exact, `walls diagnose` says
REGALLOC/SCHEDULING, and the divergence is the displacement byte of instruction
0. Same instruction count, same opcodes, same operands - only the two member
loads in the opposite ORDER:

```asm
; ours                                  ; retail
mov  eax,[ecx+0x4]   ; m_width          mov  eax,[ecx+0x8]   ; m_height
imul eax,[ecx+0x8]   ; m_height         imul eax,[ecx+0x4]   ; m_width
add  eax,0x10                           add  eax,0x10
ret                                     ret
```

## What does NOT reach it

Source operand order is inert: `m_height * m_width` and `m_width * m_height`
emit the identical object, because cl canonicalizes a commutative op over two
members of one object to ascending member offset. Refuted, one build each, all
byte-identical:

| lever | result |
|---|---|
| swap the multiply's operands | byte-identical |
| an inline `GetArea()` member used at both area sites | byte-identical (and -11 elsewhere through the shared header) |
| move the definition into the header `inline`, like its two siblings | byte-identical |
| a named local for the first operand | byte-identical |

The layout was confirmed CORRECT before any of that, which is what made the
refutations meaningful: retail's own exact `CImageSet1::ScanRight`
(`m_width - 1`) reads `[ecx+0x4]` and `ScanDown` (`m_height - 1`) reads
`[ecx+0x8]`.

## What DOES reach it: stop writing the constant as a literal

`CImageSet3::GetStride` 0x00161590 **99.50 -> 100.00 EXACT** on this change
alone:

```cpp
-    return m_height * m_width + 0x10;
+    return m_height * m_width + offsetof(WwdTileImageRecord, m_fields);
```

Nothing else moved: the operand order was never the free variable - the
CONSTANT was. A literal is a leaf; `offsetof`/`sizeof` is a constant-folded
sub-expression, and C1 hands C2 a different tree for it, which re-orders the
sibling operands the canonicalizer had otherwise fixed.

## Why the constant was spellable at all

The three `GetStride` overrides return their own record's SERIALIZED SIZE, and
each `Parse` proves the field count - so none of the three literals was magic:

| class | `Parse` reads | size |
|---|---|---|
| `CImageSet1` | width, height, collisionValue | 8 + 3*4 = `0x14` = `sizeof(WwdTileImageRecord)` |
| `CImageSet2` | width, height + six more | 8 + 8*4 = `0x28` = `offsetof(m_fields) + 6*sizeof(i32)` |
| `CImageSet3` | width, height, then a w*h payload | 8 + 2*4 = `0x10` = `offsetof(m_fields)`, plus `w*h` |

Both sibling `GetStride`s stayed byte-exact through the rewrite, which is what
proves the sizes are right.

## The rule, and the reverse use

**A magic constant is a modelling defect AND a matching lever.** Where a literal
equals a `sizeof`, an `offsetof`, or an enum value, spelling it structurally is
not cosmetic: it changes the expression tree, and it can move code the operand
order itself cannot. So when a tiny function is one reordered pair short and
every spelling of the operands is inert, **look at the constant beside them**
before parking the row.

Caveat that cost a build: keep retail's pointer WALK in the readers. Reading
`rec->m_width` / `rec->m_height` as named members took `CImageSet3::Parse`
100.00 -> 77.47 and both siblings with it; anchoring the same walk at
`&rec->m_width` is byte-neutral.

## Still open

`CFaderSine::GetFrameCount` 0x00180400 has the identical two-member shape with
NO constant beside it (`m_scaledMag + m_frameCount`, retail loading
`m_frameCount` first), so this lever has nothing to bite on there. It stays the
open case for the same canonicalizer.

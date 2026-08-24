# The same instruction reading a different displacement is a WRONG MEMBER
tags: cpp:member cpp:inheritance | asm:mov | topic:correctness topic:sieve
symptoms: `walls diagnose` says REGALLOC/SCHEDULING and "bytes first differ at +0xNNN"; the two sides' instruction, call and branch counts agree; at the divergence both sides emit the same mnemonic with the same operand shape off the SAME base register and only the ModRM displacement differs
confidence: 9/10

## Symptom

A function sits in the high 70s to high 90s with every structural reading clean:
same call multiset, same branch and `ret` skeleton, same relocation set.
`gruntz walls diagnose` classifies it REGALLOC/SCHEDULING, which is the verdict
that makes a reader stop. But at the byte the classes name, the two sides hold

```
ours    mov ecx,DWORD PTR [ebx+0x68]
retail  mov edx,DWORD PTR [ebx+0x64]
```

Same instruction, same base pointer, four bits of displacement apart. That is
not a schedule coin and not a register rotation: it is a different FIELD, and
the shipped game reads the wrong dword.

## Cause

Two source shapes produce it, and they are not equally harmless.

**A base-subobject twin.** A class deriving from two bases that each hold the
same pointer has two members with the same value at two offsets. `CGruntPuddle`
is `CUserLogic, CWapX`; `CUserLogic::m_object` is at +0x10 and
`CWapX::m_wwdObject` at +0x38, and both are set from the same `CGameObject*` in
the constructor. Naming the wrong one costs four displacements and misbehaves in
no way at all - which is exactly why nothing else finds it.

**A genuinely wrong field.** `CTriggerMgr::LoadTileArrivalFx` read the goober
puddle's gauge award from +0x68 where retail reads +0x64. The two offsets are
`Place`'s first and fourth arguments, so the pickup credited the puddle's type id
instead of its points. Byte-exact siblings settle which is which:
`CGruntPuddle::Place` (100.00%) homes arg4 at +0x64 and arg1 at +0x68, and
`CTriggerMgr::PlacePuddle` (100.00%) passes `sprite->m_points` as arg4 with a 25
default - the same 25 the reader primes its local with.

## The instrument

`gruntz walls offsetscan`. It aligns the two instruction streams on a key that
masks everything cl is free to re-choose - registers, immediates, displacements -
and then reads the displacements back at the positions the alignment called
EQUAL. A census cannot do this: 0x10 and 0x68 are among the commonest offsets in
the image, so a real four-site swap sits inside dozens of unrelated occurrences
and any threshold that admits it admits noise.

Four suppressions, each a measured false-positive class, take the sweep from 119
rows to 7:

| suppression | what it removes |
|---|---|
| the line carries a RELOCATION | `[eax+<index table>]`, a global - a link-time address objdiff masks |
| the only register is SCALED | `[ecx*4+0x127c]`, an array index off an absolute base |
| the base registers DIFFER | two accesses through different pointers that the masked key happened to align |
| both sides touch BOTH fields in the aligned run | the accesses were merely SCHEDULED in the other order (storescan's channel); one 99.56% row produced 46 such pairs forming closed permutation cycles |
| the mismatch is within 2 of the run's EDGE | one side has an extra instruction and the pairing walks off by one; three live rows read that way and every one was an insertion |

A run of literally identical stores - a constructor clearing fields - carries no
information about which store pairs with which, so the alignment zips two lists.
Those rows are reported separately as a field SET question.

## Verification

Backwards, end to end: the defect was RE-INTRODUCED into
`src/Gruntz/TerrainTileLoader.cpp`, the tree rebuilt, and the sweep named both
displacements in one line each -

```
ours   mov ecx,DWORD PTR [ebx+0x68]      ours   mov ebx,DWORD PTR [ebx+0x10]
retail mov edx,DWORD PTR [ebx+0x64]      retail mov ebx,DWORD PTR [ebx+0x38]
```

- before the fix was restored and the row went silent. That row is now the
sieve's live NEGATIVE control; the POSITIVE is those same bytes as a fixture,
because the only known live positive is the one the sieve closed.

## What the live sweep is worth

Measured 2026-08-23 by hand-reading five of the seven live `field` rows: ONE was
a genuine finding and three were alignment SLIPS that survived the edge rule
because the extra instruction sat further than two positions away.  The
remaining one is the known aligning-delta artifact.  So the detector is proven
and the channel is real, but the live rows are LEADS with a majority
false-positive rate, and low coverage predicts a slip without excluding one:
`CProjectile::LoadProjectileSprites` reads as a 4-byte layout shift at 93.7%
coverage and both sides in fact use 0x1e0/0x1e4/0x1e8/0x1ec identically.

The genuine one is worth stating because it is not a wrong member at all but a
wrong ADDRESSING SHAPE, which this reading also catches.
`CStatusBarMgr::LoadRezMachineConfig` named the row array through a pointer to
the ARRAY (`CSbiHlRow* g = m_groupSlots; g[col].m_state`), which folds the
member offset into the `lea`:

```
ours    lea edi,[esi+edx*8+0x2c0]   mov [edi],0x4        mov [edi+0x10],eax
retail  lea edi,[esi+ecx*8]         mov [edi+0x2c0],0x4  mov [edi+0x2d0],eax
```

Writing `m_groupSlots[col].m_state` reproduces retail's form byte for byte and
silences the row - and costs 99.15 -> 91.50, because the shorter body flips one
`AcquireAndPlay` and one `GetDwordDef` site's tail-merge.  Not committed for that
reason; recorded here so the next reader does not re-derive it.

## Reading a hit

Every hit is a LEAD, not a verdict. The disassembly says which member the
displacement belongs to, and two adjacent scalars of one class look here exactly
like one class's member against a base subobject's copy of the same pointer.
Settle it against a byte-exact sibling that homes the field - a constructor, a
setter, the caller that supplies the argument - the way `Place` and
`PlacePuddle` settled the puddle.

A negative displacement (`-0x14` against `-0x4`) on a body already proven exact
is the aligning-delta artifact, not a member: the two sides fold the frame base
a constant apart. `CNetSession::Verify` reports four that way at 100.00%.

## Related

- [`emitted-store-order-is-not-the-source-order`](emitted-store-order-is-not-the-source-order.md)
  — the channel the order suppression hands the row to.
- [`a-dead-stack-store-is-not-an-era-anomaly`](a-dead-stack-store-is-not-an-era-anomaly.md)
  — the other reading of a byte-level residue on an otherwise clean row.

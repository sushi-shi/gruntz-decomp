# A commutative operand order is CANONICAL — the source cannot spell it, but TU state flips it

tags: cpp:member cpp:local cpp:expr | asm:add asm:imul asm:mov | topic:wall topic:codegen-idiom
symptoms: a >=99% function whose whole residue is two member loads feeding one commutative
operator emitted in the opposite order (`mov r,[p+A]; imul r,[p+B]` vs `[p+B]` then `[p+A]`;
or the 1st and 4th term of an n-term `+` chain swapped) — and NO source spelling moves it
confidence: 9/10

cl5 CANONICALISES the operand order of a commutative integer operator over member loads.
The order it picks is a property of the operands, not of the source, so every spelling of
the expression emits the identical bytes. Measured on `CDDrawWorker::GetMemoryUsage`
@0x001523f0 (`m_height * m_width`, offsets 0x14 and 0x10) — all five of these give
`mov esi,[edx+0x10]; imul esi,[edx+0x14]`:

- `h * w` and `w * h`;
- both operands hoisted into named locals, in either declaration order;
- statement-split (`i32 size = h; size *= w;` / `size = size * w;`), either operand first.

Same on `CDDrawChildGroup::SumWeighted` @0x0015aaf0, a four-term `a+b+c+d` over one object:
source order, all three parenthesizations, per-term statement splits, and even the
distributed form `i*a + i*b + i*c + i*d` (which cl re-factors) all emit the identical
term order — the 1st and 4th term swapped relative to retail.

## What DOES flip it: a preceding definition in the same TU

The canonical order is TU-STATE PARITY. Adding **any preceding definition** to the TU
flips the `imul` pair and makes the function byte-identical to retail. Measured on
`WwdGameObject.cpp` / `GetMemoryUsage`:

| perturbation | flips? |
|---|---|
| `static i32 f(i32 a){return a+1;}` before the function | YES (-> identical) |
| `static i32 g = 1;` (file-scope datum) before the function | YES (-> identical) |
| the same function AFTER it (end of file) | no |
| an extra `#include <stdlib.h>` at the top | no |
| anything at all inside the function body | no |

The probe's own operand order is irrelevant — only its EXISTENCE matters. So a stuck
commutative-operand residue is evidence that **the TU is missing a definition that
retail emitted before this function** (a body still homed elsewhere, a file-scope datum,
or a COMDAT-emitting inline/static). It is not a property of the victim function, and
`permute`/`match_variants` cannot reach it: they mutate bodies.

Confirmed the same way on `CDDrawWorkerHost::Save` @0x00163780 (an `imul` of two adjacent
members, `[esi+0x2c] * [esi+0x28]`): the probe takes its diff to nothing.

**The parity does NOT extend to every commutative residue.** The probe left all of these
unchanged, so they are a different mechanism: the four-term `+` chain in `SumWeighted`,
the SIB base/index roles in `MonoClear`/`CHashBase::Insert`/`CHashElement::Prev`
(see [`sib-base-index-follows-local-decl-order.md`](sib-base-index-follows-local-decl-order.md)),
the vptr-stamp transposition in the `CWayPoint`/`CGuardPoint` ctor family, and the
scratch-register rotation in `SaveVideoCheckboxes`.

## The parity is NARROW — measured

The probe was run against 28 sub-100 functions across 22 TUs. It flipped exactly two
(`GetMemoryUsage`, `Save`) and was **codegen-neutral on the other 26** — not one diff line
moved, including four `CGameLevel` probe helpers, five `CLightFxRender` palette builders,
`CMulti::Render`, `SoundStream::ParseWave` and `CSBI_GruntMachine::SerializeFields`. So
"adding a definition to the TU perturbs everything" is FALSE here; it perturbs this one
canonicalisation. That also means the probe is a safe, cheap diagnostic.

The 2-term member add is canonical too, and its canonical order is not a function of the
operand pair alone: `ProbeColumn` and `ProbeHeadSoft` add the SAME two members
(`m_screenY` + `m_extent.top`) and cl picks OPPOSITE orders for them, because
`ProbeHeadSoft` has a third term. Swapping the source operands in all four `CGameLevel`
probes was byte-identical.

## The flip is NOT binary existence - the definition's KIND matters (measured 2026-08-10)

Probing the `LevelPlane.cpp` slot between `ActivateVisibleObjects` and
`DeactivateDistantObjects` against THREE victims at once (Deactivate's add pairs, `Save`'s
imul, `Load`'s imul; baseline D✗ S✓ L✓):

| probe between the twins | D | S | L |
|---|---|---|---|
| none | ✗ | ✓ | ✓ |
| `struct X;` (fwd-decl only) | ✗ | ✓ | ✗ |
| `typedef i32 X;` | ✗ | ✓ | ✗ |
| `static i32 f(i32 a){return a+1;}` | ✗ | ✗ | ✗ |
| `struct X {};` | ✓ | ✓ | ✓ |
| `struct X { i32 a; };` | ✓ | ✓ | ✓ |

Only a CLASS-TYPE DEFINITION (member count irrelevant) lands all three simultaneously; a
mere declaration, a typedef and a static function each advance the state differently, and
each victim responds on its own phase. So when a TU has SEVERAL parity victims, the probe
matrix constrains the KIND of the missing definition, not just its existence - here the
evidence says retail defined a struct/class between the twins (zero bytes emitted, content
unrecoverable; the .text is gapless, functions.tsv confirms 0x163300+0x70 abuts 0x163370).

1. If the residue is an `imul`/`add` pair of member loads on ONE object, stop editing the
   body — it is canonical. Run the probe (a throwaway `static` definition placed before
   the function) to confirm the parity, then delete it.
2. A confirmed parity flip is a TU-COMPLETENESS finding, not a matching finding: report
   the TU as missing a preceding definition and let the partition work supply the real one.
3. Never ship the probe.

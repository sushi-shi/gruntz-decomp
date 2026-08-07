# A `{}` default constructor is wrong when the class owns 64-bit clock members: retail seeds every one

- **confidence** c10
- **tags** `cpp:ctor` `cpp:class` `cpp:inline` | `asm:mov` | `topic:codegen-idiom`

## Symptom

A factory/`new`-site that inlines `CFoo::CFoo()` is short by a multiple of FOUR
instructions against retail, and the missing rows are all the same shape:

```
mov  DWORD PTR [esi+0x88],edi      # edi == 0
mov  DWORD PTR [esi+0x90],edi
mov  DWORD PTR [esi+0x8c],edi
mov  DWORD PTR [esi+0x94],edi
```

Retail also compares the freshly `new`ed pointer with `cmp esi,edi` where we emit
`test esi,esi` — that zero REGISTER only exists because the ctor expansions below
need a 0, so the comparison form is a *consequence* of the missing stores, not a
separate regalloc choice.

## Cause

Every Gruntz logic class whose members include 64-bit clock/window pairs
(`i64`, `Clock64`, `CHazardTimer`, a `CPairRecord` band) has a default constructor
that seeds each of them to 0. Our headers spell those ctors `CFoo() {}`.

Measured on `SerialObjectFactory` (0x0000d2a0), whose `SERIAL_CREATE` switch inlines
67 default ctors: the missing seeds accounted for **92 of a 108-instruction deficit**
(`CGrunt` alone 60 — its 30 clock members), and closing them took the function
86.74% -> 97.55%.

The offsets are exact evidence: the retail store set is precisely the class's
8-byte clock members and nothing else — `CPathHazard` seeds `m_leg` and `m_strike`
(two `CHazardTimer`, 0x108/0x120) and skips the `i32 m_strikeArmed` between them.

## Rule

* The seeds go in **declaration order**, at the member's declaration position,
  interleaved with the member ctor calls — so on a class that also has member ctors
  (`CGrunt` has `CMotionState`, two `CPtrList`, three `CString` and a `CGruntCellRec[9]`
  vector ctor) it must be a **mem-init list**, not a ctor body. A body store cannot be
  scheduled ahead of a member ctor call and the interleave will not reproduce.
* **MSVC 5.0 accepts a mem-init for a member of an anonymous union, including a
  nested one** (`union { struct { union { i64 m_wingzClock64; ... }; }; CPairRecord ...; }`)
  — verified on `CGrunt`. So the union modelling is not an obstacle.
* On a class with no other member ctors a plain body (`m_armClock = 0;`) is equivalent.

## Residue

Retail groups the four stores of each PAIR together (`lo,lo,hi,hi` per pair) while a
list of independent `i64` mem-inits lets cl's scheduler hoist every `lo` first. That
is the signature of each pair being ONE inlined unit — i.e. a `CPairRecord`-like
struct with a zeroing default ctor rather than two loose `i64`s. Converting the
modelling is a separate, wider change.

related: [base-ctor-pinned-out-of-line-costs-every-derived-ctor.md](base-ctor-pinned-out-of-line-costs-every-derived-ctor.md),
[instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md),
[i64-timer-pairs-are-faithful]

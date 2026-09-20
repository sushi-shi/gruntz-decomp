# A member declared as the DERIVED type stamps a vtable retail never emits
tags: cpp:member cpp:ctor cpp:virtual cpp:struct | asm:mov | topic:correctness topic:identity
symptoms: walls vptrscan withholds a ctor row, ours has one extra stamp, extra DIR32 relocation, base relocs one higher than target, one extra instruction in a constructor, `??_7X@@6B@` with no counterpart in retail .rdata, a class whose only uses are static_cast
confidence: 9/10
variants: vptr-stamp-offset-is-a-silent-identity-defect.md

Declaring a data member as `Derived` where retail has `Base` costs exactly **one
instruction and one relocation**: cl expands the implicit `Derived::Derived()`
into the enclosing constructor, which calls the base ctor and then stamps
`??_7Derived@@6B@`. Retail, whose member is the base, calls the base ctor and
stamps nothing. Sizes and layouts are identical - `Derived` adds no data - so
nothing but the vptr census sees it.

The current source stores a real `CPtrList` member and traverses it through
`POSITION` and the SDK API. A derived member would add an unsupported vptr
stamp. The earlier cast-only subclass was also removed by the SDK audit;
see the correction below.

## The trap: a missing vtable does NOT mean a missing class

cl emits `??_7X@@6B@` only when it generates a constructor or destructor for X.
A class that is **only ever reached through a cast** never gets one. So finding
no `??_7Derived@@6B@` in retail proves that nothing CONSTRUCTS the class - which
localises the defect to the member's declared type - and says nothing about
whether the class itself is real. Reading it as "the class is fabricated" leads
to deleting it, its method and its `RVA()` claim, which is a much larger and
worse-evidenced change (measured below).

## Proving the vtable's absence, with a control

MSVC 5.0's linker does not fold identical COMDATs, so a derived class that
overrides nothing still lands its own copy of the base's slots in `.rdata`. Search
the image for any vtable sharing the base's non-overridden slot values, letting the
`vector deleting destructor` slot be anything (a no-override derived class fills it
with its OWN destructor COMDAT, so an exact-bytes search misses it).

The search shape must be controlled before a zero is believed:

| run | result |
|---|---|
| POSITIVE - vtables carrying `CObject`'s Serialize/AssertValid/Dump at slots 2/3/4 | **103** |
| NEGATIVE - same, with slot 4 forced to `0xdeadbeef` | **0** |
| `CPtrList` slots 0/2/3/4, slot 1 free | **1** (only `??_7CPtrList@@6B@` @0x1eb054) |
| any vtable whose slot 0 is `CPtrList`'s slot 0 | **1** |

103 against 1 is what makes the 1 evidence rather than a broken query.

## Evidence

`walls vptrscan --all` WITHHELD `??0CGrunt@@QAE@PAUCGameObject@@@Z` because the
stamp multiset differed: ours `[CMovingLogic, CGruntCoordList, CGrunt]`, retail
`[CMovingLogic, CGrunt]`. `walls diagnose` classed the row REGALLOC/SCHEDULING and
reported "call-set delta: none" - both sides make the same 12 calls, because both
call `??0CPtrList@@QAE@H@Z`; only the stamp differs, and it shows up as base 407
instructions / 34 relocations against target 408 / 33.

Retyping the member: **89.74 -> 92.39**, a new MAX, and the census drops from 4
withheld rows to 3 with 976 aligned pairs and 0 defects.

## Identity needs positive evidence

Earlier tests found that replacing the cast-only `NextData` wrapper with the
SDK API expanded eight calls and changed caller scores. That is evidence about
inline boundaries, not proof of a source class. No allocation, member, or
independent class identity supported the subclass. The old `MfcNoInline.h`
experiment was inert because `afxcoll.inl` had already been parsed.

## SDK audit correction (2026-09-08)

The absence of a derived vtable remains insufficient by itself to disprove a
class. It also never supplied positive evidence for the cast-only class used
here. The retained `NextData` body was exactly the shipped non-const
`CPtrList::GetNext`, and every underlying object was constructed as `CPtrList`.
The SDK cleanup removes the fabricated subclass and its downcasts, uses the
public list API, and attributes 0x29a30 as an MFC header inline, consistently
with the nearby SDK CRect constructor at 0x29ac0. Current TUs expand all uses;
no fake call, extra definition, or source visibility switch is retained to
force an emitter. Earlier caller-score differences record an inline boundary
question; they do not justify a fabricated class or private-node layout.

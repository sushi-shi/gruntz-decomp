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

```cpp
// NO - CGrunt::CGrunt gains `mov [reg],OFFSET ??_7CGruntCoordList@@6B@`
class CGruntCoordList : public CPtrList { public: void*& NextData(POSITION&); };
CGruntCoordList m_coordList;

// YES - the member is the base; the derived type is still reachable where the
// source really used it, through a cast, which emits nothing at all
CPtrList m_coordList;
CGruntCoordList* CoordListOps() { return static_cast<CGruntCoordList*>(&m_coordList); }
```

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

## Do not over-correct

Deleting the class outright - member to `CPtrList`, `NextData` respelled
`GetNext`, the `RVA(0x29a30)` claim moved to `functions_static_libs.tsv` as an
out-of-line MFC header inline - was measured and is WORSE: 19 fresh regressions
instead of 10, because the eight call sites lose retail's out-of-line call
(`CVoiceManager::SelectVoiceVariant` 100.00 -> 83.71 on its own) once cl expands the
tiny `CPtrList::GetNext` body. Reproducing that call honestly is an inline-budget
question, not a spelling one: `<MfcNoInline.h>` cannot reach `afxcoll.inl`, because
the canonical include order parses `<Mfc.h>` - and with it `<afxcoll.h>` - before
`<MfcNoInline.h>`'s `#undef _AFX_ENABLE_INLINES` is seen. Moving that `#undef`
ahead of `<afxcoll.h>` inside `<MfcNoInline.h>` is byte-for-byte inert for exactly
that reason (measured: identical score, identical 19 rows).

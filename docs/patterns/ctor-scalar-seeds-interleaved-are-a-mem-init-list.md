# Ctor scalar stores INTERLEAVED with member ctors = a member-initializer list

**Tags:** `cpp:ctor` `cpp:vtable` | `asm:mov` `asm:call` | `topic:codegen-idiom`
**Confidence:** 9/10

## Symptom

A constructor's retail body runs the destructible members' ctors and the plain-scalar
zeroing **woven together**, each scalar group landing exactly between the two member
ctors that bracket it in declaration order, and the derived `??_7` vptr stamp only
*after* all of them:

```
call CString::CString      ; member @+0x1b4
mov  [esi+0x328],ebx       ; scalars declared between +0x1b4 and +0x370
...
lea  ecx,[esi+0x370]
call CPtrArray::CPtrArray  ; member @+0x370
call ??_L                  ; the CPtrArray[4] vector @+0x3a4
mov  [esi+0x3f8],ebx       ; scalars declared between +0x3a4 and +0x410
lea  ecx,[esi+0x410]
call CString::CString      ; member @+0x410
mov  [esi+0x430],ebx       ; scalars declared between +0x410 and +0x488
...
mov  [esi],offset ??_7CPlay@@6B@   ; the derived stamp, LAST
mov  [esi+0x1bc],ebx       ; and then the real body
```

Writing the scalars in the ctor BODY instead emits every member ctor back-to-back,
then the vptr stamp, then all the stores in one run - a large, obviously-wrong block
move that no amount of statement reordering inside the body will fix.

## Cause

MSVC processes base ctors, member ctors and member INITIALIZERS as one walk in
declaration order, and only stamps the derived vptr when that walk finishes. So a
scalar seeded in the mem-init list is emitted at its declaration position, in among
the member ctor calls; a scalar assigned in the body is emitted after the stamp.
The split point in the retail bytes therefore *tells you* which fields were
initializers and which were body statements.

## Fix

Move exactly the fields that appear before the `??_7` stamp into the
member-initializer list, and leave the rest in the body.

```cpp
CPlay::CPlay()
    : m_bootyTimerLo(0), m_bootyTimerHi(0), /* ... 36 scalars ... */ m_snapDurHi(0) {
    m_1bc = 0;      // everything retail stores AFTER the vptr stamp
    ...
}
```

## Caveat (measured)

**MSVC5 emits mem-initializers in DECLARATION order, not written order.** Reordering
the list to match retail's emission order changes nothing. So the within-group store
order is the scheduler's and is not steerable from the list.

## Evidence

`CPlay::CPlay` (0x8c9d0) **48.4 -> 76.5** (inlining the base `CState` ctor, see below)
**-> 94.1** on converting the nine 64-bit timer quads to member initializers. The 5.9%
residue is the intra-quad store pairing (retail lo/interval/hi/intervalHi vs cl's
strict +0/+4/+8/+0xc), which the caveat above proves is not source-steerable.

A second lever showed up in the same function: retail also INLINES the base ctor
(`mov [esi],??_7CState@@6B@` + the whole base field seed emitted in place, no `call`).
Reproduce that by defining `inline CBase::CBase() {...}` in the derived class's .cpp,
keeping the RVA-bound out-of-line copy in the base's own TU - the arrangement
GruntzMgr.cpp already uses for `~CPlay` and MoviePlayer.cpp for `~CFecFile`.

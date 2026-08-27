# Ctor scalar stores INTERLEAVED with member ctors = a member-initializer list

**Tags:** `cpp:ctor` `cpp:vtable` | `asm:mov` `asm:call` | `topic:codegen-idiom`
**Confidence:** 10/10

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

When a repeated four-store group is `+0,+8,+4,+0xc`, model the group itself as
one member object. That order is the inline constructor body's source order,
not declaration-order scalar initialization:

```cpp
struct ClockInterval {
    Clock64 m_start;
    Clock64 m_interval;

    ClockInterval() {
        m_start.m_lo = 0;
        m_interval.m_lo = 0;
        m_start.m_hi = 0;
        m_interval.m_hi = 0;
    }
};

CPlay::CPlay() {
    m_returnToMenuOnComplete = 0; // stores after the derived vptr stamp
    // ...
}
```

## Caveat (measured)

**MSVC5 emits mem-initializers in DECLARATION order, not written order.** Reordering
the list to match retail's emission order changes nothing. A repeated non-layout order
such as `+0,+8,+4,+0xc` is therefore positive evidence for an inline member constructor.

The following destructible member's setup may still braid with those stores. In
`CPlay`, retail schedules the next receiver `lea` and EH-state byte between the low
and high halves at the sync, cue, region-0, and region-3 boundaries; the reconstructed
TU schedules that setup before all four stores. That residue is scheduling, not grounds
to split the proven 16-byte object back into four scalars.

## Evidence

`CPlay::CPlay` (0x8c9d0) **48.4 -> 76.5** (inlining the base `CState` ctor, see below)
**-> 94.1** on moving the 36 scalar aliases into the member-initializer run. Realizing
the nine `ClockInterval` objects then changed every four-store group from
`+0,+4,+8,+0xc` to retail's `+0,+8,+4,+0xc`; a one-object A/B moved 94.1037 ->
94.1185 and moved the first mismatch forward. The complete model keeps the exact retail
size (0x2bd), 137 instructions, five calls, no branches, one return, and ten relocations;
its first divergence moves from +0xdb to +0x103. Its lower current fuzzy is an alignment
effect from the remaining call-setup braid, while historical MAX preserves 94.1037.

A second lever showed up in the same function: retail also INLINES the base ctor
(`mov [esi],??_7CState@@6B@` + the whole base field seed emitted in place, no `call`).
Reproduce that by defining `inline CBase::CBase() {...}` in the derived class's .cpp,
keeping the RVA-bound out-of-line copy in the base's own TU - the arrangement
GruntzMgr.cpp already uses for `~CPlay` and MoviePlayer.cpp for `~CFecFile`.

`CGruntzMgr::Run` (0x83450) supplies an independent aggregate control while it
inlines `CTriggerMgr::CTriggerMgr`. Retail zeros three consecutive 16-byte timer
bands at `+0x290`, `+0x2b0`, and `+0x2c0` before the `CPtrList[10]` vector
constructor at `+0x2d0`, each in the non-layout order `+0,+8,+4,+0xc`. Body
assignments cannot precede that later member constructor. Modeling all three bands
as `CueTimer` members with the same inline default constructor reproduces the full
store order and moves `Run` 89.76 -> 91.20. Deleting the stores was a false control:
it rose only to 90.41 while leaving all twelve retail stores target-only. The
aggregate is therefore selected by emission order and byte presence, not score.

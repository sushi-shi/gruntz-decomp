# Two adjacent 8-byte fields serialise through ONE hoisted base, not two `lea`s
tags: cpp:switch cpp:member cpp:inline cpp:serialize | asm:lea asm:add asm:call | topic:codegen-idiom
symptoms: a `SerializeMove`/`Serialize` whose only diff is one extra instruction; retail has `lea reg,[this+off]` ABOVE the `cmp mode,4` and reaches the second field with `add reg,8`, ours recomputes `lea eax,[this+off]` and `lea eax,[this+off+8]` inside each arm; 87-96% plateau
confidence: 10/10

An engine object that stores a **timing pair** — two adjacent `i64`/`Clock64`
fields, usually a start stamp and a duration — transfers both halves through the
tree's inline helper `SerBandPair` (`include/Gruntz/SerialRecords.h`), not through
a hand-written two-arm `switch`.

The helper takes the pair's address ONCE, so cl hoists the `lea` above the mode
test and strength-reduces the second field to `add reg,8`:

```asm
lea    ebp,[edi+0x88]         ; the pair base, ABOVE the switch
cmp    ebx,0x4
je     <load>
cmp    ebx,0x7
jne    <end>
mov    edx,[esi]              ; save arm
push   0x8
push   ebp                    ; +0x88
call   [edx+0x2c]
mov    eax,[esi]
add    ebp,0x8                ; +0x90 - NOT a second lea
push   0x8
push   ebp
call   [eax+0x2c]
```

The hand-expanded switch computes a fresh `lea` per field inside each arm, which
is one instruction more and re-colours the surrounding registers:

```cpp
// WRONG - two leas per arm, no hoist
switch (mode) {
    case SERIAL_SAVE:
        ar->Write(&m_lastDropTime, sizeof(m_lastDropTime));
        ar->Write(&m_dropInterval, sizeof(m_dropInterval));
        break;
    case SERIAL_LOAD: /* the mirror */ break;
}

// RIGHT
SerBandPair(ar, mode, &m_dropTiming);        // the CPairRecord union member
```

Where the class already carries a `CPairRecord` union over the two `i64`s, pass
that; where it does not, `&m_firstField` is enough (the helper's parameter is
`void*`).

**The out-of-line spelling is real too, and is NOT this pattern.**
`CPairRecord::Serialize` (0x058ee0) is the same transfer written as the two-arm
switch and is byte-exact that way. The hoist is what distinguishes the *inlined*
sites, so decide per call site by whether retail's `lea` sits above the `cmp`.

Fixed at 100% EXACT by this: `CObjectDropper::SerializeMove` 0x0c6680 (97.58),
`CTimeBomb::SerializeMove` 0x0e2080 (87.66), `CActionArea::SerializeMove`
0x008600 (92.70), `CAniPlayer::Serialize` 0x0e5c90 (94.66),
`CToyPeek::SerializeMove` 0x0983e0 (95.38). `CTimer::HandleEvent` 0x09c1c0 has
TWO such pairs and went 84.58 -> 99.43 (residue: an ebx<->ebp coin flip between
`this` and the `SerialMode` parameter).

Finding the rest: the source-side sieve is two consecutive
`X->Write(&m_a, sizeof(m_a)); X->Write(&m_b, sizeof(m_b));` inside a
`case SERIAL_*:` arm where both members are 8 bytes wide.

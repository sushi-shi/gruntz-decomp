# An inline member receiver selects a different operand-load order than an equivalent free helper

tags: cpp:inline cpp:member cpp:expression cpp:local | asm:mov asm:and | topic:codegen-idiom topic:regalloc
confidence: 10/10
symptoms: a symmetric pair of bit-mask expressions has exact calls, CFG, size,
and arithmetic, but one arm loads the two operands in the opposite order; free
helper argument-order variants are byte-flat

## Finding

Preserve the owning object as the inline helper's receiver. For an operation
whose semantic ownership is `attacker`, write:

```cpp
inline i32 CGameObject::AttackBits(CGameObject* target) const {
    return static_cast<i32>(target->m_objectType) & m_attackTypeMask;
}
```

and call `attacker->AttackBits(target)`. Do not flatten the receiver into a
second ordinary parameter merely because the emitted operation is one `and`.
MSVC 5.0 carries the receiver through a different C1 value path, and that can
decide which member is loaded first after the helper expands.

## Controlled evidence

`CDDrawChildGroup::CollideBroadcast` at `0x159f00` began at 94.3929. Restoring
the shared object-rectangle macro, symmetric mask-local census, and the authored
health assignment expression moved it through 97.79 to 99.80. A file-local
`AttackBits(target, attacker)` helper then left only the second symmetric attack
mask wrong:

```asm
; free helper expansion
mov ebp, [esi+0xf0] ; attacker mask
mov ecx, [edi+0xe8] ; target type
and ebp, ecx

; retail
mov ebp, [edi+0xe8] ; target type
mov ecx, [esi+0xf0] ; attacker mask
and ebp, ecx
```

Reversing the free helper's parameters, staging the two operands, reversing the
commutative expression, and wrapping the same expression in a target-first free
helper were byte-identical. Moving the operation into the real `CGameObject`
class and calling `oi->AttackBits(oj)` changed only those two loads. The whole
function then matched exactly: 0x22e bytes, 168 instructions, six calls, 33
branches, one return, and three ordered relocations, 100.0000 fuzzy.

This closure followed two prior bounded campaigns comprising 231 and 388
compiled states. Their best free-helper/TU-state islands reached 99.940475 but
could not make the member-owner distinction, so breadth within the flattened
abstraction was not evidence against the class-level source shape.

Because the method is an authentic shared-header inline, parsing it perturbed
six unrelated current rows. Their historical MAX values remained preserved;
banking the new source hashes adjudicates that expected C1 movement. Do not move
the helper back to file scope merely to protect the transient current scores.

Reverse-audit rule: when a two-object operation has a natural semantic owner and
only a commutative operand-load pair differs, try the owner as an inline member
receiver before scheduling permutations. Require the member form to explain the
ownership as well as the bytes; an invented receiver is not a register lever.

## Named-result control in the later helper-rich TU

The member boundary is not a context-independent guarantee of load order.
At `4a4cbe25d`, the same direct-return member and both caller sites remain,
but both attack-mask load pairs are reversed. The function still has exactly
558 bytes, 168 instructions, six calls, 33 branches, one return, a 48-byte
frame, and three ordered relocations. Its first difference is at byte `+0x84`;
constants, stores, displacements, and ordered referents agree.

Giving the existing member a real result local repairs the first pair:

```cpp
inline i32 CGameObject::AttackBits(CGameObject* target) const {
    i32 bits = static_cast<i32>(target->m_objectType) & m_attackTypeMask;
    return bits;
}
```

The first remaining difference moves to `+0x1c1`, the second attack-mask load
pair. All the counts above remain unchanged. Restoring the direct return in
the same real TU reproduces both incorrect pairs. Initializing `bits` with the
target type and then applying `&=` emits the same caller as the named complete
expression: the useful distinction is the result-local boundary, not that
compound assignment spelling.

Negative composition controls leave the same second pair: a separate target
type local, a separate attack-mask local, a const-pointer target parameter,
a const-reference target parameter with both caller arguments dereferenced,
and a named result in the adjacent `ObjectTypeBits` helper. None closes the
remaining difference, and those additional changes are not retained.

Reverse use: preserve a historically proven member receiver, then distinguish
its expression return from a named real result before changing caller control
flow. A recovered operand pair is partial progress, not an exact closure or
proof that the remaining source families are exhausted.

The shared-header edit also recovers the unchanged `CTimer::AddTime` at
`0x9c0e0`: 99.8214 to exact, 163 bytes, 57 instructions, no calls or
relocations, five branches and one return. This is a TU-state consequence,
not evidence that the timer uses the attack-mask helper. Conversely,
`CSpotLight::Tick`, which also does not use it, moves from 84.2097 to 83.2984
with its existing CFG mismatch and unchanged source fingerprint; its banked
84.2097 remains preserved. Neither movement licenses unrelated source edits.

# Whole-object assignment pins an aggregate to the frame; field-wise assignment lets cl enregister it

tags: cpp:struct cpp:local cpp:assignment | asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail keeps both halves of a small struct local in ONE pair of frame
slots and reloads them (`mov eax,[esp+N] / mov ecx,[esp+N+4]`) where our build
carries one half in a callee-saved register and homes some OTHER local instead;
an extra `mov <save>,eax` copy, an extra spill and an extra reload; the frame is
one dword smaller than retail's
confidence: 9/10
variants: aggregate-copy-materializes-only-under-register-pressure.md

## Symptom

`CBattlezMapConfig::ResolveTileClaim` 0x2dfa0. Both sides run the same block:
seed a coordinate from a member, conditionally replace it from a list node, and
write it back through a flag.

```asm
; retail - `saved` is an object, both halves live in the frame
mov  ecx,[eax]                  ; c->m_x
mov  edx,[eax+0x4]              ; c->m_y
mov  ebp,0x1                    ; flag  -> a callee-saved register
mov  [esp+0x50],ecx             ; saved.m_x
mov  [esp+0x54],edx             ; saved.m_y
...
test ebp,ebp
mov  eax,[esp+0x50]
mov  ecx,[esp+0x54]
mov  [esi+0x174],eax
mov  [esi+0x178],ecx

; ours - cl scalar-replaced `saved`, so the FLAG got the frame slot
mov  ebp,eax                    ; saved.m_x -> a callee-saved register
mov  [esp+0x28],edi             ; saved.m_y
mov  DWORD PTR [esp+0x48],0x1   ; flag  -> the frame
...
mov  eax,[esp+0x48]
test eax,eax
mov  ecx,[esp+0x28]
mov  [esi+0x174],ebp
mov  [esi+0x178],ecx
```

The two sides allocate the mirror of each other, and ours pays an extra copy, an
extra spill and a reload for it.

## Cause

cl 5.0 scalar-replaces a small aggregate local whose every read and write names a
FIELD. One whole-object assignment - `a = *p`, or `dst = a` - is an operation on
the object, and it stops the replacement: the local becomes a real 8-byte frame
object, and the callee-saved register it would have taken goes to whatever local
is next in line.

So the spelling of the assignment, not the declaration, is the lever. Declaring
`Coord saved` instead of two `i32`s is byte-flat on its own; what moves the
allocation is writing

```cpp
saved = *c;                       // not saved.m_x = c->m_x; saved.m_y = c->m_y;
unit->m_entrancePx = saved;       // not the two field stores
```

## Evidence

Steerable. `ResolveTileClaim` 82.6708 -> 84.0000 with no other change, and its
frame moved to retail's 0x38 from 0x34 - the residue an earlier review had
recorded as the wall. Aggregating the two `i32` locals into one `Coord` first was
byte-identical (same instructions, slots renumbered), which isolates the
assignment spelling as the whole of the effect.

The reverse reading also holds: when retail keeps a small struct in the frame and
we enregister half of it, look for a whole-object assignment in retail's bytes
(an adjacent 2- or 4-dword load/store run with no arithmetic between) before
reaching for a register-pressure explanation.

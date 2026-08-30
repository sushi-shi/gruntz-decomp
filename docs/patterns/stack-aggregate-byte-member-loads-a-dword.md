# `andl $0xff` on a byte member of a STACK aggregate is cl's own idiom, not a wrong lvalue
tags: cpp:local cpp:class cpp:cast | asm:and asm:mov asm:movzx | topic:wall
symptoms: base `mov ecx,dword ptr [esp+0x1d]` + `and ecx,0xff` where retail has `xor eax,eax` + `mov al,BYTE PTR [esp+0x1d]`; a `mask_immediates` row reading base-only `0xff`; an unaligned dword read at an odd frame displacement
confidence: 10/10
variants: byte-field-plus-struct-copy-reads-the-pad.md

cl 5.0 has two byte-widening idioms and picks between them by **where the byte
lives**, not by anything you can write. A `u8` member of a **frame-local
aggregate** promoted to `int` is always `mov r32, m32` + `and r32, 0xff` - an
unaligned 32-bit read at the member's own displacement. The same member reached
through a **pointer** is always `xor r32,r32` + `mov r8, m8`. So an unaligned
dword read masked down to a byte is NOT evidence that the source widened a
narrow field: check whether the lvalue is a local or a deref first.

```cpp
BrickzCell rec = *tile;              // -> mov ecx,[esp+0x1d]; and ecx,0xff
if (rec.m_occupantIdBytes[1] == m_ownerId)

if (tile->m_occupantIdBytes[1] == m_ownerId)   // -> xor ecx,ecx; mov cl,0x5(%edx)
```

Measured over eleven probe variants, all of which produced the dword+mask form:
union member and plain `u8` field; `memcpy` (extern and intrinsic), struct
assignment and copy-initialisation; a pointer to the local, a reference to the
local, a pointer declared before the copy; an inlined helper taking the local by
reference and one taking it by pointer; a one-element array reached by decay;
index 0 as well as index 1. Only a genuine pointer parameter flipped it. `char`
instead of `u8` gives `movsbl`, and `u8 == u8` gives a byte compare - neither is
this shape.

CBattlezMapConfig::StepRowSpawn 0x26470 reads this way and its
`m_occupantIdBytes[1]` is byte-correct. Restoring the bounded candidate loop
and the inline `CPtrArray::GetAt` access recovered retail's index in EBP,
spilled array cursor, branch/return topology and frame. The remaining residue
is now precisely this local-aggregate byte-load choice plus its downstream
register colouring. A target-adjacent 128-state C1 forest was flat at the
88.8080 source island, and the eleven local-aggregate variants above bound the
source-level family.

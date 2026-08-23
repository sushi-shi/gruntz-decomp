# A hand-expanded `abs` spills a sign mask where retail spends `cdq`
tags: cpp:expression cpp:local | asm:cdq asm:sar asm:xor | topic:codegen-idiom
symptoms: `walls loopscan` names `retail+cdq` against `ours+sar`; a lone `sar r,0x1f` followed by a spill of the mask and a reload; source containing `(x ^ (x >> 31)) - (x >> 31)`
confidence: 9/10

cl 5.0 compiles `abs()` to `cdq / xor / sub`, which costs no register: EDX is
the mask. Transcribing that sequence back as `(x ^ (x >> 31)) - (x >> 31)`
gives the mask a NAME, so it becomes a value with a live range - and when a
second `abs` in the same expression wants EDX, the first one's mask is spilled
and reloaded.

```cpp
// ours - the mask is a named value, so it spills across the second term
i32 dx = gx - (m_object->m_screenX >> TILE_SHIFT_PX);
dx = abs(dx);
i32 dy = gy - (m_object->m_screenY >> TILE_SHIFT_PX);
i32 dist = ((dy ^ (dy >> 31)) - (dy >> 31)) + dx;

// retail - two intrinsics, two cdq, no spill
i32 dx = gx - (m_object->m_screenX >> TILE_SHIFT_PX);
i32 dy = gy - (m_object->m_screenY >> TILE_SHIFT_PX);
i32 dist = abs(dx) + abs(dy);
```
```asm
; ours
sar edx,0x1f
mov DWORD PTR [esp+0x14],edx      ; the named mask spills
...
mov eax,DWORD PTR [esp+0x14]      ; and reloads
; retail
cdq
xor edi,edx
sub edi,edx
...
cdq
xor eax,edx
sub eax,edx
```
Steerable, and a direct instance of the INLINE/MACRO PRIOR: the expansion is
C2's output, not source. `CGrunt::StepGooSuckerBehavior` 0xf0e20 81.56 ->
89.10, and all four of its loop bodies go from a size mismatch to exact.
Grep the tree for `>> 31)) - (` before believing any `sar r,0x1f` is source.
The evaluation ORDER falls out too: two `abs()` calls let cl take retail's
term order (dx then dy), which the mixed form could not.

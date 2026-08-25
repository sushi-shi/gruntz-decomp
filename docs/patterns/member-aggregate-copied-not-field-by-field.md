# Four scalars read out of a member RECT are a STRUCT COPY — and the frame size proves it

tags: cpp:struct cpp:local cpp:member | asm:mov asm:add | topic:codegen-idiom topic:regalloc
symptoms: `sub esp,N` bigger than the slots you write, reserved-but-never-touched dwords,
`add reg,0x10` / `mov ebx,eax` then `[ebx]`/`[ebx+4]`/`[ebx+8]`/`[ebx+0xc]`,
a member stored twice in a row (`mov [esp+N],eax; inc eax; … mov [esp+N],eax`)
confidence: 9/10
variants: frame-slot-holes-prove-a-local-aggregate.md, struct-copy-dead-member-store-frame.md

Reading a member `RECT`/`SIZE` into loose scalars (`i32 l = vp.left, t = vp.top, …`)
looks byte-equivalent and is not. cl reserves an aggregate local's WHOLE footprint even
when it enregisters every member, so retail's frame is `4 * (members it enregistered)`
bytes bigger than yours — that hole IS the aggregate. Two more tells: the source address
is materialized once (`add eax,0x10` then a `mov` into a callee-saved register) instead of
folding `+0x10` into each disp8 load, and an adjusted member keeps the copy's own dead
store in front of the adjusted one.

```cpp
// before - four scalars: no frame slots, +0x10 folded into every load
LevelCoordRect& vp = m_world->m_level->m_planeCtx;
i32 l = vp.left, t = vp.top, r = vp.right, b = vp.bottom;

// after - the copy (LevelCoordRect IS tagRECT)
RECT vp = m_world->m_level->m_planeCtx;
// …and where only some members are adjusted, copy THEN bump:
RECT r = vp;  r.right = r.right + 1;  r.bottom = r.bottom + 1;
```

```asm
add    eax,0x10          ; the aggregate's address, materialized once
mov    ebx,eax
mov    eax,DWORD PTR [ebx]      | mov [esp+0x14],eax   ; the copy's own store …
mov    ebp,DWORD PTR [ebx+0x4]  | inc eax
                                | mov [esp+0x14],eax   ; … then right+1 over it
```

STEERABLE, and it cascades: fixing the copy also recovers the `clamped`-in-register merge
and the pointer's callee-saved colour. Evidence (2026-07-28, `src/Gruntz/Play.cpp`):
`LoadSBITextEdges` 97.8 → **100 EXACT** (frame 0x18 → retail's 0x24 = 3 enregistered
members), `PlayCueAt` 88.0 → **100 EXACT** (0x14 → 0x24), `ShrinkViewport` 93.8 → **100
EXACT** (copy + hoisting `clamped = 0` above it), `NotifyVisibleEntities` 85.5 → **100
EXACT**, `ExpandViewport` 85.8 → 91.3. The sibling `StepScroll` 63.5 → **100 EXACT** is
the pointer half alone: `RECT* vr = &v->m_mainPlane->m_viewRect;`. All six were filed as
register-coloring walls.

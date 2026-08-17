# The in-place tile snap's pointer/value register pair is canonical - no spelling reaches retail's `and al,0xe0`
tags: cpp:member cpp:assign cpp:local | asm:and asm:mov | topic:wall topic:regalloc
symptoms: retail `mov ecx,[this+off]` / `mov eax,[ecx+X]` / `and al,0xe0` where the base has `mov eax,[this+off]` / `mov ecx,[eax+X]` / `and ecx,0xffffffe0`; base two bytes longer per coordinate; the rest of the function byte-identical
confidence: 9/10

`p->m_screenX = (p->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;` twice (X then Y)
is retail's tile-centre snap, written in ~20 places. Retail always emits

```asm
mov ecx,DWORD PTR [esi+0x10]   ; the object pointer -> ECX
mov eax,DWORD PTR [ecx+0x5c]   ; the coordinate     -> EAX
and al,0xe0                    ; 2 bytes, only reachable from AL
add eax,0x10
mov DWORD PTR [ecx+0x5c],eax
```

and cl 5.0 SP3 always gives us the pair the other way round (`pointer -> EAX,
value -> ECX/EDX`), which costs the 3-byte `and reg,0xffffffe0` twice. The
`and al` form is a pure size peephole - it is 2 bytes only for AL, so it fires
iff the VALUE lands in EAX, and that follows from the register pair alone.

MEASURED REFUTED, real cl 5.0 `/O2 /MT /GX /GR`, in an isolated harness that
reproduces the base byte-for-byte (sortKey block above, two `this`-calls below,
`this` in ESI): in-place; reversed `+` operands; `& 0xffffffe0`; a value temp
per statement; both temps then both stores; `&=` plus `+=`; `x - (x & 0x1f) + K`;
`(x>>5<<5)+K`; a `Coord*` view of the pair; a cached `Obj* o` local; an inline
member on the OWNER (`this`-based) and on the OBJECT (`p`-based); a
by-reference `SnapRef(int&)` helper; and a value-returning `Snap(int)` helper.
All fifteen emit the base pair. The pointer takes EAX whenever ECX holds `this`
in a different register - the only probe that ever produced retail's pair was
the one where cl folded the load's destination onto the dying `this` register
(`mov ecx,[ecx+0x10]`), which the real functions cannot reach because `this`
lives in ESI there.

The tree-wide screen is mechanical: count `and al,0xe0` per unit in
`build/objdiff/compare-new/{base,target}`. Retail has 2 per snap PAIR; we have
1 when the coordinates are computed into named locals used later, and 0 for the
in-place form. Units where retail has 2 and we have 0: exittrigger,
gruntcreationpoint, gruntvoice, secretteleportertrigger (4), statichazard,
warlord, wormhole (4).

So this is a C2-anchored register-pair wall, not a source defect: do NOT
re-derive it, do NOT introduce a `SnapToTileCenter` helper for it (measured
byte-identical, and it is an invented abstraction), and do not read the missing
`and al` as a missing statement. The same pair decides the mergeability of a
neighbouring `flags |= imm` - see
[switch-arm-tail-crossjump-vs-duplicate.md](switch-arm-tail-crossjump-vs-duplicate.md).

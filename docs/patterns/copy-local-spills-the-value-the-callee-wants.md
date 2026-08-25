# A local that only copies a value into the call is what spills it

tags: cpp:local cpp:pointer cpp:call cpp:union | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: base frame is 4 bytes larger than target; a value retail keeps in ESI/EDI across several calls is reloaded from `[esp+N]` at each use in the base; the callee-saved bindings are rotated by one against retail; `walls diagnose` reports REGALLOC/SCHEDULING with equal instruction counts

cl 5.0 assigns callee-saved registers to the call-crossing values by weight, and
one register short is a cascade: every later binding rotates and every `[esp+N]`
displacement moves. Two source shapes decide the weights, and both are about
WHERE a value is named rather than what it computes.

**Drop a copy-local that only feeds calls.** A cursor copied out of a union (or
any aggregate) into its own local becomes a separate value competing for a
register; passed directly, the union itself carries the live range and wins one.

```cpp
// base: `data` is spilled, and the four member addresses the tail needs spill too
RecordBytes<PidHeader> p;
p.m_dwords += 4;
u8* data = p.m_bytes;
...
DecodeRun8(data);
DecodeByteRun1Plane(decoded, data, w, h);

// retail: the union cursor stays in ESI across both calls
RecordBytes<PidHeader> p;
p.m_dwords += 4;
...
DecodeRun8(p.m_bytes);
DecodeByteRun1Plane(decoded, p.m_bytes, w, h);
```

**Declare a global-receiver read where it is first used.** Declared early it is
scheduled early and takes the first free register; sunk to its use it lands where
retail has it.

```cpp
// base: the g_gameReg load is emitted before the object's coordinate reads
CGruntzMapMgr* b = g_gameReg->m_tileGrid;
i32 px = (mx & ~TILE_MASK_PX) + TILE_HALF_PX;
i32 tx = px >> TILE_SHIFT_PX;

// retail: coordinates first, then the grid
i32 px = (mx & ~TILE_MASK_PX) + TILE_HALF_PX;
i32 tx = px >> TILE_SHIFT_PX;
CGruntzMapMgr* b = g_gameReg->m_tileGrid;
```

Steerable. `CDDSurface::DecodePcxData` 0x1457a0 92.94 -> 99.98 (residue is only
the palette loop's end bound resolving to the neighbouring `g_warpU` symbol);
`CGrunt::TryPowerupAtTile` 0x57aa0 93.31 -> 100.00 EXACT. The mirror is also
real: binding a receiver that retail reloads is what fixes
`CSpotLight::CSpotLight` 0xb1200 93.25 -> 96.46 (three draw-fill stores off one
bound object pointer), so read whether retail reloads before adding or removing
the local.

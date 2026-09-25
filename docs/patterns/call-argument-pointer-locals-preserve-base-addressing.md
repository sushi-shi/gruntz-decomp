# Name the call-argument pointers, not an entire aggregate alias

tags: cpp:local cpp:pointer cpp:call cpp:member | asm:lea asm:push | topic:regalloc topic:codegen-idiom

`CWwdDotObject::BltDirtyRegions` (`0x1664a0`) matched retail except for an
EDI/EBX exchange between the current and previous dirty-region position
addresses. Both pointers were formed before the distance calculation and
survived into two `BlitDirtyRect` calls. Arithmetic still addressed fields
relative to `this`.

Name the exact values crossing that boundary:

```cpp
i32* dirtyPos = &m_dirty.m_lastX;
i32* shadowPos = &m_shadow.m_lastX;
// Keep distance/minimum calculations relative to the real members.
// In the separate-region branch:
dst->BlitDirtyRect(src, dirtyPos, &m_dirty.m_w);
dst->BlitDirtyRect(src, shadowPos, &m_shadow.m_w);
```

These locals belong inside the branch where both regions are valid. They are
the existing API's position arguments, not new storage or a fabricated layout.
This changes **99.6983% -> 100%**: all 307 bytes, 116 instructions, five calls,
eight branches, four returns and five ordered relocations match.

The negative controls distinguish a real local census from generic aliasing.
Naming whole `WwdDirtyRect*` aliases and using them for every field eliminates
the `this` carrier, changes arithmetic addressing and enables a shared tail:
268 bytes, 103 instructions, four calls, nine branches and three returns.
Whole-aggregate references produce the same wrong topology. Neither is a
closer base for this residue, even though both preserve source behavior.

Reverse use: when retail preserves member addresses for call arguments but
retains base-relative field reads, test locals for the argument pointers alone.
Do not replace every member access with an aggregate alias. Recompare from
the first divergence, and preserve the original branch and call topology.

The full TU rebuild also moves the unchanged earlier `CWwdDotObject::BltDirty`
from current 100% to 99.80645%. Its 194-byte / 62-instruction body differs only
in the two pixel-offset multiply carriers inside `CDDSurface::GetPixel`;
four calls, four branches, one return and both relocations agree. Its existing
same-source 100% MAX remains valid. Do not remove the recovered pointer locals
or introduce inert declarations to steer that sibling's current state.

Follow-up: [sequencing the real offset inside the existing GetPixel helper](inline-pixel-offset-statements-select-product-carriers.md)
recovers that sibling's complete normalized pair while retaining these pointer
locals. The perturbation was real, but did not prove that the helper's source
statement grouping was already complete.

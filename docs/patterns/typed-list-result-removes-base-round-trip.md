# Keep a typed intrusive-list result typed

tags: cpp:template cpp:cast cpp:inheritance cpp:local | asm:lea asm:test asm:jcc | topic:correctness topic:cfg

A restored typed list can expose an obsolete caller conversion chain. Inspect
the complete inheritance layout before blaming the list's inline accessor.

`SoundTask` is polymorphic; its non-polymorphic `CBaseListItem` subobject is
at offset four. `CLTList<SoundTask>::GetFirst()` already converts the raw link
to `SoundTask*`. `SoundDevice::TickVolumeRamps` still assigned that result to
`CBaseListItem*`, then downcast it to `SoundVolumeRamp*`. VC5 retained the
nullable base conversions: subtract four, add four, subtract four, with
intervening null guards. Retail performs only the first adjustment.

Keeping the head and cached next node as `SoundTask*` removes the round trip.
Every operation used by this walker belongs to `SoundTask`; no ramp-specific
field is accessed. Its sibling `ClearVolumeRamps` already uses that static
type. The virtual `Tick`, list removal, cached-next lifetime and typed delete
remain unchanged.

Controlled result at `0x136e20`: **86.6667% -> 100%**. Base changes from
190 bytes / 85 instructions / 16 branches to retail's 168 bytes / 75
instructions / 12 branches. All four calls, three returns and four ordered
relocations agree. No helper, inheritance or layout change is needed.

Reverse audit: after recovering a typed container, find callers that erase its
result back to a base and immediately recover a derived pointer. A nonzero
base offset plus repeated null guards is the machine signature. Keep the
narrowest static type required by the consumer, not a gratuitously derived
type. Do not remove necessary adjustments or change ownership to hide them.

# Sentinel-seeding ctor: the arg-store and `-1`/`0x80000000` stores re-permute — scheduling wall
tags: cpp:ctor cpp:local | asm:mov | topic:wall topic:scheduling
symptoms: small __thiscall ctor seeding a few arg fields + several constant sentinels (0x80000000 / -1 / 0); identical instruction multiset, the edx-held last-arg store + the `-1` immediate store float to different positions vs retail; ~60%
confidence: 7/10

A tiny ctor that copies a couple of args into fields and then seeds a block of
constant "sentinel" fields (`0x80000000`, `-1`, `0`) plateaus ~60% because MSVC
5.0 /O2 schedules the **last positional arg's store** (held in `edx`) and the
**`-1` immediate store** at different points than retail. The instruction multiset
is byte-identical; only the *order* of ~3-4 stores permutes:

- retail keeps the edx arg-store EARLY (right after the ecx arg-stores, before the
  shared-`0x80000000` register stores) and interleaves the `-1` store between the
  first and second `0x80000000` store: `mov [t+8],edx; mov [t+0x20],ecx; mov
  [t+0x38],-1; mov [t+0x5c],ecx; mov [t+0x64],ecx`;
- our cl groups the three `0x80000000` stores together, then `xor ecx,ecx` (for the
  trailing `=0` fields), then the edx arg-store and the `-1` store last.

Source order does steer the arg→register pinning (which arg lands in edx: list the
intended-edx arg SECOND so its load precedes `mov eax,ecx`), but it does NOT steer
where that edx store and the `-1` immediate store land in the constant block — that
is the /O2 list-scheduler's free choice on side-effect-only stores with no data
dependency. WALL (logic exact). Evidence: CRemusNode::CRemusNode @0x15b2c0 — three
field-order spellings all ~60%, identical multiset, only the m_08/m_38 stores
re-ordered.

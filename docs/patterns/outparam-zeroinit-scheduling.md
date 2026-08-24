# Lookup-helper out-param zero-init: target SINKS `mov [&out],0` past the arg pushes
tags: cpp:local asm:mov asm:push | topic:codegen-idiom topic:scheduling
symptoms: identical instruction multiset, 2-3 instrs permuted per lookup, plateau ~80-95%, the `Lookup(name,&spr)` family
confidence: 8/10

**NOT A WALL — SUPERSEDED. The lever is
[out-param-reset-between-arg-setup-and-call-is-in-the-helper.md](out-param-reset-between-arg-setup-and-call-is-in-the-helper.md):
put the reset inside an inline wrapper.** cl 5.0 binds an inline callee's arguments
before substituting its body, so `out = 0` written as the wrapper's FIRST STATEMENT is
emitted after the argument computations — retail's schedule, exactly. Confirmed on
`CPlay::SetEffectSpriteDurations` (67.07 -> 98.95) and on three more `CPlay` functions
that went EXACT (`DrawStateMessage`, `DrawMessageFrame`, `LoadLoadingBarSprite`,
2026-08-17). Everything below is still accurate about what does NOT move it, and is
kept because it bounds the CALLER-side search — but the conclusion "source-invariant"
was wrong: the caller-side axis was exhausted, the callee-side axis was never tried.

The sprite/anim "lookup then use" family (`…->Lookup(key, &spr)`) plateaus at high-but-not-100%
because retail SINKS the out-param zero-init store (`mov [&spr],0`) past the `lea &spr`/arg
pushes, while our `cl` HOISTS it before. The instruction MULTISET is identical — only the order
of those 2-3 instructions per lookup permutes.

Affected and worth re-attacking with the helper: SpriteLoaders / IconLoaders /
ActionOptionsMenuBar (84-94%), all CGrunt `Create*` (99%), and the ~14 wwd/levelplane
functions listed below. Convert ONE site at a time and read the per-function score:
the helper is per-site, not per-idiom (`CPlay::LoadCursorAnimation` regressed 97.62 -> 92.68
on it, because its key is a parameter rather than a pooled literal and there is no
`push <literal>` to schedule the store behind).

## Re-measured 2026-07-28 (matcher-3, `CDDrawChildGroup::CreateNamed_1595b0` @0x1595b0)

That function is the minimal case: **68 bytes, every byte identical to retail except this one
store's position** (`push eax / STORE / push ecx` vs retail `push eax / push ecx / STORE`). It is
the shared residue of ~14 functions across `wwdgameobject` / `wwdobjmgr` / `levelplane`
(`AddLogicHit/Attack/Bump`, `LookupAnimSprite`, `ApplyName`, `ApplyLookupSprite/Geometry`,
`ResolveLinkedObject`, `CreateNamed_1593e0/1595b0/159a10`, `CreateSprite`, `Find`,
`PruneOrphans`, `RegisterNamed`, `Read`), so it is worth knowing exactly what does NOT move it:

**Source spellings — six, all byte-identical output** (compiled standalone, `/O2 /MT`):
`T* v = 0;` · `T* v; v = 0;` · an extra unused local declared first · the map bound to a
reference/local first *(this one DOES sink the store — but it also hoists the receiver, so
`lea ecx,[eax+0x10]` replaces retail's `mov ecx,[edx+0x14]; add ecx,0x10`, and the pushes split
around it; no spelling gets both halves)* · a nested scope · the address taken through a `T**`.

**Compiler flags — the whole optimizer axis, all identical:** `/Ox`, `/O2 /Ob2`, `/O2 /Oa`
(assume no aliasing), `/O2 /Ow`, `/O2 /Oi-`, `/O2 /Ot`, `/O2 /Gr`, `/O2 /Gz`. (`/Os` and `/Og-`
change the whole function, not this.)

**`/G6` IS the switch — and it is not retail's compiler setting.** Under `/G6` (Pentium Pro
scheduling) the store sinks past both pushes, exactly as retail has it. But `/G6` also reorders
the two independent loads above the pushes (`mov edx,[esi+0xc]` before `lea eax,[esp+4]`, where
retail matches the `/GB` order), and it changes **49 of the 59 functions** in `wwdobjmgr`,
including many that are currently EXACT. So retail is `/GB`(=`/G5`)-scheduled everywhere else and
still sinks this store: the difference is in the store-sinking heuristic alone, i.e. most likely a
different MSVC 5.0 service-pack build, not a flag we hold. Do not re-derive this; do not switch a
unit to `/G6`.

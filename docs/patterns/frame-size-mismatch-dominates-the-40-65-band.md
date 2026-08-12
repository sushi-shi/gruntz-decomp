# In the 40-65% band, check the PROLOGUE's frame size before anything else

- **confidence** c9
- **tags** `cpp:locals` `topic:method` | `asm:sub-esp` `asm:push` | `topic:wall`

## Symptom

A function whose logic reads correct sits at 40-65% and the `--diff` output is a wall of
`[esp+N]` operands that differ by a constant. The first diff line is the prologue:

```
-sub esp,0x134            -push ecx              -sub esp,0x98      -sub esp,0x50
+sub esp,0x13c            +sub esp,0xc           +sub esp,0x8c      +sub esp,0x54
```

Every stack reference in the body is then off by the delta, and objdiff scores each of
them as a mismatch. **The percentage is measuring one modelling error, not hundreds.**

## Why this is the highest-yield first check

Measured across one 40-65% batch (2026-08-07):

| fn | base frame | retail frame | delta |
|---|---|---|---|
| `CChatBoxOwner::ProcessCheatInput` 0x205c0 | 0x134 | 0x13c | retail +8 |
| `CDDSurface::SaveRle16` 0x144640 | 0x50 | 0x54 | retail +4 |
| `CGrunt::ChargeStep` 0xef6b0 | `push ecx` (4) | 0xc | retail +8 |
| `CGrunt::ResetEntranceAnimation` 0x62e10 | `push ecx` (4) | 0xc | retail +8 |
| `CGrunt::StepArrivalDefense` 0xf2b20 | 0xc | 0x10 | retail +4 |
| `CGrunt::StepDiggerBehavior` 0xf36a0 | 0x94 | 0x8c | retail -8 |
| `CGrunt::StepGooSuckerBehavior` 0xf0e20 | 0x90 | 0x88 | retail -8 |
| `CGrunt::LoadPickupSprites` 0x65e80 | `push ecx` (4) | 0 | retail -4 |

Each delta is a concrete, findable modelling fact, and each is different:

- **retail has MORE** - a local object we never declared. `SaveRle16`'s +4 was a
  `BITMAPINFO` (44 B) modelled as a bare `BITMAPINFOHEADER` (40 B); retail's giveaway was
  `mov ecx,0xb / rep stos` (11 dwords) and a `Write(&hdr, 0x2c)`.
- **retail has FEWER** - a local WE spill that retail keeps in a register, or a local
  retail folds into a dead parameter's home slot. `LoadPickupSprites`'s `push ecx` holds a
  `CAniElement*` scratch; retail reuses the never-read 4th parameter's slot at `[esp+0x20]`
  for it and allocates nothing. After removing two oversized `Coord[2]` out-parameter
  locals, `StepDiggerBehavior` still keeps `this` in `ebx` with an eight-byte-wider frame,
  where retail keeps it in `ebp` the whole way.

## The `mov <reg>,ecx` tell

Which register receives `this` is a readout of the same thing. Retail using `ebp` while cl
uses `esi`/`edi` (or cl emitting a `mov [esp+N],this` spill retail lacks) means cl ran out
of callee-saved registers earlier - usually because it also promoted a constant (see
[known-zero-reload-before-call.md](known-zero-reload-before-call.md)) or because your local
set differs.

## Rule

`gruntz sema disasm <rva> --diff --lite | head -20`. If line 1 of the diff is `sub esp` or a
`push`/`pop` count, **stop reading the body** - reconcile the frame first. Chasing scheduling
inside a body whose frame is off by 4 is measuring noise.

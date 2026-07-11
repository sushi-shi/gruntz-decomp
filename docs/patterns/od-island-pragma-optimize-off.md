# A per-unit /Od profile masking a `#pragma optimize("", off)` ISLAND in an /O2 TU

confidence: 9/10
tags: cpp:pragma cpp:flags | asm:ebp asm:mov | topic:flags topic:codegen-idiom

## Symptoms

- A small cluster of functions matches ~99.5% ONLY under an `/Od` flag profile,
  while every neighboring function in the same retail `.text` block is normal
  `/O2` (register-allocated, frame-pointer-omitted).
- The /Od-looking functions have a full `ebp` frame, every local in a distinct
  `[ebp-N]` stack slot, no strength reduction, no register reuse.
- The TU-layout evidence (one obj = one flag set; the functions are
  text-CONTAINED inside a proven single original obj) makes a per-FILE compiler
  override structurally impossible.

## The mechanism

The original DEVS wrapped exactly those functions in

```cpp
#pragma optimize("", off)
i32 CDDSurface::DecodeRun8(void* src) { ... }
i32 CDDSurface::DecodeRun24(void* src) { ... }
#pragma optimize("", on)
```

MSVC 5.0 honors `#pragma optimize` per-function: the island compiles with the
/Od shape (full frame, per-local stack slots, declaration-order `[ebp-N]`
layout) while the rest of the TU stays /O2. This is period-plausible dev code
(hand-annotated around fragile/buggy optimizer cases — here the RLE
run-decoders).

## Steering

Move the bodies into the real /O2 TU and fence them with the pragma pair. The
fuzzy % is IDENTICAL to the old per-unit /Od profile (measured: DecodeRun8
99.50, DecodeRun24 99.54, RunDecode1 99.57, RunDecode3 99.54 — byte-for-byte
the same residual, the documented `/Od local-slot-ordering` wall
([od-local-slot-ordering](od-local-slot-ordering.md)) is unchanged).

Proof case (wave4-K): the `fileimagerundecode` unit (flags = "od", 4 fns) was a
crutch — its four fns are text-contained in TWO different /GX /O2 objs
(DIRSURF.CPP gets DecodeRun8/24, the file-codec obj gets RunDecode1/3). The
unit is deleted; the pragma islands reproduce the shape in place.

## When you see a singleton flags-profile override

Treat every per-unit `od`/`odi`/`o1` singleton as a SUSPECT masking either a
wrong TU composition or an in-source pragma island; re-derive it from the
TU-layout evidence before trusting it (`config/units.toml` FLAGS doctrine).

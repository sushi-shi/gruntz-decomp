# An "unoptimized-codegen wall" is a `#pragma optimize("", off)` region in the source

**Tags:** `cpp:pragma` | `asm:ebp` `asm:idiv` | `topic:codegen-idiom`
**Confidence:** 9/10

## Symptom

One or two adjacent functions in an otherwise-matching TU plateau very low (30-60%)
with a note blaming a "compile-profile wall" / "/Odi crutch" / "x87 spill schedule".
Their neighbours in the same TU are at 100%. Nothing you do to the source shape helps,
because the residual is not a shape difference - it is an *optimization-level*
difference.

## The tell (read the retail prologue, not the body)

Retail's version of the stuck function is compiled **unoptimized** while its
neighbours are not. Any ONE of these proves it; usually you see all of them:

| retail shows | /O2 would emit |
|---|---|
| `push ebp / mov ebp,esp` frame + `mov esp,ebp / pop ebp` epilogue | frameless, `add esp,N` |
| the incoming parameter reloaded off `[ebp+8]` at *every* use | cached in a register once |
| `mov ecx,100 / idiv ecx` for `x / 100` | magic-number strength reduction (`imul 0x51eb851f / sar`) |
| every intermediate spilled to `[ebp-N]` (incl. x87 `fstp`/`fld` round-trips) | values kept live in registers / st0 |
| every `return` funnelling to ONE shared epilogue via `jmp` | duplicated or tail-merged epilogues |

The **integer-divide-by-constant** row is the single cleanest signal: MSVC5 strength-
reduces constant division at `/O1` *and* `/O2`, so a literal `idiv` by an immediate
divisor means the function was compiled with optimization OFF.

Then confirm the boundary: disassemble the functions immediately before and after.
If they are frameless / strength-reduced, the unoptimized run is a *region*, and the
original source bracketed it.

## Fix

Wrap exactly that run of functions in the pragma. This is real period source, not a
steering hack - a 1990s codebase disabling the optimizer around a hand-tuned FP curve
is exactly why the pragma exists.

```cpp
#pragma optimize("", off)

RVA(0x001350b0, 0x5d)
i32 SoundDevice::VolumeToAttenuation(i32 value) { ... }

RVA(0x00135110, 0x8e)
i32 ConvertVolumeToPercent(i32 v) { ... }

#pragma optimize("", on)
```

`#pragma optimize` applies to every function *defined after* it, so put the `off`
above the first stuck function and the `on` above the first neighbour that is
already matching. clang (the label pass) ignores the pragma; only cl acts on it.

## Measured

`src/Dsndmgr/DirectSoundMgr.cpp`, both functions carried `@early-stop` notes:
`ConvertVolumeToPercent` (0x135110) **33.96 -> 100.00 EXACT** from the pragma alone;
`VolumeToAttenuation` (0x1350b0) **58.11 -> 90.30** from the pragma, then **100.00
EXACT** once its `double t = ...` local was inlined into the one expression (under
/Od every named local really does get a stack slot, so an extra local you invented
for readability now costs an `fstp`/`fld` pair and shifts every `[ebp-N]`).
`BuildVolumeTable` (0x1351a0), the first function past the `on`, kept the same
optimized instruction bytes. Its `g_volumeTable + 0x190` bound was later proved
to be element 100 of the 101-entry table, not a neighbouring global; see
`folded-base-address-names-the-neighbour.md`.

## Corollary

Inside an `off` region, **source shape maps 1:1 to code**: every named local is a
stack slot, every sub-expression is evaluated in written order. That makes these
functions far EASIER to match than /O2 ones - but only if you stop writing
"readability" temporaries the original did not have.

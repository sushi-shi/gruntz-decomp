# Which `#pragma`s are load-bearing, and why

Measured 2026-08-08 by disabling each pragma family tree-wide and diffing all 4301
per-function fuzzy scores against a control build. Flags are `/nologo /c /O2 /MT`
(+`/GX`, +`/GR` per unit) — see `config/units.toml`.

| pragma | count | functions moved | verdict |
|---|---|---|---|
| `intrinsic(...)` | 31 | **0** | DEAD — `/O2` implies `/Oi`. Deleted. |
| `inline_depth(...)` | 4 | **0** | DEAD — cl 5.0 ignores it. Deleted. |
| `function(memcpy)` | 2 | 4 | REQUIRED |
| `optimize("", off/on)` | 6 (3 pairs) | 6 | REQUIRED |
| `pack(push,N)/(pop)` | 18 | 6 (0.01–0.09 each) | keep — semantic |
| `once` | 1 | 0 | cosmetic |

## Why the two survivors cannot be a compiler flag

Both were tested against the flag that would replace them. Neither substitutes.

**`optimize("", off)` — proven by RVA interleaving.** The six /Od functions sit
*inside* a compiland's contiguous span, bracketed on both sides by optimized
functions of the same unit (`CDDSurface::DecodeRun8` 0x140aa0 and `DecodeRun24`
follow a caps-logging function and precede `RotateBlit` 0x141040). A compiland has
one flag set, so no per-TU flag can produce an unoptimized island inside it. A
source-level, function-scoped mechanism is the only possibility, and this is it.
Retail really does contain unoptimized code here — remove the pragma and the six
go from 100.00% to 10–58%.

The option string is **not** determined: `("", off)` and `("g", off)` produce
byte-identical output at all three sites; `("t", off)` and `("s", off)` do not
(DecodeRun8 19.80 / 10.42). So the disabled optimization is `/Og`; whether Monolith
wrote the narrow or the broad form is unrecoverable.

**`function(memcpy)` — proven by the `/Oi-` experiment.** Retail's relocation names
`_memcpy` (not `_memmove`), so the source really called `memcpy` and cl declined to
inline it. Setting `/Oi-` on the two TUs kills intrinsics for *every* function in
them: `zBitVec::SetSize` 100 → 69.89, `CVariantSlot::Set` 100 → 70.46,
`_zvec::_zvec` 100 → 80.24. The TU needs intrinsics ON everywhere except `memcpy`,
which is exactly what the pragma expresses and a flag cannot.

## The /Od sweep

A tree-wide scan of the delinked target objs (prologue is `push ebp; mov ebp,esp`
AND ≥5% of instructions carry an `[ebp-N]` operand) finds **exactly the six known
functions and no others**, so the unoptimized region of retail is fully accounted
for. Separation is clean: /Od functions score 0.13–0.63 on the ratio, optimized
controls in the same units score 0.00–0.02.

Two candidates it surfaced are **false positives**: `FillPolygon` (68.33) and
`WarpTextureBlit` (71.51) in `imagepolyclip` carry an ebp frame with ratio
0.23–0.25, but so does our base (0.21 / 0.24) — they are ordinary optimized
float-heavy code that keeps a frame pointer, not `/Od`. Base-vs-target agreement is
the discriminator; ebp-frame presence alone is not.

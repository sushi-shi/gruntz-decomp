# String & FP-constant pooling — VC5 mechanics (measured 2026-08-04)

How `cl` 5.0 / `link` 5.10.7303 (the retail toolchain) treat literal constants,
established by compiling test TUs under wine and linking with retail's linker
(experiment: two TUs sharing a string literal, a double and a float, over
`/O2`, `/O2 /Gf`, `/O2 /GF`, `/Od`, `/Od /Gf`, C and C++, `/GX /GR` on and off).

## Strings: compiler AND linker, two halves of one mechanism

- **Compile half (`/Gf`, implied by `/O2`):** every *unique* literal in the TU is
  emitted as its own section — **writable `.data` COMDAT, selection `Any`**, named
  by content: `??_C@_0BE@CHKD@hello?5pooling?5world?$AA@`. Equal literals within
  one TU collapse to that one COMDAT *at compile time* (the name is derived from
  the content, so there is nothing left to merge). Same behavior in C and C++,
  with or without `/GX /GR`.
- **Link half:** identical literals across TUs carry the *same* `??_C@` name, and
  the linker's `Any` COMDAT selection keeps ONE copy — the **first contribution
  in link order** — and folds the rest onto its address (verified in the `.map`:
  the surviving symbol is credited to the first obj on the link line). So yes:
  every equal string, same TU or not, ends at one image RVA.
- **No sub-string merging:** `"pooling world"` gets a full separate copy even
  though it is a suffix of `"hello pooling world"` (VC5 never overlaps tails).
- **`/Gf` explicitly is a no-op at `/O2`** — output byte-identical modulo the COFF
  timestamp. It only matters at `/Od`, where the default is NO pooling at all:
  each literal *occurrence* becomes a `$SG<n>` static in a plain `.data` section
  (two occurrences in one TU = two copies; nothing folds at link).
- **`/GF` = `/Gf` + read-only:** the literal COMDATs move to `.rdata`. Retail's
  literals live in **writable `.data`** → retail was built *without* `/GF`
  (`/O2 /MT`, pooling via the implied `/Gf`). This pins the flag harder than the
  old "no effect" note in `zlib-matching.md`.
- **Retail effect:** a whole-image scan finds essentially zero duplicated strings
  (1 hit ≥10 chars, an import hint-name collision) — pooling was fully effective.
  Consequence for src (`pooled-string-literals-one-owner` memory): a pooled
  literal has exactly ONE owner TU (first in link order); other TUs' identical
  literals are the same datum and can never take their own `DATA` pin.
- **Dev-hoisted strings are NOT pooled literals:** where the original code
  hoisted literals into named file-scope `static char[]` arrays (visible as
  per-TU string runs at the head of a TU's data band), each array is its own
  named static datum — never folded with a `??_C@` literal or with another TU's
  copy, and pinnable per-TU. The likely motive: a named static dedups the string
  even in `/Od` (debug) builds, where literal pooling is off.

## FP constants: per-TU, never pooled

- Float/double literals compile to a **single plain (non-COMDAT) `.rdata`
  section per TU** — the TU's FP constant pool — holding `$T<n>` **static**
  symbols (`$T<n>` is cl's generic compiler-temp name; with `/GX` it also names
  `.xdata$x` EH tables — don't conflate).
- Within a TU, equal FP constants dedup (even at `/Od`). Across TUs they
  **never fold**: no COMDAT, no external name, so the linker keeps every TU's
  pool. Linked proof: two TUs using `0.1234567891234` and `1.5f` → both constants
  appear TWICE in the exe, once per TU's pool. Retail agrees: the double `0.5`
  sits at 4+ distinct aligned RVAs in GRUNTZ.EXE.
- Consequence for data attribution: an FP constant is a *per-TU* datum — a
  duplicated value at two RVAs is two TUs' pools, each independently ownable
  (unlike pooled strings). The `__real@…` naming sometimes used in notes is a
  VC6+ convention; VC5 objs have only static `$T<n>`.

## Flag summary

| Flags            | String literals                                  | FP constants        |
| ---------------- | ------------------------------------------------ | ------------------- |
| `/Od`            | `$SG<n>` statics, one per *occurrence*, no fold  | per-TU `$T` pool    |
| `/Od /Gf`        | `??_C@` writable-`.data` COMDATs, fold at link   | per-TU `$T` pool    |
| `/O2` (retail)   | same as `/Gf` — implied                          | per-TU `$T` pool    |
| `/O2 /GF`        | `??_C@` COMDATs in **`.rdata`**                  | per-TU `$T` pool    |

Retail = `/O2 /MT` (strings in writable `.data` ⇒ no `/GF`); the build's
`cflags` need no change.

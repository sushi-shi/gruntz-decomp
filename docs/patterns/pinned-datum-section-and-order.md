# A pinned datum's retail SECTION proves its const-ness, and its retail RVA proves its declaration order
tags: cpp:global cpp:const | topic:data topic:codegen-idiom
symptoms: `.data`/`.rdata` section stuck far below 100 while every payload is present; objdiff `data_diff` shows one long `DIFF_DELETE` run whose bytes also appear in a `DIFF_INSERT` later; `static const char s_x[] = "..."` under a `DATA()` pin in the writable range; a `double`/`float` global pinned into `.rdata`; `labels ... MISS <rva> data candidate not in base obj`
confidence: 10/10

Two facts about a `DATA(rva)` pin are load-bearing for the data score and are
often wrong in a reconstruction, because neither is visible in the disassembly of
any function.

**1. The section the RVA falls in states whether the definition was `const`.**
For `GRUNTZ.EXE` the boundaries are `.rdata` = `0x001e7000..0x00207fa8`
(`IMAGE_SCN_MEM_READ` only) and `.data` = `0x00208000..0x00229400` initialised,
`..0x002c27ac` for `.bss`. cl 5.0 sends a `const` file-scope definition to
`.rdata` and a non-`const` one to `.data`, so a mismatch means cl emits the
object into a section the delinked target does not have and the whole payload
scores zero on **both** sides.

```cpp
// WRONG - pin lands in retail .data (writable), so retail's TU had no `const`
DATA(0x0020e924)
static const char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";

// WRONG - pin lands in retail .rdata, so retail's TU DID write `const`
DATA(0x001efb40)
float g_one = 1.0f;
```

Two traps when adding `const`:

* a namespace-scope `const` has **internal linkage** in C++, so a cross-TU
  global needs `extern const T x;` in its owning header and a plain
  `const T x = v;` definition (writing `extern` on the definition trips the
  `cpp extern decls` ratchet);
* an internal-linkage `const` with **no reader in its own TU is discarded**, and
  the pin then silently drops - `labels` prints
  `MISS <rva> [unit] data candidate not in base obj`. Give it the header
  declaration so cl must emit it.

A datum pinned into `.data` whose initialiser is zero also lands in `.bss`;
that combination means the recorded value is wrong (`g_defaultProjActSize` is
32 at `0x0021ad28`, `g_defaultZ` is 24 at `0x001f04e8`).

**2. Ascending retail RVA within a unit IS the original declaration order.**
The delinker packs a unit's ordinary (non-COMDAT) data in ascending RVA;
cl emits it in source declaration order; objdiff diffs the two combined streams
byte-wise with an LCS. Extra bytes on the base side are cheap, but a
*transposition* costs the whole smaller block. So sorting the `DATA()`-pinned
definitions in a TU by RVA recovers the retail order and closes the transposition
- measured `gruntentrancemove .data` 37.6% -> 88.3% from ordering alone, and
`mainmenubuilder` 47.4% -> 89.3%.

The audit for (1) is a per-unit comparison of `build/gen/delink_data_manifest.tsv`
(`storage` column, derived from the retail RVA) against the section each symbol
actually landed in inside `build/objdiff/base/<unit>.obj`.

**What this does NOT fix.** The residue after both corrections is delinker
layout, not source: the target packs with `data_manifest._alignment(rva, size)`
(a power of two that divides the RVA *and* the size) while cl aligns a
file-scope object to 4/8, and the delinker gives its `.bss` symbols no COFF
size so objdiff infers each one's extent from the next symbol. Recognise those
two and stop - see `topic:scoring-artifact`.

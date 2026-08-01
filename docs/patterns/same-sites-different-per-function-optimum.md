# The same source sites have a DIFFERENT optimum in each function - score the product, never a ladder

**Tags:** `cpp:local` `cpp:struct` | `topic:tooling` `topic:codegen-idiom`
**Confidence:** 9/10

## The claim

When several functions share a source idiom (a copied-and-adapted loop, a family of
overloads), the spelling that matches retail in one of them is **not** the spelling that
matches retail in the others. The choice is decided by each function's own register
pressure, so it has to be *measured per function*.

And the corollary that actually costs you time: a **sequential** edit-compile ladder
cannot find these spellings at all, because it cannot distinguish

* "wrong idea" from
* "right idea, wrong partner" (a change that only pays combined with another), and
* "right idea, wrong spelling" (a change whose family contains a winner it is not).

Both of those look identical from a ladder: a regression.

## Measured

`src/Font/Font.cpp` has three word-wrap bodies (`MeasureWrapped` 0x17ad10,
`DrawWrapped` 0x17a460, `LayoutWrapped` 0x17b120) that are near-copies of one greedy
break loop. Five sites were enumerated in each: how the outer `MeasureText` extent is
read, whether the word scan and the newline test call `text.GetLength()` or reuse the
outer `len`, how the head extent is read, and which length feeds `Right()`. Cartesian
runs (192 / 96 / 48 cells) gave **three different winners over the identical five
sites**:

| function | first extent | head extent | `Right()` count |
|---|---|---|---|
| `MeasureWrapped` | whole-struct copy | whole-struct copy | fresh `GetLength()` |
| `DrawWrapped`    | read inline      | read inline      | outer `len` |
| `LayoutWrapped`  | read inline      | whole-struct copy | fresh `GetLength()` |

Propagating `MeasureWrapped`'s answer onto `DrawWrapped` by hand - the obvious move,
since they are the same code - took `DrawWrapped` **75.2 -> 72.8**. Its own matrix put
it back at 75.2 with the opposite spelling at two of the three sites.

## The two ladder traps, both real

**Right idea, wrong partner.** `FontRenderer::MeasureText` (0x17ac50): dropping the
redundant `i = 0` from the `for`-init tested **71.8 -> 71.6** by hand, and reads as
refuted. It is not: the 48-cell matrix winner is that change **crossed with** writing
the null-path extent stores height-before-width, at 72.5. Retail coalesces the loop
counter's initial `0` with the constant `0` those stores need (both live in `esi`), so
neither half is worth anything without the other.

**Right idea, wrong spelling.** `Font::AllocateMemory` (0x179720): retail loads
`m_glyphs` once per iteration (`0x17977a mov edi,[esi+0xc]`) where we reloaded it
between the two metric stores. Hand-testing the obvious fix, `Glyph& g = m_glyphs[i];`,
gave **92.1 -> 76.2** - apparently a refutation of the whole single-load reading. The
12-cell matrix carried five spellings of that same idea, and one of them,

```cpp
Glyph g;
g.width = 0;
g.height = 0;
m_glyphs[i] = g;      // 99.8; every member-store spelling scores 91.9
```

took the function to **100.00 EXACT**. A whole-struct assignment is also what explains
retail materialising the zero *twice* (edx for the surface slot and the height, ecx for
the width) - the residue of constant propagation through a struct copy.

## How to run it

Enumerate every suspect SITE and every legal SPELLING per site, put the whole family in
one manifest, and score the product in one run:

```bash
python -m gruntz.permute.batch_source_variants config/axes/<fn>.json --top 10 --output /tmp/<fn>-out
```

Use `batch_source_variants` directly when the axes are hand-authored - it runs only the
manifest. (`match_variants --axes-from` also emits AST mutations, which can overlap a
hand-authored axis and abort the run with `axis <name> overlaps an edit in candidate ...`;
`--max-depth 0` suppresses them where the flag is supported.)

`find` spans must be **globally unique in the file**. In a file of near-identical
siblings there may be no unique line in the region you care about - anchor the axis on
the function signature and fold that region's sub-site product into that one axis's
options in Python. The product is preserved; only the axis boundaries move.

## When it does NOT pay

The product is only worth running over sites you have reason to suspect. Three negative
results from the same campaign, both after the control flow was already proven identical
(`sema disasm --branches --diff` reporting AGREE):

* `SoundStream::ParseWave` (0x137b70, 99.82%) - 72 cells over the declaration order,
  merged-vs-separate declarations, and scoping of every local. **Every cell scored
  identically to baseline.** The residual is `sub esp,0xc` vs our `0x10`: retail packs
  one of the four locals into the dead `src` argument home slot. That is an MSVC frame
  packer decision, not a declaration-site one.
* `BuildLevelTitleString` (0xe44e0, 95.78%) - 24 cells over buffer declaration order, an
  extra 4-byte scalar before/after the buffers, and hoisting the `CFile` above the null
  guards. **All twelve `cfile=below` cells scored identically** (95.45), so neither the
  array order nor an added scalar perturbs the frame at all; all twelve `cfile=top` cells
  crater to 77.02 because hoisting the declaration also hoists the constructor past the
  guards, which is simply wrong. Retail's `CFile@+0x10 / flag@+0x20 / CString-temp@+0x24`
  ordering versus our `flag@+0x10 / CString@+0x14 / CFile@+0x18` is therefore *also* the
  frame packer: MSVC lays the destructible-object block out in source order, and retail's
  order cannot be produced without moving the constructor.

* The four Font wrap bodies each carry exactly one extra branch - cl rotates the inner
  per-char loop and duplicates the `y < bottom` test where retail's back-edge lands on
  it. `while`, `for(;;)` + top `break`, and `do/while` compile **byte-identically**, so
  the loop keyword is not the axis; do not spend cells on it.

**The shape of the negative result matters.** In both cases `sema disasm --branches --diff`
already reported AGREE and the block skeleton already matched, so what was left had to be
either instruction selection or frame/register allocation. The matrix is how you tell those
two apart: if *every* cell over the declaration sites scores identically, the sites are not
the lever and the residual is the allocator. Record it - the next lane otherwise re-authors
the same matrix.

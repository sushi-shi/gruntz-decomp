# When the compiler owns the name, stop matching on names

**The revelation:** some data has no source spelling and no stable symbol — the
compiler invents both. A float constant in an expression becomes a pool entry
called `$T36166`; that number renumbers on any edit to the translation unit. You
cannot name such a datum, and you must not try: **the identity is the CONTENT.**

So `DATA_COMPGEN` does not label anything. It states a retail ADDRESS beside a
literal, and the pipeline proves the pairing by comparing **bytes** — on both
sides, independently — then masks the volatile name away so the two objects
content-address identically.

This is the opposite discipline from `DATA()`, and conflating them is what makes
compiler-generated data look unreachable.

---

## Why `DATA()` cannot reach it

`DATA(rva)` binds a retail address to a **declaration**: the macro text is
scanned and joined to the AST `VarDecl` below it, and the resulting mangled name
is authority-checked against the base object. That needs a declaration to exist.

A float constant in an expression has none:

```cpp
if (dist >= mag * 0.9) { ... }
```

There is no variable here. cl still allocates eight bytes of `.rdata` for `0.9`,
puts them in the TU's floating-point pool, and calls them `$T36166`. Nothing in
the source can carry `DATA()`, because nothing in the source *is* the datum.

`RVA_COMPGEN` does not help either — that pins compiler-generated **code** by
giving its mangled name verbatim (`??_G` scalar-deleting destructors and the
like). Pool entries have no mangled name to give.

---

## What `DATA_COMPGEN` actually is

```c
#define DATA_COMPGEN(addr, value) value
```

It compiles to nothing. It is a **claim written at the use site**:

```cpp
// src/Gruntz/Grunt.cpp
flash = static_cast<i32>(frac * frac * DATA_COMPGEN(0x001e9a40, 750.0));
```

read as: *retail address `0x001e9a40` holds the constant `750.0`.* There is no
source-side name: cl's pool name is volatile, and an address-derived alias such
as `fp_1e9a40` would only repeat the coordinate while pretending to add identity.

Since the retail-reloc FP oracle landed (`data_manifest.fp_pool_rows`), a claim is
written **only for a slot the oracle cannot reach** — one with no reloc-corroborated
referrer yet. Every oracle-covered slot goes bare; the 2026-08-13 reconciliation
removed 41 such pins with zero movement (`docs/data-attribution.md` §3b-iii).

### The generated symbol name is derived, never supplied

The emitted symbol is minted from the address alone:

```python
name = "$T%d" % rva
```

so `DATA_COMPGEN(0x001e9a40, …)` lands in `symbol_names.csv` as

```
0x1e9a40,$T2005568,grunt,0x8,data               # 0x1e9a40 == 2005568
```

a cl-shaped pool name the delinker uses to carve the datum — and which
`VOLATILE_T` (`^\$T[0-9]+$`) then erases on the way to `$anon_f64_…`, exactly as
it erases our own `$T36166`. Both sides converge because both volatile names are
discarded. Identity is established by address, owning TU, type, and payload bytes;
an extra free-form source name contributes no evidence.

**The value's SPELLING is the allocation's type**, and that is load-bearing:

| written | means | payload |
| :-- | :-- | :-- |
| `0.9` | an **f64** pool entry | `struct.pack("<d", 0.9)` |
| `1.4f` | an **f32** pool entry | `struct.pack("<f", 1.4)` |
| `"Wormhole"` | a pooled `??_C@` literal | the narrow bytes |

Integers are **rejected**: an integer literal lives in the instruction stream as
an immediate, not in `.rdata`. So is anything else — identifiers, wide strings —
because the tool must know exactly how many bytes to compare.

---

## The proof, both sides, on real bytes

**Retail side.** Read the image at the claimed address:

```
750.0 as f64 little-endian : 00 00 00 00 00 70 87 40
retail @0x001e9a40         : 00 00 00 00 00 70 87 40     ✔
```

And the adjacent pair from `Grunt.cpp`, two claims on two consecutive f64 pool
slots:

```
retail @0x001e9740 : 00 00 00 00 00 00 f0 bf | 00 00 00 00 00 00 59 40
                     ^^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^
                     -1.0                      100.0  ( = DATA_COMPGEN(0x001e9748, 100.0) )
```

Two independent `DATA_COMPGEN` claims land on two consecutive slots, and both
decode. That is not a coincidence you can arrange by accident.

**Our side.** The claim is authority-checked against the base object cl actually
produced from this source — never assumed:

* a **string** value must equal a `??_C@` COMDAT payload in that object; cl's own
  spelling for those exact bytes *is* the emitted name;
* a **float** value's bits must occur in that object's section bytes — the TU's
  `$T<n>` pool.

A claim that cannot find its bytes on our side is an error, not a silent skip.

---

## How the two objects come to share a name

objdiff pairs symbols **by name**. Our object calls the constant `$T36166`; the
delinked retail object calls it something else entirely. Neither name is usable,
so the normalizer **renames both sides to a name computed from the content** —
and they collide deliberately.

The name *is* the bytes:

| kind | minted name | identity |
| :-- | :-- | :-- |
| f32 pool entry | `$anon_f32_<8 hex>` | the 32-bit value, little-endian |
| f64 pool entry | `$anon_f64_<16 hex>` | the 64-bit value, little-endian |
| pooled string | `$anon_str_<sha256>` | the NUL-terminated payload |
| anything else | `$anon_data_<sha256>` | a structured record (below) |

plus an `_<occurrence>` suffix so two identical payloads in one object stay
distinct.

Run it on `projectile.obj` and the two independently-produced objects agree
exactly:

```
$ llvm-nm build/objdiff/normalized/base/projectile.obj   | grep '$anon'
$ llvm-nm build/objdiff/normalized/target/projectile.c.obj | grep '$anon'

  $anon_data_70f87a7cc3bbf7d0c06dcca4ce8e564f59550163e718822820e5740ddb7c5f77_0
  $anon_f64_3fb999999999999a_0     $anon_f64_3fd999999999999a_0
  $anon_f64_3fc999999999999a_0     $anon_f64_3fe3333333333333_0
  $anon_f64_3fd3333333333333_0     $anon_f64_3fe6666666666666_0
  $anon_f64_3fe999999999999a_0     $anon_f64_3feccccccccccccd_0
```

Both lists are identical — 9 symbols, same names. `0.9` is
`0x3feccccccccccccd`, so `$anon_f64_3feccccccccccccd_0` is *the* pool entry at
retail `0x001eaa98` — carried today by the retail-reloc oracle with no pin, its
bare literal spelled in `CProjectile`'s flight-distance `if` ladder alongside the
`0.1 / 0.2 / 0.3 / 0.4 / 0.6 / 0.7 / 0.8` siblings.

Before normalization our side of that list reads `$T36166 $T36170 $T36180
$T36182 $T36205`. Afterwards those spellings do not exist in the object at all.

### What goes into the digest, and what is deliberately left out

The generic `$anon_data_` record is a structured document, not a raw hash —
schema `gruntz-anon-symbol-v2`, carrying kind, storage class, span,
meaningful size, the payload **with relocation sites masked**, and the
relocation rows themselves. Masking matters: a relocated word's bytes are a
placeholder the linker overwrites, so hashing them raw would make two identical
objects disagree. The reloc *targets* participate in identity instead.

Three payload rules exist because two allocators are involved and only one of
them is ours:

* **`.bss` hashes NOTHING** (`meaningful = b""`). An uninitialised allocation
  states no bytes; its physical span is the object *plus* whatever hole-filling
  its allocator chose — cl packs 4-byte ints into the gaps, the delinker appends
  per definition. Neither quantity is part of the identity, so a BSS static is
  keyed on name, storage and occurrence alone.
* **`$E` text helpers strip trailing `0x90`** before hashing: base COMDATs and
  packed delinked text have different alignment spans, and the body ends before
  the padding.
* **A string keeps only up to its NUL**, not its aligned span.

## The part that makes it work: mask the volatile name

cl's pool symbols are numbered, and the numbers move. In `projectile.obj` today:

```
$T36166  $T36170  $T36180  $T36182  $T36205
```

Add a line to that file and they all shift. Matching on those names would make
every unrelated edit look like a data regression.

So normalization erases them. `canonicalize_data_symbols.py` treats a whole
family as volatile and content-addresses instead:

```python
VOLATILE_SG = re.compile(r"^\$SG[0-9]+$")     # string-pool ordinals
VOLATILE_T  = re.compile(r"^\$T[0-9]+$")      # floating-point pool ordinals
VOLATILE_E  = re.compile(r"^\$E[0-9]+$")      # dyn-init / EH cleanup funclets
NAMED_STATIC = re.compile(r"^.+\$S[0-9]+$")   # file-static decoration
```

After normalization, `grep '\$T[0-9]'` on the same object returns **nothing** —
the names are gone and the payloads are paired by content.

**Every** `$S<n>` counter in a name is volatile, not just the trailing one. c2
appends `$S<n>` to a TU-local static, and c1xx *also* spells the one-byte guard
of a function-local static as `?$S<n>@?1??Fn@@…@4EA` — an inner counter that
renumbers the same way. Both are masked, so a guard content-addresses on its
enclosing function rather than on a number. Proven collision-free: 13 symbols
tree-wide carry more than one `$S<n>` and none of them pair.

### Live proof that the ordinals really do drift

This project's own documentation, written earlier, records a real symbol as

```
_?s_ambientCoin@?4??GetAmbientId@CPlay@@QAEHXZ@4HA$S41910
```

The same symbol in the tree right now is

```
_?s_ambientCoin@?4??GetAmbientId@CPlay@@QAEHXZ@4HA$S42105
```

Nothing about that datum changed. 195 unrelated symbols were added or removed
between those two builds. Any pipeline that had pinned the name would now be
reporting a phantom regression — which is precisely why the ordinal is wildcarded
and a rewrite is accepted **only when exactly one object symbol matches**.

Note also `?4` where clang writes `?1`: VC5 numbers a local static's scope
ordinal by how many blocks it has already left, and prefixes `_` on top of the
C++ mangling. Two more reasons a name is the wrong key.

---

## The counter-example — when this is NOT the right tool

`DATA_COMPGEN` is for data the compiler invents from a literal **at a use site**.
It is the wrong instrument for two neighbouring cases, and reaching for it there
fabricates an owner:

* **A real datum with a declaration** takes `DATA(rva)` on that declaration. If
  it seems unreachable, the cause is usually a *binding* failure, not a missing
  mechanism — see the `$S`/`?4` rewrites above, and the macro-expansion bug that
  silently dropped six `?g_*` rows out of an entire TU.
* **Data whose real C++ definition lives in a HEADER** — a function-local static
  inside a header inline, plus the `??_B` guard byte beside it, which has no
  source spelling at all — has **no owning TU**. cl emits each as a COFF COMMON
  into every TU that instantiates the inline and the linker merges them, so any
  source position would invent an owner. Those live in a manifest,
  `config/retail/compiler-generated-data.tsv`, whose every row is re-proven
  against every base object's COMMON table on each build. That re-proof is what
  keeps it from being the retired declaration-only `DATA_SYMBOL` in a new coat.

---

## The general rule

Three kinds of thing need three different keys:

| the thing | key | why |
| :-- | :-- | :-- |
| a declared global | its **mangled name**, authority-checked in the base obj | source owns the name |
| a compiler-invented pool entry | its **bytes** | the compiler owns the name, and renumbers it |
| a folded COMDAT with no owning TU | a **manifest row**, re-proven per build | nobody owns the position |

**Corollary:** whenever an identifier is minted by the toolchain rather than by
the source, treat it as a *coordinate*, not a name. Mask it, address the content,
and require a unique match — otherwise you have built a pipeline that reports
churn as regression and hides real defects inside the noise.

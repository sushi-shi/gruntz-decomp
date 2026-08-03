# Incremental-link thunks as a TU / library oracle

**A reusable technique for any MSVC-era decompilation, not just Gruntz.** If the target
binary was linked with `/INCREMENTAL` (the default for a dev build, and very common in
shipped 90s games), the linker leaves a machine-readable record of **which translation
units were objects on the link line and which arrived from a static library** — and, by
extension, where the real compiland boundaries are. You can read it straight out of the
binary with no symbols, no PDB and no decompiler.

## The mechanism

MSVC's incremental linker wants to relink after an edit without re-fixing every call
site. So it puts a **jump table at the very start of `.text`** — a run of 5-byte
`E9 rel32` thunks — and routes calls through it. Move a function, patch one thunk, done.

Two properties make it an oracle:

1. **It only thunks objects on the link line.** A member pulled from a `.LIB` is not
   incrementally replaceable, so it never gets a thunk; callers `call` it directly.
2. **Within a thunked object it thunks essentially every call**, not just cross-object
   ones. (Measured on Gruntz: inside the thunked region, 6728 calls go through a thunk
   and exactly **1** is direct.)

So for each function you can ask *does a thunk jump to it?* — but **the test is
asymmetric, and getting this wrong will cost you**:

| answer | what it proves |
|---|---|
| thunk **present** | **proof** the function was compiled into a `.obj` on the link line |
| thunk **absent** | **nothing on its own** — either a `.LIB` member, *or* a function nothing calls directly |

A thunk exists per *directly called* function. An address-taken callback (a window
proc), a vtable-only virtual, or dead code gets no thunk even inside a link-line object.
At **unit** level absence is still strong evidence — a 118-function unit with zero
thunks is a library member, not 118 uncalled functions — but at **function** level it is
not evidence at all. In Gruntz this cost four false "defects" before it was caught:
`_BattlezMapComboEditProc@16` and `_MultiMapComboEditProc@16` (window procs, address
stored in `.text`, never called), plus two genuinely uncalled functions.

## How to read it

```python
# 1. the band: a run of `E9 rel32` at the start of .text
tg = set()
i = 0
while text_rva + i + 5 <= band_end:
    if data[text_off + i] == 0xE9:
        tg.add(text_rva + i + 5 + int32_at(text_off + i + 1))   # thunk target
    i += 5
# 2. classify every known function
thunked = fn_rva in tg
```

Finding `band_end`: walk 5-byte slots from the start of `.text` while they are
overwhelmingly `E9`, and allow a run of non-`E9` before giving up — the band is a
**reserved region with slack**, so it does not end at the last thunk. In Gruntz the
`E9`s stop at `0x44a8` but the band is reserved to `0x7c20`; real code starts after it.
(The same slack appears in the IAT — the incremental linker reserves growth room
everywhere.)

## What it tells you

### 1. Which code was a static library

Group your reconstructed TUs and count thunked vs non-thunked functions:

| pattern | meaning |
|---|---|
| every function thunked | the TU was a `.obj` on the link line |
| no function thunked | the TU was (almost certainly) a `.LIB` member |
| **mixed, thunked in the minority** | **your TU spans two real compilands** — the thunked ones are proven object-side |
| mixed, unthunked in the minority | usually **not** a defect — just uncalled functions in an object |

The thunked functions also occupy a contiguous *address region*: in Gruntz, thunk
targets span `0x7970..0x11c860` and stop dead, while game code runs to `0x1936e0`.
That line is the object/library divide, visible as a plain address threshold.

### 2. Where your TU boundaries are wrong

The "mixed" bucket is the payoff. A real compiland is entirely one or the other, so a
mixed TU is **provably** two compilands merged in your model, and the minority side
names exactly which functions are on the wrong side. This is an *independent* signal —
in Gruntz it agreed with 21 of 38 functions that had already been flagged by RVA
clustering, and found 17 more that clustering missed.

It is also a sharper diagnosis than "linker-pooled COMDAT": a `??_G`/`??1` destructor
or a small inline sitting in the object region while its class lives in the library
region means the COMDAT was **emitted into the link-line object that first referenced
it**, not merely "placed oddly".

### 3. Whether the split is a project boundary

Map the divide onto your source tree. In Gruntz, 227 of 237 `src/Gruntz` units are
object-side and the engine modules (DDrawMgr, Image, Bute, Rez, Wwd, Crypto, Dsndmgr,
DinMgr2, Font, Utils, vendored zlib) are library-side — matching the leaked
`C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,Gruntz}` layout, where the engine projects
built `.LIB`s consumed by the game's link.

## Reproducing it in your own relink

Archive the library-side units with the era's own `LIB.EXE` and link the archive instead
of the objects (`gruntz link --engine-lib`). Thunk count is then a **fitness metric** for
your partition — the closer your object/library split matches the original, the closer
your count gets:

| partition | `E9` thunks | vs retail's 2695 |
|---|---|---|
| everything as objects | 4559 | 1.69x |
| by source module | 2976 | 1.10x |
| by the thunk oracle, mixed TUs unsplit | 3242 | 1.20x |

Note the oracle partition scoring *worse* than the module guess is not a contradiction:
the mixed TUs are large, and leaving them whole on the link line thunks all of their
functions. **The count only converges once the mixed TUs are split**, which is precisely
what the oracle is telling you to do.

## Caveats

- **Byte-scanning for `E8`/`E9` produces false positives** (an `0xE8` byte inside another
  instruction or in data). Require the target to be an exact known function start.
- The technique needs `/INCREMENTAL`. If the target has no thunk band at the top of
  `.text`, it was a full link and this oracle is unavailable.
- A `/FORCE` in *your* relink makes MSVC silently ignore `/INCREMENTAL` (LNK4075), so
  you cannot reproduce the shape until your own link is clean.
- The divide tells you *object vs library*, not which library — several `.LIB`s look
  alike from here. Distinguishing them needs the members' placement order.

## Gruntz specifics

`pe.ILT_LO..ILT_HI = 0x1000..0x7c20` (reserved band), `E9`s to `0x44a8`, 2695 thunks,
2583 distinct targets, object/library divide at `0x11c860`. 243 units object-side,
89 library-side, **18 mixed** — the live worklist.

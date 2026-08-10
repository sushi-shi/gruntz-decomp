# The data-access map — what retail's code actually touches

`gruntz audit data_access_map` builds a persisted, queryable record of **every
instruction in retail `GRUNTZ.EXE` that references `.rdata` / `.data` / `.bss` /
`.idata`**, decoded for the byte range it covers, the access width, the
direction, and the addressing form.

## Why it exists — the score structurally cannot answer this

**We choose the extent of every datum we claim, so a too-small claim always
scores 100.** objdiff only ever compares what we told it to compare. Model
`float x` where retail has `struct { float x, y; }`, get the four bytes right,
and the section reports exact while `y` is silently unmodelled.

It runs the other way too. `?g_idleGeom@@3PAUBzGeomPair@@A` was an invented
object whose member order was `{m_y; m_x}`; both orderings give
X = 472,525,474,525 so X matched **by coincidence**, while the old Y ran
`0,101,98,146` against retail's `101,98,146,144`. It survived byte comparison
and was ultimately a phantom.

The map answers the question the score cannot: *which bytes does retail's code
actually touch, and how wide?*

Its counterpart is the **completeness** analysis, which computes from *our* side
which bytes no claim covers. Neither alone distinguishes padding from an
unmodelled field. Intersected they do:

| | touched by retail | untouched |
|---|---|---|
| **covered by a claim** | modelled | possibly over-claimed |
| **uncovered** | **unmodelled data** | **padding** |

## How the sites are enumerated — `.reloc`, not a disassembly

Every absolute operand in a PE is relocated, so the `.reloc` HIGHLOW table is a
**complete and reliable index of where the absolute data references are**. The
sweep walks that table and disassembles only *at* each site, to learn the width
and the form. That sidesteps needing a correct whole-image disassembly, and it
covers code we have never reconstructed — which is the point.

Two objdump passes over `.text` back the decode:

* **linear** — the section decoded from its start; desyncs on data-in-text.
* **anchored** — the same bytes with every byte *outside* a recovered function
  extent overwritten by `0x90`, so the decoder re-syncs exactly on each function
  start instead of drifting through an embedded jump table. `0x90` is one byte,
  so a function start stays on an instruction boundary whatever the gap parity.

A site inside a recovered function is decoded by the anchored pass, otherwise by
the linear one; whichever actually places the relocated operand in an instruction
wins. A site neither pass can place, and that lies outside every recovered
function, is **not an access at all** — it is a relocated pointer cell that lives
in `.text` (295 of them, a 16-byte-stride table at `0x18faa0`). Those go to the
`cell` table, not the `access` table.

## Forms — and the blind spot, stated honestly

| form | meaning | complete? |
|---|---|---|
| `direct` | `ds:addr` — the datum itself | yes, reloc-anchored |
| `indexed` | `[reg*s+addr]` — `addr` is a TABLE base, `s` a hard element-size witness | yes, reloc-anchored |
| `derived-disp` | `[reg+disp]` after a proven `mov reg,&sym` in the same block | partial, recovered |
| `lea` / `imm` | the address is taken; the object escapes, width unknown | yes, reloc-anchored |
| `indcall` | `call/jmp [addr]` — the cell holds a function pointer | yes |
| `iat` | an indirect call into the import table | yes |

**An access through a base register whose value did not come from an absolute
operand in the same basic block carries no relocation and no local provenance,
so it is invisible here.** That is:

* every `this`-relative field access inside a callee,
* every access through a pointer loaded *from* memory,
* every use of an address that escaped through a call argument.

`derived-disp` recovers only the single-block case, by a deliberately
conservative forward propagation: it stops at any control transfer, at any
branch target or function start, at any write to the base register (explicit
operand *or* implicit clobber such as `mul`/`div`/`stosd`), and after a 48
instruction budget. It never follows a register copy. A wrong provenance would
*invent* a data reference, which is worse than missing one.

`--build` prints the coverage accounting, including how many address escapes it
cannot follow and how many register loads are handed straight to a callee.

**Two further limits worth knowing:**

* For an `indexed` access the recorded range is the **base element only**. The
  table extends an unknown distance, so a touched range whose `forms` column
  contains `indexed` is a *lower bound*.
* A member **swap between two same-sized members** produces no width difference
  and is therefore invisible to an access map. The `g_idleGeom` bug is only
  detectable this way when the swapped members differ in size or in float-ness;
  otherwise it needs *value* evidence.

## Artifacts

| path | what |
|---|---|
| `build/gen/data_access_map.sqlite` | the query index (tables below) |
| `build/gen/data_access_map.tsv` | the access table, grep-able and diffable |
| `build/gen/data_touched_ranges.tsv` | coalesced byte ranges retail touches — the completeness lane's input |

### Schema

```
access (insn_rva, insn_len, mnemonic, site_rva, target_rva, width, end_rva,
        rw, form, base_reg, index_reg, scale, disp, fpu, ext, origin,
        fn_rva, fn_name, fn_unit, sym_rva, sym_name, sym_off, in_extent, text)
cell   (site_rva, target_rva, kind, where_sec, sym_rva, sym_name, sym_off,
        tgt_sym_rva, tgt_sym_name, tgt_sym_off)
claim  (rva, name, unit, type, section, extent, extent_src, sect_pct,
        n_access, n_read, n_write, n_addr, n_cells)
field  (sym_rva, off, size, path, type, is_ptr, is_float, resolved)
finding(category, severity, sym_rva, sym_name, addr, detail, evidence)
meta   (key, value)
```

`width` is the byte count the instruction touches — `0` means the instruction
takes the *address*. `fpu` records the x87 operand kind (`f32`/`f64`/`f80` for
`fld`/`fst`, `i16`/`i32`/`i64` for `fild`/`fist`), which is how a `double`
declared as a `float` is caught. `scale` is the index scale, which is element-size
evidence: `[base + i*8]` proves an 8-byte element whatever we declared (see
`docs/patterns/2d-array-codegen-signature.md`). `field.resolved = 0` marks a
member whose declared type could not be sized — its extent is inferred from the
next field's offset and **no verdict fires through it**.

### Query surface

```
gruntz audit data_access_map --build                  # sweep and persist
gruntz audit data_access_map --at 0x2448d0            # everything touching one address
gruntz audit data_access_map --range 0x245508:0x245520
gruntz audit data_access_map --symbol g_sfDeviceId    # claim dossier: field map + every access
gruntz audit data_access_map --fn StepCompassMove     # every data access one function makes
gruntz audit data_access_map --findings width         # the derived worklist
gruntz audit data_access_map --touched                # the coalesced touched-byte ranges
gruntz audit data_access_map --sql "SELECT ..."       # raw sqlite
```

## The five derived categories

| category | question | signal |
|---|---|---|
| `unclaimed` | accessed but no `DATA()` claim covers it | unmodelled data |
| `unaccessed` | claimed but nothing in the image reads, writes, addresses or points at it | phantom candidate |
| `width` | access width disagrees with the declared field | wrong type |
| `stride` | an index scale inside a claim disagrees with its element size | wrong element size or a missing dimension |
| `adjacent` | one access spans two claims, or both are reached from one base register | one object, not two |

`unclaimed` runs are triaged before they reach the worklist, each class counted
separately in the build summary so nothing is silently dropped: `string-pool`
(printable, read-only — pooled literals reached by an inlined `strcmp`),
`fp-pool` (x87-only, read-only — an unpinnable constant pool), `library` (every
accessor is carved library code), `idata` (import thunk slots, the linker's).
Only `data` is the worklist.

## Suppressed false-positive classes — each measured, each named

These are not noise filters. Each is a codegen idiom that was *observed* to
produce a wrong verdict, and each is counted in the build summary
(`width-skip-*`, `stride-skip-*`) so it can be re-argued.

* **`width-skip-byte-buffer`** — a wider access on a byte **array element** is
  the inlined CRT block move/compare (`rep movsd` over a `char[]`). A byte
  **scalar** read four bytes wide is still reported: that is a type error, not a
  block op.
* **`width-skip-dword-pair`** — MSVC 5.0 copies an 8-byte constant with a pair
  of dword loads, so 4-byte accesses at `+0` and `+4` of a `double` are the copy,
  not a layout bug. `?g_movingLogicMin@@3NB` is the canonical case: 13 dword
  reads at `+0` and 13 at `+4`, never an `fld`, and the `double` is correct.
* **`width-skip-vptr`** — `structs.json` systematically omits the vptr of a
  polymorphic class, so an access before the first declared field is *our* blind
  spot, not a layout defect. It omits the **MI-secondary** vptrs too, which do
  land between declared members: an instruction that writes a `??_7…` address
  into the object is a vptr *stamp*, at whatever offset. `?g_buteTree@@3VCButeTree@@A`
  `+0x8` is the canonical case — `mov DWORD PTR ds:0x6bf628,0x5f04dc` looked like
  an unmodelled pointer member in the hole between `m_errSink` (+4) and
  `m_teardown` (+12), and is `??_7CButeTree@@6BzPtrColl@@@`, already catalogued in
  `config/retail/vtables_game.csv`. Holes that are *not* vptr stamps are still
  reported.
* **`width-skip-and-mask`** — cl 5.0 loads a narrow global with a **full-width**
  read and masks the register (`mov edx,DWORD PTR ds:g_idx` … `and edx,0xffff`),
  because `movzx` was slow on the Pentium. Caught two ways, because the mask can
  sit past a branch or behind a register copy: positively, by finding the `and`
  within a short single-block window (recorded as `ext = m2`/`m1`); and by its
  store side — *nobody writes a 4-byte object only 2 bytes at a time*, so a
  read-only wide access over a field that is stored at its declared width is the
  same idiom. `?g_idx_64da80@@3GA` and `?g_sfDeviceId@@3GA` are both `u16` and
  both correct; each takes four such reads inside `SFManager_SelectBestDevice`.
* **`width-skip-unresolved`** — the declared type could not be sized. Accusing
  through a type we cannot read would be a fabricated finding.
* **`width-skip-negative-addend`** — an `indexed` access whose reloc target sits
  in the last few bytes *before* the next claim is `[reg + &next - k]`, the
  negative-addend spelling of a 1-based index into the FOLLOWING array (the same
  idiom `assert_relocs` knows). `?g_sfRouterId@@3KA` (DWORD, 0x24df9c) looked
  like a char buffer at `+3` because `BuildSoundFontPath` scans `?g_sfDir@@3PADA`
  (0x24dfa0) backwards as `cmp BYTE PTR [ecx+0x64df9f],0x5c`.
* **`stride-skip-subelement`** — `[i*4 + base + k]` inside a 12-byte record:
  MSVC scales the index by the dword, not by the element. Benign. A scale
  *larger* than the declared element is the opposite and is reported `high`.

**A narrow access only indicts the declared width when the narrow access is
itself a STORE.** `mov cx, WORD PTR ds:g_rUp` is `(u16)g_rUp`, not evidence of a
`u16` field. Getting this wrong produced seven false positives before it was
fixed; the check now asks the question of the narrow access, not of the offset.

## Calibration and the self-test

`--calibrate` runs the sieve over a **control set**: claims that live in a data
section objdiff scores at exactly 100.0, whose declared type fully resolves, and
that retail actually accesses. Their bytes are byte-identical to retail.

Read the result carefully. **A section at 100.0 does not make a `width` finding
a false positive** — the bytes match, the *type* can still be wrong, and that is
the entire premise. What the control set gives is a bound on noise: a sieve that
flags a large fraction of byte-exact, fully-typed claims is measuring its own
bugs. Two such bugs were found exactly this way (the narrow-read/store confusion
above, and an array-flattening cap that made every offset past 2048 look
unmodelled).

The control set is a bound on noise and **nothing more** — it cannot see a
false-positive class that only fires outside it. All three `width` findings the
sieve shipped with were false positives, and none of them was in the control set:
the two `u16` globals score 96.00 and `?g_buteTree@@3VCButeTree@@A` is untyped
storage. They were caught by reading the retail instruction stream around each
site, which is what adjudicating a finding actually costs. **`width` is 0 today,
and a zero here is a claim to re-verify, not a result to trust.**

`--selftest` plants known defects into the in-memory claim set — `src/` is never
touched — and requires the sieve to report each. **A sieve that returns 0 rows
because it is blind is indistinguishable from a sieve that returns 0 rows
because the tree is clean.** The injections are `narrow`, `widen`, `float`,
`swap`, `halve`, `stride`, `split` and `phantom`, one per detector.

## The `undercount` class — a too-small array COUNT (`float[2]` for `float[10]`)

The `stride` detector checks an indexed access's *element width* against the
declared element; it never checks the *count*. So a claim declared with the
right element but too FEW of them — `i32[1]` where retail iterates 101 — passes
every width branch (`scale == elem`) and was invisible. `undercount` closes that:
**a claim declared with exactly one element that retail reaches through a scaled
index is under-declared**, because you do not index a single-element array with a
variable — the index itself proves a length ≥ 2 the extent does not carry.

The instance that motivated it is `?g_panTable@@3PAHA`, declared `i32[1]` at
`0x253c48`. `DirectSoundMgr::SetPanByIndex` reads `g_panTable[-idx]` for idx
0..100, i.e. it is a **backward cursor into `g_volumeTable`'s tail**
(`g_volumeTable[100]` ends exactly at `0x253c48`, so `g_panTable[-k]` is
`g_volumeTable[100-k]`). This one is genuinely byte-correct: the next retail
symbol is `_g_ssLogEnabled` at `0x253c4c`, **4 bytes on**, so retail's own cl
reserved a 4-byte `.bss` slot here — declared size *equals* the retail
inter-symbol gap. It is the exceptional case, not the rule.

**A too-small `.bss` array is NOT generally byte-neutral.** When the array has
its own forward storage, under-declaring its COUNT makes cl emit a smaller `.bss`
COMDAT/COMMON and places every subsequent symbol earlier — the emitted COFF and
the linked `.bss` both differ. It only *reads* neutral through two specific
masks, and the fix in each case is different:

1. **objdiff infers `.bss` size from the next symbol.** `obj/read.rs` sets a bss
   symbol's size to the next symbol's offset, and the size-inference patch
   compares sizes only when a side states one. So as long as the symbol SET and
   ORDER agree, a wrong declared size is invisible *to objdiff* — but the linked
   image is not. The oracle that does bite is **declared extent vs the retail
   inter-symbol gap**: `g_panTable`'s is 4 == 4 (neutral); a genuine truncation
   has gap ≫ declared, and `build_claims` already records that direction as
   `declared-overlap`. The reverse (declared < gap with the tail ACCESSED) is a
   real under-reservation.
2. **An indexed access folds to offset 0.** `[reg*scale + base]` is recorded at
   `target_rva = base`, `sym_off = 0`, `in_extent = 1`, so the per-symbol overrun
   check sees the one access that PROVES array-ness as a legal read of element
   [0]. Count evidence can therefore only come from the *form* (indexed at all),
   which is why `undercount` keys off `form='indexed'`, not a past-the-end target.
3. **The note on the neighbour was mistaken for a review of the datum.**
   `VolumeScale.h` (an earlier *agent* pass, not a human review) documented the
   adjacent `g_volumeTable` overrun in detail and treated `0x253c48` only as that
   loop's *terminating address* — it never asked what `g_panTable`'s own `[1]`
   meant. A reviewed note on one datum is not a review of the next.

## The `shortfall` class — the same defect with COUNT ≥ 2

`undercount` fires only for declared count ≤ 1, the sole case an indexed access
(which folds to offset 0) can settle. For an array declared with **2 or more**
elements that retail walks one or more elements too far, the evidence is a
**direct same-width access immediately past the declared end, by a function that
also accesses the array body** — the same loop over-running. `shortfall` reports
exactly that: `prev` is an array, the tail run starts at `prev.end`, the run's
width equals the element size, the section matches, and the tail's accessing
functions intersect the body's. Every one of those clauses is load-bearing —
dropping the last two turns two correct arrays (`g_levelMsgStrings[8]`,
`g_ratings[344]`, whose neighbours are a separate byte flag and a `<gap>`-only
global) into phantom shortfalls.

**Unlike `undercount`, a `shortfall` is never byte-neutral.** cl emits the array
symbol one or more elements short, so the linked `.bss`/`.data`/`.rdata` and the
emitted COFF both shift from that symbol onward. The tree is clean of them today
(0 findings), which the `--selftest` `shrink` injection proves is a *clean* zero
and not a blind one: it cuts an array a single function walks down to one element
and requires the finding back.

The residual blind spot, stated honestly: an array declared count ≥ 2 that is
too small AND reached only through a scaled index (which folds to offset 0, so no
past-the-end target is ever recorded) is caught by neither detector. It needs the
accessor disassembled — the reason `data_access` keeps a per-function view.

## Evidence rules

* **A content-derived address is self-confirming.** Matching a datum by its
  bytes means copying retail's bytes from the address found *by* those bytes, so
  a wrong constant still scores 100. Read the retail instruction **operand**,
  not the payload.
* **Adjacency proves nothing.** Do not close a gap by inventing an aggregate;
  that is the same error in the opposite direction.
* Do not fabricate an identity — `// @identity-TODO` is the honest marker.

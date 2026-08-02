# `DATA_SYMBOL` — what it is, and why it cannot be fully retired

`DATA(rva)` annotates a real C++ **definition** — `DATA(rva) i32 g_x;` — so the
compiler emits storage bound to `rva` (linkable).

`DATA_SYMBOL(rva, size, mangled)` is a bare **name→rva binding with no storage**.
It makes a *reference* resolve reloc-masked (so a function matches) but the linker
has nothing to point at. It is a reconstruction-incompleteness placeholder.

## Taxonomy of the `DATA_SYMBOL` uses

The backlog started at ~232 and is nearly drained — the live count is the row count
of `config/data-symbol-baseline.tsv` (read it; don't trust a number written here).

| kind | retire by |
| --- | --- |
| **POD / scalar / const global** (`?g@@3H/M/N/PAD/PBD/PAX…`, `_g_…`) | **convert to a real `DATA(rva) type name;` def** — this gate |
| **ctor'd-object global** (`?g@@3U/V<Class>@@A`) | reconstruct the class's ctor + static-init (matching campaign) |
| **compiler-generated** (`$S` local statics, `??_7/8` MI vtables, `??_R` RTTI) | reconstruct the emitting construct |

### Why the last two categories cannot be mechanically converted

- **ctor'd objects.** `CActReg g_actColl;` fails to compile — `CActReg` (via
  `zDArray`) has no usable default constructor. Retail places these registries in
  **zero-init `.bss`, runtime-`Init`'d** (hand-DynInit, not a C++ static-init), which
  a ctor'd C++ type cannot express: writing `Class g;` either errors or fabricates a
  `??__E` static-init retail does not have. Retiring one means reconstructing the
  class's default ctor (or proving it trivial) *and* matching whatever static-init
  retail emits — a per-type matching task, not a convert.
- **`$S` function-local statics** (`static CResolveNode clip;` inside
  `CImage::RenderFrame`; local `static const char s_fmt[]="…"`) emit only when that
  function body is reconstructed **with** the local static (+ its guard variable /
  ctor). Retire by reconstructing the function, not by conversion.
- **`??_7X@@6BY@@` MI-secondary vtables** (e.g. `zPTree : zErrHandling,
  CButeNodeEntry`) emit only when the class's multiple-inheritance is modeled with
  real virtuals whose vtable-key TU is reconstructed. Retire by completing the vtable
  model (gated by the `vtable_*` audits), not by conversion.

**Conclusion:** `DATA_SYMBOL` reaches 0 only at ~100% reconstruction. It is the
*correct* tool for ctor'd-object bindings and un-reconstructed compiler-gen symbols;
it is **wrong** only for POD/scalar/const globals, which must be real `DATA` defs.

## The ratchet (`gruntz.audit.data_symbol`, normal tier)

**The retireable backlog is at 0** (2026-08-02): `config/data-symbol-baseline.tsv`
is empty and the gate is **FATAL on any NEW** POD/scalar/const `DATA_SYMBOL` — a
scalar/const global must be defined, never bound storage-less. The conversion
recipe that drained it: `DATA(rva) type name;` in the owner TU, C linkage
inherited from the one header `extern "C"` declaration (never repeat
`extern "C"` on the def); recover const content from the retail image
(`pe.data[pe.off(rva)]` — e.g. `filebuf::openprot = 0644`, the two DX GUID
values); a `.bss` global is a plain zero-init def. The ctor'd-object and
compiler-gen ones are NOT flagged — they retire only through the reconstruction
campaigns above.

## Code bound as data: ILT forwarding thunks (retired mechanically)

16 of the old backlog rows (`_CreateGrunt` & co, GameObjectFactory.cpp) were not
data at all: **.text addresses in the incremental-link thunk band**
(`0x1000..0x7c20`, 5-byte `E9 rel32` forwarders). Retail's creator-table call
sites reference the THUNK; the reconstructed body lives elsewhere under the same
C name. `synth_pdb.read_ilt_thunk_names` now walks the raw band and names every
thunk whose exact jump target is a curated body (and synthesizes the 5-byte
function record when Ghidra never carved the thunk), so callers' relocations
resolve with **no annotation in src at all**. A `DATA_SYMBOL` pointing into
`.text` is always this mis-model — name the body and let the band scan do the
rest (user ruling: no DATA_SYMBOL is ever a keep; they hide proper code).

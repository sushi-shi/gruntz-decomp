# `DATA_SYMBOL` — what it is, and why it cannot be fully retired

`DATA(rva)` annotates a real C++ **definition** — `DATA(rva) i32 g_x;` — so the
compiler emits storage bound to `rva` (linkable).

`DATA_SYMBOL(rva, size, mangled)` is a bare **name→rva binding with no storage**.
It makes a *reference* resolve reloc-masked (so a function matches) but the linker
has nothing to point at. It is a reconstruction-incompleteness placeholder.

## Taxonomy of the 232 `DATA_SYMBOL` uses

| kind | count | retire by |
| --- | --- | --- |
| **POD / scalar / const global** (`?g@@3H/M/N/PAD/PBD/PAX…`, `_g_…`) | ~139 | **convert to a real `DATA(rva) type name;` def** — this gate |
| **ctor'd-object global** (`?g@@3U/V<Class>@@A`) | ~68 | reconstruct the class's ctor + static-init (matching campaign) |
| **compiler-generated** (`$S` local statics, `??_7/8` MI vtables, `??_R` RTTI) | ~25 | reconstruct the emitting construct |

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

Frozen backlog of the retireable (POD/scalar/const) `DATA_SYMBOL`s in
`config/data-symbol-baseline.tsv`; **FATAL on any NEW** one — a new scalar/const
global must be defined, never bound storage-less. Drive the backlog to 0 by
converting each to `DATA(rva) type name;` (recover const content from the retail
image via `gruntz.core.pe.cstr` / `pe.data[pe.off(rva)]`; `.bss` scratch is zero-init
sized by the gap to the next symbol). The ctor'd-object and compiler-gen ones are NOT
flagged — they retire only through the reconstruction campaigns above.

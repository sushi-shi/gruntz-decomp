# Codebase cleanliness metrics

Tracked targets for the reconstruction's **type/call/name layer** — the goal is a
clean, portable C++ source (the single source of truth), not just byte-matches.
Each metric is matching-neutral to drive toward its target unless noted.

**Live tracking.** These counts are computed by `gruntz verify board`
(comment- and string-stripped, so prose/`//` annotations don't inflate) and printed
as the fast tier's `board` gate, each with a **delta vs the committed
text/semantic baselines** (`config/cleanliness/cleanliness-*-baseline.tsv`) —
`down = good`. So a matcher sees its own cast/placeholder/view change the moment
it builds and steers on it. The default build measures only the fast text
metrics; `gruntz verify check --tier full` also measures the build/IR-derived
semantic metrics. The gate NEVER writes a baseline: bless text floors with
`gruntz verify board --update`, or both families with
`gruntz verify board --semantic --update` (the
orchestrator refreshes it at integration, like `match_baseline.tsv`).

**No counts are written down here.** The tables below name each metric's row key in
`config/cleanliness/cleanliness-text-baseline.tsv` and
`config/cleanliness/cleanliness-semantic-baseline.tsv` — read the live number there or from the build
report. A pasted snapshot silently rots (this doc once carried figures that were off
by 2×–30×), and the `rg` commands are *approximations* of the board's
comment-/string-stripped count, kept only to show what each metric looks for.

The **hard rule** these encode: *there should be no casts, no `void*` members, no
offset-/address-derived identifiers, `Unknown`/`Method<N>` placeholders, and no per-TU/fabricated
views where we can afford it* — a survivor means something isn't typed or named
right. Keep a cast/placeholder ONLY when removing it changes the matched bytes and
no correct typing/naming avoids it (i64 DWORD-pair overlays, pointer-as-int
transcription). Portable types only: plain `char*`/`void*`/`i32`/`u32`, **never**
Windows typedefs (`LPTSTR`/`LPCTSTR`/`LPVOID`/`DWORD`) — cross-platform port later.
Real MFC *classes* (CString/CObList) stay (their codegen is matched); for a
CString body rely on the implicit `operator LPCTSTR()` — pass the CString
directly where `const char*` is expected, no cast (and no helper).

## Naming (target 0 where provable)

| metric | baseline row | what it looks for | target |
|---|---|---|---|
| `m_<hex>` fields | `m_<hex> fields` | `\bm_[0-9a-f]{2,}\b` (minus `m_<n><g-z>`) | provable→named; only genuinely-unknowable remain. **Renames come LAST** — model first |
| address-/slot-derived names | `address-derived identifiers` | `local_14`, `m_10map`, `g_ratingRaw_64da84` and equivalent decorated forms | 0 — use an evidence-backed role, or an honest `reserved` name when no role is proven |
| `Unknown` identifiers (class/method/field/vslot) | `Unknown ids` | `\w*[Uu]nknown\w*` | **0** — name by xref/RTTI |
| `g_<hex>` globals | `g_<hex> globals` | `\bg_[0-9a-f]{4,}\b` | 0 (name by usage) |
| `Method<N>`/`Stub_`/`vfunc_`/`FUN_` | `Method/Stub/FUN/Gap` | placeholder function names | 0 |
| virtual-slot placeholders | `virtual slot placeholders` | unnamed vtable slots | 0 (name the slot by xref) |
| positional arg placeholders | `positional arg placeholders` | `a1`/`arg0`-style params | 0 (name by use) |

## Casts (target 0 where affordable — type the member/param/local/return)

Policy and the named-cast rules live in `docs/cast-metric-policy.md`; offset-casts
`(char*)x + N` are **banned outright** in every form.

| metric | baseline row | target |
|---|---|---|
| C-style casts | `C-style casts` | 0 — named cast, or (better) fix the type |
| `reinterpret_cast`s | `reinterpret_casts` | drive down; every survivor needs a reason |
| casts with no recorded reason | `unexplained casts` | **0** (the OPEN-drain metric) |
| offset-cast macros | `offset-cast macros` | 0 (banned) |
| `void* m_` members | `void* m_ members` | 0 (type the member) |

## Structure / views (target: no fabricated or per-TU views; classes in headers)

| metric | baseline row | target |
|---|---|---|
| classes/structs declared in a `.cpp` | `.cpp-local views` | 0 — cross-TU classes belong in headers; no per-TU re-decls |
| fabricated placeholder classes | `placeholder classes` | 0 (recover the real class; never invent a view) |
| view classes in `*Views.h` | `view classes (*Views.h)` | 0 (fold into the real class) |
| fake-view caller/callee edges | `caller-callee FAKE-VIEW` (semantic baseline) | 0 |
| directly nested casts | `nested static_casts` (semantic baseline) | 0 through correct typing/conversions — inspect the reported source/intermediate/final types; never hide a pair in a helper/local |
| hand-rolled vtables | `*Vtbl structs`, `->vtbl accesses`, `g_*Vtbl globals`, `m_vtbl/m_vptr members`, `placeholder vtable slots` | 0 — model real virtuals |
| `.cpp` extern decls / external prototypes | `cpp extern decls`, `cpp external prototypes` | 0 (declare in the owning header) |
| the same symbol `extern`-declared in 2+ headers (or twice in one) | `duplicate header externs` | 0 — **one** declaration, in the owner header, which consumers `#include`. List them with `gruntz verify board --dup-externs` |

> `cpp extern decls` reads 0 and always did once the .cpp copies were drained — but the
> construct had simply moved into headers, where that metric does not look: 52 symbols
> were `extern`-declared in 2+ headers (`g_frameTime` in **eight**; `GruntzMgr.h`
> declared eight of its own globals **twice**), 72 redundant declarations in all, and
> the counter stayed green throughout. `duplicate header externs` is the half that
> follows the construct. It counts REDUNDANCY (occurrences − 1 per symbol), not header
> externs, because a header extern IS the legitimate owner declaration — only a second
> one is debt. Exactly one pair is exempt: `<Mfc.h>` / `<Win32.h>` are mutually
> exclusive umbrellas (MFC's C1189 forbids both), so their mirrored prelude
> declarations are never both visible in a TU.
>
> Removing a duplicate is usually byte-neutral, but it is not free: every TU that saw
> both copies loses one from cl 5.0's cumulative declaration count, which steers /O2
> register allocation (`docs/patterns/declaration-count-window-steers-regalloc.md`).
> Work **one symbol at a time** and read the per-function objdiff.

## Build gates (fatal at 0)

| metric | command | state |
|---|---|---|
| Vtable catalog | `gruntz verify vtables` | **FATAL** — catalog rows are structurally valid; a class whose `??_7` retail never emitted simply has no `data_vtables.tsv` row |
| src claims ∩ functions_static_libs.tsv | `gruntz verify library-overlap` | **FATAL** (no allowlist) — FULL generated symbol set: rva-macro + RVA_COMPGEN + DATA (vendored zlib excluded by source, not allowlist) |
| compiler-generated DATA pins | `gruntz delink` | **FATAL** — every `class=common` row in `config/retail/data_compgen.tsv` must be emitted as a COFF COMMON by some base obj, and a COMMON with no emitting obj is an error |

The `truncated masks` row survives in the semantic baseline at **0** with no
instrument behind it — the immediate-mask sieve is retired (see
[tooling-map](tooling-map.md)). The defect it caught: our base masks a 32-bit
word with the 8/16-bit complement of retail's constant (`andl $0xdf` vs
`andb $0xdf`), so the write clears the intended bit AND everything above the
byte. Always a width or union-member slip; NOT the MSVC5 enum story
(`docs/patterns/enum-complement-is-sixteen-bit.md`).

## Match (the binary-matching goal)

`gruntz verify status`. Never write the number down — see the note at the top of this file.

## Workstreams

WS1 de-hack casts · WS2 name backlog (incl. **no `Unknown`**) · WS3 fold
fabricated/MFC views into real classes · WS4 promote cross-TU classes to headers +
reconstruct real calls.

## Aggregate suspicion queues

Cast counts cannot detect a flattened aggregate when code legally takes the
address of its first scalar field. The shape to look for: an integer or
floating-point pointer initialized from a scalar member/global, plus I/O that
spans beyond a scalar's declared extent. Pointer indexing and arithmetic
strengthen a row but are not required: the scalar address escape is itself
suspicious. The other global form is multiple `DATA()` symbols pinned at
interior offsets of one typed object — the Model reports the interior claim, and
`gruntz verify data-access` reports what retail actually touches there. Mere RVA
adjacency is never proof that globals share an owner; confirm each row from
retail accesses and serialization boundaries before regrouping fields. (The two
one-shot discovery scans that first produced these queues are retired — see
[tooling-map](tooling-map.md).)


## Numeric-domain metrics (the enum campaign)

Added with `docs/enum-modeling-plan.md`. All three are **ratcheted** (down-only).

| metric | baseline row | what it looks for | target |
|---|---|---|---|
| magic case labels | `magic case labels` | `case 0x3e8:` - a `case` whose label is a bare number, i.e. an un-named member of some domain | 0 |
| unnamed domain compares | `unnamed domain compares` | `x == 0x36` - the comparison twin. `== 0` / `== 1` are EXCLUDED: those are null/bool tests, not domain membership | 0 |
| .cpp-local enums | `.cpp-local enums` | a domain declared inside a `.cpp`. Fine when genuinely TU-private; ratcheted so a CROSS-TU domain is never stranded there (that is how this tree ended up with three spellings of the grunt/pickup id space) | ratchet only |

Naming a value is matching-neutral - `docs/patterns/enum-domains.md` measures
literal -> enumerator as leaving `.text` byte-identical - so all of this is pure
debt, not a trade-off against the score.

One companion gate runs beside them: **`gruntz verify enum-domains`** (fast
tier) — a `_SPLIT` domain's declared storage must match every `GZ_ENUM_STORAGE`
width used for it, no bare `enum X {` outside the macros (single-enumerator tag
types exempt), range tests name boundaries rather than members, and enumerator
names follow the domain convention. Negative controls live in
`gruntz verify selftest`.

The `/std:c++20` strict-enum probe that once ran beside it (compiling the tree
with the domains as `enum class`, to surface a domain used as a raw array index
or silently widened through an `i32`) is retired along with its baseline file.
The judgement it demanded stands: which domain an integer carries is decided
from evidence, never guessed.

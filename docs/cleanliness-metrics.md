# Codebase cleanliness metrics

Tracked targets for the reconstruction's **type/call/name layer** — the goal is a
clean, portable C++ source (the single source of truth), not just byte-matches.
Each metric is matching-neutral to drive toward its target unless noted.

**Live tracking.** These counts are computed by `gruntz.cleanliness.board`
(comment- and string-stripped, so prose/`//` annotations don't inflate) and printed
in the **`gruntz build` report** right under the match summary, each with a **delta
vs the committed baseline** (`config/cleanliness/cleanliness-baseline.tsv`) — `down = good`. So a
matcher sees its own cast/placeholder/view change the moment it builds and steers on
it. Bless a new baseline with `python -m gruntz.cleanliness.board --update` (the
orchestrator refreshes it at integration, like `match_baseline.tsv`).

**No counts are written down here.** The tables below name each metric's row key in
`config/cleanliness/cleanliness-baseline.tsv` — read the live number there or from the build
report. A pasted snapshot silently rots (this doc once carried figures that were off
by 2×–30×), and the `rg` commands are *approximations* of the board's
comment-/string-stripped count, kept only to show what each metric looks for.

The **hard rule** these encode: *there should be no casts, no `void*` members, no
`m_<hex>`/`Unknown`/`g_<hex>`/`Method<N>` placeholders, and no per-TU/fabricated
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
| fake-view caller/callee edges | `caller-callee FAKE-VIEW` | 0 |
| hand-rolled vtables | `*Vtbl structs`, `->vtbl accesses`, `g_*Vtbl globals`, `m_vtbl/m_vptr members`, `placeholder vtable slots` | 0 — model real virtuals |
| `.cpp` extern decls / external prototypes | `cpp extern decls`, `cpp external prototypes` | 0 (declare in the owning header) |

## Build gates (fatal at 0)

| metric | command | state |
|---|---|---|
| SIZE missing | `python -m gruntz.cleanliness.class_sizes` | **FATAL** (`--full` tier) |
| VTBL missing | `python -m gruntz.cleanliness.class_vtables --assert-unique` | **FATAL** — catalogue is complete (proven-absent `??_7` carry `VTBL_ABSENT`) |
| src claims ∩ library_labels.csv | `python -m gruntz.match.verify_library_overlap` | **FATAL** (no allowlist) — FULL generated symbol set: rva-macro + RVA_COMPGEN + DATA (vendored zlib excluded by source, not allowlist) |
| stub metadata / dup / stub-vs-matched | `python -m gruntz.match.verify_stubs` | **FATAL** |

## Match (the binary-matching goal)

`python -m gruntz.match.status --report build/objdiff/report.json summary`, or just
`gruntz status`. Never write the number down — see the note at the top of this file.

## Workstreams

WS1 de-hack casts · WS2 name backlog (incl. **no `Unknown`**) · WS3 fold
fabricated/MFC views into real classes · WS4 promote cross-TU classes to headers +
reconstruct real calls.

## Aggregate suspicion queues

Cast counts cannot detect a flattened aggregate when code legally takes the address of
its first scalar field. Run `python -m gruntz.audit.flattened_aggregates` to find every
integer or floating-point pointer initialized from a scalar member/global, plus I/O that
spans beyond a scalar's declared extent. Pointer indexing and arithmetic strengthen a
row but are not required for reporting: the scalar address escape is itself suspicious.
This is an investigation queue, not a ratchet; confirm each row from retail accesses and
serialization boundaries before regrouping fields.

For the other global form—multiple `DATA()` symbols pinned at interior offsets of one
typed object—run `python -m gruntz.audit.shredded`. That audit uses compiler-derived
object extents; mere RVA adjacency is not treated as proof that globals share an owner.


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

Two companion gates run beside them:

- **`gruntz audit enum-domains`** (`--gate`, normal tier) - split-domain storage
  widths must agree tree-wide, no bare `enum` in a header (single-enumerator tag
  types exempt), range tests name boundaries rather than members, and enumerator
  names follow the domain convention. Negative controls live in
  `gruntz.match.gate_selftest`.
- **`gruntz audit strict-enums`** - compiles the tree at `/std:c++20`, where the
  domains become `enum class`, and reports what the MSVC build cannot see: a
  domain used as a raw array index, a domain silently widened through an `i32`
  parameter, two domains conflated behind one `i32`. Floor in
  `config/cleanliness/strict-enums-baseline.tsv`. **Expect the count to RISE before it falls**
  — it measures how much of the tree still treats domains as ints, so declaring a
  new domain increases it until that domain's consumers are typed. It is NOT
  drivable to zero mechanically: the residual sites each need a judgement about
  which domain an integer carries, and some carry more than one. Ratcheted
  down-only; drain with evidence, never by guessing a type.

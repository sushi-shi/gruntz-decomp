# Codebase cleanliness metrics

Tracked targets for the reconstruction's **type/call/name layer** — the goal is a
clean, portable C++ source (the single source of truth), not just byte-matches.
Each metric is matching-neutral to drive toward its target unless noted.

**Live tracking.** These counts are computed by `gruntz.cleanliness.board`
(comment- and string-stripped, so prose/`//` annotations don't inflate) and printed
in the **`gruntz build` report** right under the match summary, each with a **delta
vs the committed baseline** (`config/cleanliness-baseline.tsv`) — `down = good`. So a
matcher sees its own cast/placeholder/view change the moment it builds and steers on
it. Bless a new baseline with `python -m gruntz.cleanliness.board --update` (the
orchestrator refreshes it at integration, like `match_baseline.tsv`). The counts in
the tables below are a **snapshot** (2026-07-05, comment-stripped) — the report is
authoritative.

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

| metric | command | current | target |
|---|---|---|---|
| `m_<hex>` fields | `rg -oN '\bm_[0-9a-f]{2,}\b' src include \| rg -v 'm_[0-9]+[g-z]' \| wc -l` | 22413 | provable→named; only genuinely-unknowable remain |
| `Unknown` identifiers (class/method/field/vslot) | `rg -oN '\w*[Uu]nknown\w*' src include \| wc -l` | 835 | **0** |
| — `ClassUnknown_N` (distinct) | `rg -oN 'ClassUnknown_[0-9]+' src include \| sort -u \| wc -l` | 86 | 0 (name by xref/RTTI) |
| — `VirtualMethodUnknownNN`/`UnknownVirtualMethodNN` | `rg -oN '(Virtual[Mm]ethodUnknown\|Unknown[Vv]irtual[Mm]ethod)\w*' src include \| wc -l` | 186 | 0 (name the vtable slot by xref) |
| `g_<hex>` globals (distinct) | `rg -oN '\bg_[0-9a-f]{4,}\b' src include \| sort -u \| wc -l` | 441 | 0 (name by usage) |
| `Method<N>`/`Stub_`/`vfunc_`/`FUN_` | `rg -oN '\b(Method[0-9a-f]{3,}\|Stub_[0-9a-f]+\|vfunc_[0-9]+\|FUN_[0-9a-f]+)\b' src include \| wc -l` | 742 | 0 |

## Casts (target 0 where affordable — type the member/param/local/return)

| metric | command | current | target |
|---|---|---|---|
| `)this` casts | `rg -N '\)this' src include \| wc -l` | 468 | ~0 (raw-offset→field; keep only load-bearing byte-arith) |
| `)m_` casts | `rg -N '\)m_' src include \| wc -l` | 1429 | ~0 (type the member) |
| `(char*)` casts | `rg -N '\(char\*\)' src include \| wc -l` | 1400 | ~0 (name +N field / type buffer) |
| `(const char*)` casts | `rg -N '\(const char\*\)' src include \| wc -l` | 204 | ~0 (drop — implicit `LPCTSTR` conversion) |
| `void* m_` members | `rg -N 'void\* m_' src include \| wc -l` | 657 | ~0 (type the member) |

## Structure / views (target: no fabricated or per-TU views; classes in headers)

| metric | command | current | target |
|---|---|---|---|
| classes/structs declared in `.cpp` | `rg -c '^(class\|struct) [A-Z]' src/**/*.cpp \| awk -F: '{s+=$2} END{print s}'` | 2744 | cross-TU-called ones → headers (WS4); no per-TU re-decls |
| `src/Stub/ApiCallers.cpp` fns | `rg -c 'RVA\(0x' src/Stub/ApiCallers.cpp` | 126 | 0 (re-home by xref, WS7) |
| `src/Stub/types/*.h` files | `ls src/Stub/types/*.h \| wc -l` | 9 | 0 (consume→real homes, WS6) |

## Build gates (fatal at 0)

| metric | command | current | state |
|---|---|---|---|
| SIZE missing | `python -m gruntz.cleanliness.class_sizes` | 0 | **FATAL** (enforced) |
| VTBL missing | `python -m gruntz.cleanliness.class_vtables` | 333 | reporting → fatal at 0 |
| src claims ∩ library_labels.csv | `python -m gruntz.match.verify_library_overlap` | 0 | **FATAL** (enforced, no allowlist) — FULL generated symbol set: rva-macro + RVA_COMPGEN + DATA (vendored zlib excluded by source, not allowlist) |
| stub metadata / dup / stub-vs-matched | `python -m gruntz.match.verify_stubs` | 0 | **FATAL** (enforced) |

## Match (the binary-matching goal)

| metric | command | current |
|---|---|---|
| exact / fuzzy | `python -m gruntz.match.status --report build/objdiff/report.json summary` | 1829/3279 (55.78%) · 68.51% fuzzy |

## Workstreams (see task tracker)

WS1 de-hack casts · WS2 name backlog (incl. **no `Unknown`**) · WS3 fold
fabricated/MFC views into real classes · WS4 promote cross-TU classes to headers +
reconstruct real calls · WS5 caller-audit tool (header-promotion / orphan /
access-mismatch worklists) · WS6 consume+axe `src/Stub/types/` · WS7 re-home
`ApiCallers.cpp` by xref.

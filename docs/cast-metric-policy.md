# Cleanliness endgame: drive every metric to 0 + the cast policy

**Governing directive (2026-07-16).** After the reconstruction/fold work, the campaign's job is to
drive **every** metric in `config/cleanliness-baseline.tsv` to **0**, then move to other tasks
(next: DATA-section attribution + the objdiff DATA-match loop — see the roadmap). The tracked
metrics, all drive-to-0:

- the cast counts — `)this casts`, `)m_ casts`, `(char*) casts`, `(const char*) casts`
- `void* m_ members`
- `.cpp-local views`
- `placeholder classes`
- `cpp extern decls` and implicit `cpp external prototypes` — external declarations belong in
  their owner headers even when the prototype omits the `extern` keyword
- `caller-callee FAKE-VIEW` (the one mechanical slice of the call-graph reconcile — retype the
  receiver to the real class so the call binds; NOT the MISSING/SPECIAL total, which is the deep
  per-function matching residual, not a cleanliness crater)
- `m_<hex> fields` — **last** (pure naming; do it after the structural metrics are 0)

## The cast policy

A cast is a **symptom**; the defect is usually the type above it. Three cases:

1. **Mis-model cast — ELIMINATE by real typing.** A view-cast `(CFooView*)x`, a `)this` cast, a cast
   that only exists because a `void* m_` member or a `.cpp`-local view is fake: fix the *type* (retype
   the member to its real class, dissolve the view onto the canonical class) and the cast falls out on
   its own. Never launder a mis-model into a C++ cast — that's the same lie, hidden from the metric.

2. **Genuinely-needed cast — make it a C++ NAMED cast (never C-style `(T)`).** Once the type above is
   correct and a real language-required conversion remains (an MFC container element `(CObject*)` /
   `POSITION`, a sentinel `(void*)0x64`, a COM `(HWND)`, a proven reinterpret between two real types),
   use the right named cast. This un-matches the C-style-pattern metrics, so they slide to 0:
   - **`static_cast`** — math / numeric / value conversions: `(u32)m_dwell` → `static_cast<u32>(m_dwell)`,
     `i32`↔`u32`, `i32`→`float`, `enum`↔`int`, arithmetic width changes, and up/down casts within a
     *known* hierarchy.
   - **`reinterpret_cast`** — pointer / handle reinterprets between unrelated real types.
   - **`const_cast`** — const only.
   - **`dynamic_cast`** — RTTI downcasts (rare here; the binary is mostly /GR-off).

3. **Offset-cast `(char*)x + N` — HARD BAN, no exception.** Pointer + byte-offset to reach a member is
   *always* a mis-model: the member at `+N` is real, so it becomes named access `&x->m_field` /
   `x->m_field`. This is banned **in every form** — do NOT satisfy the `(char*)` metric by rewriting
   `(char*)x + N` as `reinterpret_cast<char*>(x) + N`; that is still the banned offset-cast, just hidden
   from the regex. Fix the member.

## Byte-neutrality

All of the above is matching-neutral: a C++ named cast compiles to the same bytes as the C-style cast,
a named member access to the same bytes as the offset-cast, a renamed member/method to the same bytes
under /O2. Verify per change (`gruntz build --fast`, touched unit % unchanged) and gate on
MAX-fuzzy — see [max-fuzzy gate](../README.md) and `docs/cleanup-plan.md`.

# Cleanliness endgame: drive every metric to 0 + the cast policy

**Governing directive (2026-07-16).** After the reconstruction/fold work, the campaign's job is to
drive **every** metric in the two `config/cleanliness/cleanliness-*-baseline.tsv`
files to **0**, then move to other tasks
(next: DATA-section attribution + the objdiff DATA-match loop — see the roadmap). The tracked
metrics, all drive-to-0:

- the cast counts — `)this casts`, `)m_ casts`, `(char*) casts`, `(const char*) casts`
- every `reinterpret_cast` (a down-only ceiling even when a reviewed instance is
  temporarily legitimate)
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

Named does not mean invisible. The cleanliness board counts every
`reinterpret_cast` and ratchets that total down. A reviewed ABI/container cast may
remain while its owner is understood, but a new one fails the gate: first prove why
the existing type cannot express the operation.

### Directly nested `static_cast` review

`python -m gruntz.audit.nested_static_casts` reports an AST cast whose operand is
another `static_cast`; neighboring casts in separate statements do not count. Its
`source -> intermediate -> final` output is a review queue, not a claim that every
pair has pointer-reinterpretation semantics. The audit is semantic and therefore
runs only in `gruntz build --full`.

Remove value-preserving detours. A source already typed `u32` does not need
`i64 -> double`, `u32 -> i64`, or `i32 -> u32` staging. Fix an incorrectly signed
field at its declaration when evidence supports that stronger cleanup.

The remaining common pairs change the value domain and must be reviewed against
retail instructions before removal:

- `i32 -> u32 -> float/double/i64` preserves the low 32 bits and then zero-extends;
  retail commonly exposes this as a zeroed high dword followed by `fild qword`.
- `char -> u8 -> u32` prevents sign extension before byte packing and shifts.
- `float -> i32 -> u8` makes truncation happen before byte narrowing.
- `i32 -> u8 -> enum` recovers a one-byte serialized enum from a promoted value.
- strict enum-storage proxies expose only their owning enum and the canonical
  explicit `i32` conversion used by `IDX`; unused cross-storage and arbitrary
  integer bridges do not belong in the proxy.

None of these authorizes type punning. An unrelated pointer/object chain is a
modeling defect, and a chain must not be split across throwaway locals or hidden in
a helper merely to evade the audit.

3. **Offset-cast `(char*)x + N` — HARD BAN, no exception.** Pointer + byte-offset to reach a member is
   *always* a mis-model: the member at `+N` is real, so it becomes named access `&x->m_field` /
   `x->m_field`. This is banned **in every form** — do NOT satisfy the `(char*)` metric by rewriting
   `(char*)x + N` as `reinterpret_cast<char*>(x) + N`; that is still the banned offset-cast, just hidden
   from the regex. Fix the member.

## Byte-neutrality

All of the above is matching-neutral: a C++ named cast compiles to the same bytes as the C-style cast,
a named member access to the same bytes as the offset-cast, a renamed member/method to the same bytes
under /O2. Verify per change (`gruntz build --fast`, touched unit % unchanged) and gate on
MAX-fuzzy — see [max-fuzzy gate](../README.md).

## Clean-room mandate

**(user, 2026-07-04, reaffirmed)** The goal is a clean-room implementation. When a
cleanup change is byte-*affecting* rather than byte-neutral — regalloc churn,
reordering, header-fattening — the % drop is **ACCEPTED**: report and bless it, never
revert correct work or skip it to protect the score. A % drop must not deter the work;
a change stops only on **build failure or wrong evidence**, never on a current-%
dip. The match % is pushed back up afterwards, once the source is clean.

Reconstruction/matching work is the exception — there the push-to-100% doctrine
applies. See the MAX-match convention in `CLAUDE.md`.

# A repeated FP block: copy-paste it (macro), don't `__inline` a helper
tags: cpp:float cpp:inline cpp:macro | asm:fld asm:fxch asm:fmul | topic:codegen-idiom topic:scheduling
symptoms: a function runs the SAME multi-statement `double` block N times down parallel member columns (per-axis X/Y/Z, per-channel R/G/B, …); reconstructing it as one `__inline` helper called N times plateaus far LOW (~35%) with a deep, over-scheduled FP stack — the recompile hoists several columns' constant `fld`s into the prologue and interleaves the iterations
confidence: 7/10

When retail emits N byte-identical copies of an x87 block (one per axis/column), the
original source was almost certainly **N literal copy-pasted blocks** (or a
function-like `#define` macro), NOT a helper inlined N times. The two are NOT
equivalent at /O2:

- An `__inline` helper gives MSVC 5.0's scheduler ONE big region spanning all N
  iterations. It greedily floats independent loads (each column's `fld
  [this+const]`) up into the prologue and **interleaves** the iterations on a deep
  FP stack — diverging from retail's shallow, one-column-at-a-time stack where the
  shared scalar (e.g. the frame `dt`) persists on `st0` across blocks.
- A **macro** (or hand-copied blocks) gives the scheduler N SEPARATE regions with a
  sequence point between them, so each block schedules locally with a shallow stack
  — matching retail's copy-paste.

Lever: write the repeated body as a `#define BLOCK(v,a,s,…) do { … } while (0)`
taking the per-column member tokens, expanded once per column. (CMotionState::Step
0x16ecd0: `__inline StepAxis(...)` x3 → **35%**; the identical body as a `STEP_AXIS`
macro x3 → **65%**, prologue + control flow + every member store now byte-exact.)

```cpp
#define STEP_AXIS(v, a, s, vmax, loBand, hiBand, posClamp, scr)   \
    do { double step0 = dt * a; double t = (v - step0 * -0.5) * dt; \
         scr = t; /* … clamp / quadratic-solve / band, all on v,a,s … */ \
    } while (0)
void CMotionState::Step(double dt) {
    /* prologue … */
    STEP_AXIS(m_28, m_10, m_40, m_d8, m_70, m_88, m_f0, m_a0);  // X
    STEP_AXIS(m_30, m_18, m_48, m_e0, m_78, m_90, m_f8, m_a8);  // Y
    STEP_AXIS(m_38, m_20, m_50, m_e8, m_80, m_98, m_100, m_b0); // Z
}
```

This closes the cross-iteration interleave (the big DELETE/INSERT desync). The
REMAINING residual inside each block — the `fld st(0)`-vs-`fld [mem]` operand choice
and the `fxch` ordering of the quadratic-solve — is the per-block x87 stack-schedule
wall and is NOT further steerable; see [[x87-fp-stack-schedule]]. Recognize the
65%-ish plateau with byte-exact stores and stop.

variants: [[x87-fp-stack-schedule]] (the per-block residual), [[inline-switch-serialize-record-unroll]] (the integer analogue: `__inline` vs unrolled records).

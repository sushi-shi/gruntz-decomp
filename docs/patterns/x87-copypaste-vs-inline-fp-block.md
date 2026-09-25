# Repeated FP blocks: test the macro and inline boundaries
tags: cpp:float cpp:inline cpp:macro | asm:fld asm:fxch asm:fmul | topic:codegen-idiom topic:scheduling
symptoms: a function runs the SAME multi-statement `double` block N times down parallel member columns (per-axis X/Y/Z, per-channel R/G/B, …); reconstructing it as one `__inline` helper called N times plateaus far LOW (~35%) with a deep, over-scheduled FP stack — the recompile hoists several columns' constant `fld`s into the prologue and interleaves the iterations
confidence: 5/10

Repeated x87 blocks justify testing both a macro and an inline function with
the complete parameter, result and local-lifetime boundaries. Their VC5 output
can differ even when their arithmetic is equivalent. They do not, by
themselves, identify which source spelling the original used.

An earlier `CMotionState::Step` experiment reported cross-axis load hoisting
and a deeper FP stack with an inline helper, reduced by a macro expansion.
That is evidence about those particular source states, not a universal
scheduler rule. In particular, macro expansion adds **no C++ sequence point**
and does not inherently partition scheduling into independent regions.

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

The historical 35% to 65% observation is not a stopping criterion. Subsequent
Step history reached 82.9390%, directly contradicting this page's former
claim that the remaining schedule was unsteerable. The fresh complete-family
review is tracked by `reassess-sqr-motion` and
`reassess-sqr-constref-template-profile` in `config/lithtech_lineage.tsv`.
Those rows, not this pattern, own candidate dispositions and reopening evidence.

Reverse-use signature: compare axis-local loads, spills and FP-stack exchanges
from the first divergence, while independently checking all member offsets,
guards and raw relocation identities. A changed helper boundary can move the
first divergence before its arithmetic expansion. Compare the alleged improved
feature with the baseline too; a dipped state is useful only if it introduces
something the baseline lacked. Preserve source-backed abstractions and compose
independent lifetime evidence before declaring a scheduling residue bounded.

variants: [[x87-fp-stack-schedule]] (the per-block residual), [[inline-switch-serialize-record-unroll]] (the integer analogue: `__inline` vs unrolled records).

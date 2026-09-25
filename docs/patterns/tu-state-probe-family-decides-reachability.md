# Translation-unit context can change an unchanged function

Recorded VC5 experiments changed unrelated declarations while leaving a target
body unchanged and obtained different allocation or scheduling. Function text
alone is therefore not a complete description of compiler input.

The [historical IL experiment](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/tu-state-probe-family-decides-reachability.md#quantified-2026-08-13-the-input-mechanism-is-c1xx-symbol-handle-renumbering-and-each-probe-kind-has-a-measured-stride)
used /d1il<prefix> to capture the ex/gl/in/sy streams and /d2il<prefix> to replay
them. Its reported replay reproduced the changed output. This locates an input
difference at the frontend/backend boundary; it does not prove the downstream
decision is made by the frontend. See the separate
[backend investigation](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/relevations/cl5-globalopt-has-a-511-handle-phase.md).

For a controlled comparison, keep compiler, flags, headers, and target source
fixed; retain both objects and compare ordered referents as well as instructions.
IL byte differences need interpretation, including source-line records.

A flat sweep means those probes did not move that input. Neither uniform nor
mixed probes prove a function unreachable, a TU permanently insensitive, or a
particular backend cause. Historical handle strides are measurements of those
probes, not portable compiler constants.

Do not retain unused declarations, includes, or fake locals to select an output.
Use the bounded [permuter workflow](../permuter.md), not an unbounded hunt for
a favorable score. Source correctness and the current MAX policy still govern
what is kept.

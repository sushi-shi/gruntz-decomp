# An inline can have expanded and out-of-line uses

A caller may contain both an expanded helper body and calls to its out-of-line
copy. That is compatible with an ordinary inline definition; it does not by
itself require separate APIs or manually expanded source.

The [recorded VC5 probes](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/inline-budget-emits-ool-comdat.md)
vary caller content and repeated eligible call sites, observing different
expansion counts. Caller context and nested expansion matter. The repository's
[inline model](../../scripts/gruntz/walls/inline_model.py) is a conditional
predictor derived from a sibling compiler and calibrated on selected VC5 probes,
not a proof of every VC5 inlining decision.

Check resolved call targets and ordered sites, including tail jumps. Confirm
eligibility separately; [template members](vc5-template-members-inline-without-inline-keyword.md)
are an exception to a keyword-only /Ob1 rule. An /Ob0 comparison helps expose
boundaries, but optimization can still merge or remove sites.

A locally defined COMDAT supports body availability. An undefined or absent
symbol does **not** prove the body was unavailable: another site may expand,
a nested site may remain external, or delinking may obscure the provider.

An all-expanded harness is saturated, not proof of a zero-cost helper.
Calibrate a partially rejecting case before inferring a budget; do not equate
machine-code bytes with the compiler's internal size estimate. A missing call
can also be tail merging or dead-code elimination.

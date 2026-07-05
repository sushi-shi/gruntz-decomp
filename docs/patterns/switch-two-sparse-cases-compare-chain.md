# A 2-sparse-case `switch` lowers to a compare CHAIN (no jump table) — case-order block layout
tags: cpp:switch cpp:branch | asm:cmp asm:je asm:jne | topic:codegen-idiom topic:tail-merge
symptoms: `cmp K1; je L1; cmp K2; jne skip` with the FIRST case out of line / second case inline, if-ladder stuck ~80%
confidence: 8/10

A `switch` on a small set of **sparse, non-contiguous** case labels (e.g. `case 4` /
`case 7`) does NOT emit a jump table under MSVC5 /O2 — it emits a **compare chain**
`cmp K1; je L1; cmp K2; jne default; <case-K2 body inline>; ...; L1: <case-K1 body>`.
The decisive layout property: the FIRST case label's body is placed **out of line**
(the `je` jump target) and the SECOND case's body is **inline** (the `jne default`
fall-through). An `if (x==K1){A} else if (x==K2){B}` ladder instead inlines the FIRST
arm (`cmp K1; jne next; A inline; …`), which is retail's layout MIRRORED — the wall.

So: when the target shows `cmp K1; je <fwd>; cmp K2; jne <skip>` with case-K1 out of
line, spell it as a real `switch`, not an if-ladder. (This REFUTES the older note that
"a switch would add a jump table" — only true for dense/contiguous ranges; 2 sparse
cases stay a compare chain.)

## Also verify the case guard's polarity from the branch sense
A per-case early-out (`case K: if (guard) return 0; break;`) picks whether the
`return 0` is inline vs the jump target by the `je`/`jne` sense at the guard. If the
target does `test eax,eax; jne <emit>` (jump AWAY to the shared continue/emit block,
`return 0` inline via `pop;pop;ret`), the guard is `if (guard == 0) return 0;` — the
zero case falls through to return, the nonzero case breaks to emit. A wrong-polarity
`if (guard != 0) return 0;` emits `je <continue>; xor eax,eax` (return 0 inline, emit
out of line) — the mirror. Read the branch sense from the disasm; it also pins the
callee's real semantics (here the validator's NONZERO = "valid, proceed").

Evidence: CBattlezMapConfig::Method_02bfc0 0x2bfc0 — if-ladder + `!=0` guard 80.7%;
switch 95%; switch + `==0` guard 100%.

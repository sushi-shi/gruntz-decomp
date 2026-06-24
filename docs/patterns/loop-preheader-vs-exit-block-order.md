# Guarded counted loop: `for` floats the invariant preheader ABOVE the entry guard; `do-while` keeps it inside but swaps the success/error exit-block order
tags: cpp:loop cpp:branch cpp:eh | asm:lea asm:jcc asm:jmp | topic:wall topic:scheduling topic:regalloc

symptoms: a counted loop whose body computes loop-invariant base pointers from `this` (the `lea esi,[edi+N]` preheader) and returns 1 on completion / 0 on an in-loop failure. Two MSVC5 /O2 shapes are reachable and neither matches retail fully:
 - `for (i=0; i<count; i++) { ... }` reproduces retail's **exit-block order** (success epilogue first / error tail second) and the backward `jl` back-edge, but **hoists the invariant `lea` preheader ABOVE the `i<count` entry guard** and adds a `jmp` into the body (retail computes the leas AFTER the `jle` guard, in the taken region).
 - `if (count > 0) { <leas>; i=0; do { ... } while (i < count); }` puts the leas correctly AFTER the guard but **inverts the exit-block order** (error epilogue first / success second) and emits `jge exit; jmp top` (two jumps) for the back-edge instead of one `jl`.
confidence: 7/10

The conflict is between two independent MSVC5 layout decisions:
 1. Loop-invariant code motion places the preheader either before or inside the entry guard, depending on whether the loop is a rotated `do-while` (preheader inside) or a `for`/`while` (preheader in a pre-guard block).
 2. The hot/cold epilogue ordering (which of `return 1` / `return 0` falls through vs. jumps to the tail) tracks the back-edge polarity, which the two forms emit oppositely.

Retail wants BOTH the inside-the-guard preheader AND the success-first exit order; no single source spelling reaches both at once. The `for` form scores higher (the exit-block order + single `jl` back-edge cover more bytes than the 3-4-instruction preheader-hoist delta), so prefer it. The residual is the hoisted `lea`/`add edi,N` preheader landing before the guard plus the `this`-in-register scheduling around it (retail keeps `this` live in `edi` through the guard then reuses it as the running base pointer; the recompile spills `this` to the saved slot and reloads).

```cpp
// PREFER (success-first exit order, jl back-edge; preheader hoisted ~4 instrs):
T* p = base_from_this();           // loop-invariant; MSVC floats this above the guard
for (i32 i = 0; i < m_count; i++) { ...; if (!p->Step()) return 0; p = advance(p); }
return 1;
// vs.  if (m_count>0){ T* p=...; do{...}while(++i<m_count);}  -> preheader OK but error-first exit
```

WALL (regalloc/scheduling). Evidence: CGruntzMgr::SyncOptionsState (@0x093170) — the dual-slot options-reload loop: `for` form 92.95%, `if`-guarded `do-while` 90.7%; the logic (inline strcmp, srand(time(0)), the matched/successor dual-slot unroll) is byte-exact, only the preheader placement + `this`-register scheduling differ. related: nested-if-success-deepest-error-tail.md (the non-loop exit-ordering case), void-vs-bool-return-epilogue-split.md.

# An inline-strcmp `== 0` reject emits the setcc form only when bound to a `bool` local
tags: cpp:return cpp:local cpp:branch | asm:sete asm:test | topic:codegen-idiom
symptoms: a chain of `if (strcmp(name, "X") == 0) return ...;` rejects where retail materializes each compare as `xor ecx,ecx; test eax,eax; sete cl; test cl,cl; je` but your recompile folds it straight to `test eax,eax; jne` (or the inline neg/sbb)
confidence: 8/10
variants: return-bool-via-local-setcc.md

MSVC5 expands `strcmp(a, "X")` inline to a byte loop ending in `sbb eax,eax; sbb eax,-1`
(result in eax). When you then write `int eq = (strcmp(...) == 0); if (eq) ...` the compiler folds
the comparison into a direct `test eax,eax; jne` and drops the boolean entirely. Retail instead
keeps the `xor ecx,ecx; test eax,eax; sete cl; test cl,cl; je` setcc-in-scratch shape. The
load-bearing detail (unlike return-bool-via-local-setcc.md, which works off a *call* result and a
named `int`) is that here the local must be typed **`bool`** — a `bool eq = (strcmp(...) == 0)`
forces the `sete` materialization where an `int eq` folds to a branch. The `i32 r` two-local form
(`r = strcmp(...); eq = (r==0)`) does NOT flip an `int` either; only the `bool` type does.

```cpp
bool eq;
eq = (strcmp(rec->m_name, "I") == 0);   // xor ecx,ecx; test eax,eax; sete cl; test cl,cl; je
if (eq) { return 0; }
// vs.  int eq = (strcmp(...) == 0);     // test eax,eax; jne   (no setcc)
```
```asm
33 c9        xor  ecx,ecx
85 c0        test eax,eax
0f 94 c1     sete cl
84 c9        test cl,cl
74 09        je   <fall-through>
```
STEERABLE. Evidence: UnknownClassArrays::Method_02f620 (@0x02f620) — the seven I/G/L/P/J/C/R
type-code rejects flipped from `test/jne` to the `sete` form on `bool eq` (44.9% → 48.6%), aligning
the strcmp byte stream with retail. related: return-bool-via-local-setcc.md (the call-result `int`
variant), int-to-bool-normalize.md (the inline neg/sbb form).

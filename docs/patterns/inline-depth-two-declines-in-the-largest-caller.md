# A depth-2 inline is declined in the biggest callers only — flatten it with a macro, not a split

tags: cpp:inline cpp:ctor cpp:class | asm:call | topic:codegen-idiom topic:wall
symptoms: one shared body is expanded in fifty-odd derived ctors and emitted as a real COMDAT with a
`call` in exactly the two largest ones, where retail expands it too; the affected ctors also come up
short on `__ehunwind$` funclets because the declined body owned the destructible temp
confidence: 9/10

## The shape

`CWarlord::CWarlord` and `CInGameIcon::CInGameIcon` — 0x750 B and 0x15f0 B, the two largest logic
constructors — carried

```asm
mov  ecx,esi
push edi
call <?AttachToObject@CUserLogic@@>      ; ~ctor+0x48
```

where retail expands the whole body (`[esi+0xc] = obj`, the `zBitVec` temp into `m_link.m_str`, the
`g_logicTypesRegistered` guard, three `AddLogic*` calls, `[esi+0x28] = 0x3e9`). The other 56 derived
ctors expanded it: `llvm-nm build/objdiff/base/*.obj | grep AttachToObject` named **exactly two**
objects.

The chain was `CWarlord::CWarlord` -> `CUserLogic::CUserLogic(obj, INLINE_BASE)` ->
`AttachToObject()`. cl expanded the ctor (its vptr stamps and `m_link` ctor call are in the caller)
and stopped one level short. Not a size problem — the same callee inlines fine into 56 smaller
ctors. It is the per-caller greedy budget running out at depth 2, the same mechanism as
[inline-depth-splits-one-body-into-two-shapes](inline-depth-splits-one-body-into-two-shapes.md).

## What was measured (one build each, whole tree)

| spelling | AttachToObject COMDATs | main-band fuzzy | EH funclets exact | CWarlord | CInGameIcon |
|---|---|---|---|---|---|
| inline fn, called from the inline ctor (before) | 2 | 92.82 | 1729 | 70.99 | 92.16 |
| **macro expanded in both ctor entities** | **0** | 92.77 | **1758** | **78.11** | 79.86 |
| macro outer + the inner `RegisterLogicTypesOnce` textual too | 0 | 92.77 | 1758 | 78.11 | 79.86 |
| split into two inline fns (`AttachToObject` + `ClearActGate`) | **2** | 92.73 | 1729 | 70.99 | 92.16 |
| body written directly in the inline ctor (depth 1) | 0 | 92.68 | 1758 | 78.11 | 79.86 |

Three things fall out.

**1. Splitting one inline into two does not buy depth.** Peeling the four trailing stores off into a
second inline member left `AttachToObject` at depth 2 and it was declined exactly as before — the
COMDAT count stayed at 2 and every score was unchanged. The budget is spent on the LEVEL, not on the
callee's size.

**2. The EH funclet count is the honest signal, not the fuzzy.** The declined body owned a
destructible temp, so the callers were a whole EH state short. Expanding it took `__ehunwind$??0CWarlord…$27`
from 0.00 to 100.00 (a funclet that did not previously exist) and `$2`/`$3`/`$5` from 67.5/47.0/46.7
to 100.00 — **+29 exact funclets tree-wide** for a 0.05 dip in main-band fuzzy. When a whole inline
body is missing from a caller, look at `eh_band` before believing the percentage.

**3. Flattening by hand costs more than the macro.** Writing the body straight into the inline ctor
gets the same expansion, but it leaves whatever is still nested one level down — here a three-store
`ClearActGate()` — and that alone cratered four small ctors: `CGuardPoint` and `CWayPoint`
99.67 -> 31.09, `CLevelTime` 99.67 -> 35.13, `CLightFx` 96.46 -> 22.50. Each expands it **once**, so
this is a direct counter-example to the "single-expansion callers are byte-exact under all spellings"
claim in [inline-expanded-twice-costs-a-register](inline-expanded-twice-costs-a-register.md): a
three-instruction inline function expanded once can still rotate the caller's whole register plan.
The macro flattens every level at once and has no residue.

## The recipe

1. `llvm-nm build/objdiff/base/*.obj | grep <mangled callee>` — if only a handful of objects name it
   and retail expands it there, the callee is being declined at depth 2, not mis-modelled.
2. Turn the shared body into a textual macro next to the class in its own header, and expand it in
   **every** entity that carried it (both ctor entities here — the header-inline one and the pinned
   out-of-line one in `MotionState.cpp`).
3. Do not chase the callers' fuzzy afterwards. Check the COMDAT is gone and the EH band went up.

## Not this

Do not reach for `#pragma inline_depth` — see
[msvc5-inline-depth-zero-is-the-only-live-lever](msvc5-inline-depth-zero-is-the-only-live-lever.md);
`1` is latched whole-compiland at the first definition the TU parses and `0` outlines far more than
the one call site you meant.

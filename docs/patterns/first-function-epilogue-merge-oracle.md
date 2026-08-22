# cl 5.0 does not cross-jump /GX epilogues in a TU's first function - a TU-boundary oracle

tags: cpp:eh cpp:struct msvc5:gx | asm:ret asm:jmp | topic:method topic:tooling topic:wall topic:codegen-idiom
symptoms: retail duplicates the full /GX return epilogue (restore fs:0, pops,
add esp, ret) at every `return` arm while our compile funnels the arms through
one shared exit (`jmp` + single ret); diagnose says CFG with target rets >
base rets; no source construct moves it
confidence: 9/10 for the validated direction; the mid-TU unmerged case is OPEN

## The titration (2026-08-22, real cl 5.0 SP3, /O2 /MT /GX)

A minimal CreateChildren-shaped probe (three `if (call()==0) { err-store;
return 0; }` guards after three `new` expressions) compiled ALONE in a TU
emits retail's UNMERGED shape - four full epilogues, four rets. Adding ANY
preceding emitted function body flips it to the merged single-exit shape:

| preceding construct                   | probe's rets |
|---------------------------------------|--------------|
| nothing (first body in TU)            | 4 (unmerged) |
| plain function body                   | 1 (merged)   |
| body with EH object                   | 1            |
| tiny one-ret body                     | 1            |
| COMDAT-emitted inline (address-taken) | 1            |
| multi-return body                     | 1            |
| class DECLARATION w/ inline members   | 4 (stays cold)|

Once warmed, no tested construct between bodies cools it again (class decl,
template def, #pragma pack, dynamic-init static, string table, inline+user).
The merge state is an optimizer-warmup fact, not a source fact.

## The oracle and its first production

A function whose RETAIL epilogues are unmerged while ours merge is evidence
the function OPENED its era compiland. `CDDrawSubMgrPages::CreateChildren`
(82.64, parked as a "merge coin") carried exactly this signature; splitting
DDrawSubMgr.cpp at 0x1588f0 into a DDrawSubMgrPages.cpp compiland made it
**100.00 EXACT byte-identical** and the new unit 39/40 exact at 99.90%. The
four CDDrawSubMgrPages symbols BELOW the boundary (IsLoaded/GetClassId/
??_G/??1) are header-inline vtable realizations owned by the realizing TU,
not counter-evidence.

Corroboration required before splitting: the boundary must be a class-family
boundary in the rva order, and the split must keep both files' plain blocks
ascending (the tu-order gate arbitrates).

## The open dimension

`CTileTriggerContainer::AddLogic` (0x116610) is unmerged in retail MID-TU,
surrounded by same-class methods - first-in-TU cannot explain it, and no
probed between-construct resets the state. The predicate has at least one
more input (candidate axes: the function's own shape - pointer-returning
factory arms, EH state count - or a C1 handle-state interaction). Titrate
with an AddLogic-shaped probe before applying the oracle to mid-class rows.

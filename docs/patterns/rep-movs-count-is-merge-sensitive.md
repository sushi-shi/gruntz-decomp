# A `rep movs` count mismatch is merge-sensitive, not proof of a different object count

tags: cpp:struct cpp:inline cpp:switch | asm:rep asm:movs asm:jmp | topic:tooling topic:wall topic:negative-control
symptoms: `gruntz walls aggregate-copies` reports more copy instructions on one side, but `gruntz walls diagnose` also reports duplicated inline prefixes, different call counts, or a different branch skeleton
confidence: 10/10

`rep movs*` is strong evidence that cl saw a whole-object copy at that site, but
the number of surviving instructions is not necessarily the number of source
copy expressions. C2 can tail-merge identical expanded switch arms on one side
and retain both on the other. Diagnose the function and compare the local copy
neighborhoods before changing an aggregate into fields or deleting a copy.

`CButeMgr::SetString` at 0x1732a0 is the negative control. In the historical
global-`CButeValue` transcription, two legitimate inline copy bodies retained
two `rep movsd` blocks while retail shared one. The function also differed in
call set and repeated-prefix topology, so deleting an authored aggregate to
force the count would have been wrong.

## Correction from the surviving item model

NOLF source later proved a different abstraction: nested
`CButeMgr::CSymTabItem`, a typed payload union, and
`operator=(const CSymTabItem&)`. Applying the union alone made `SetString` dip to
95.2445 and other setters fall as low as 82.0889. Nesting was codegen-flat.
Composing the const-reference assignment then made `SetString` exact at 0x3fc
bytes, 318 instructions, 33 calls, 41 branches, one return, and 54 relocations.

The assignment helper itself is exact with its authored shared trailing return,
even though C2 emits eight return epilogues. Its equal aggregate arms are merged
at the caller exactly as retail requires. This closes the historical
2-versus-1 `rep movsd` residue without deleting a copy expression and falsifies
the prior conclusion that retail selected direct `void*` storage.

The old lookup, `goto`, `else`, scope, overlay, and deleting-destructor panels
remain bounded evidence only for the superseded source hash. Their failure did
not bound the untested surviving operator/union family. The reusable conclusion
is narrower and stronger: a copy-instruction count is downstream of abstraction,
inlining, and tail merging, so it cannot by itself count source objects.

Safe reverse use:

1. Ignore current dips whose historical MAX is already 100.
2. Run `gruntz walls diagnose <rva>`.
3. If duplicated or merged blocks remain possible, treat the count only as a
   location hint.
4. Infer a missing or extra aggregate only after the surrounding blocks and
   call-set are structurally paired.

# A too-small `RVA()` size arg truncates the delinked target
tags: build:label build:delink | topic:scoring-artifact
symptoms: a tiny leaf parked at a round 50%/80%; the base carries 1–3 EXTRA trailing instructions and is otherwise identical
confidence: 10/10

**Not a codegen problem — a measurement problem.** The size argument on the `RVA()`
label bounds how many bytes the delinker carves out of retail as this function's
*target* object. If that size is short, the delinker stops mid-function: objdiff then
scores byte-perfect code against a **truncated** target, and the function parks at a
suspiciously round percentage that no source change will move.

**Detector.** Normalized target asm is a strict **PREFIX** of the base's. If the base
simply has 1–3 more trailing instructions and everything before that matches, suspect
the size arg before you suspect the reconstruction. The lost tail is almost always
exactly the last basic block — the `return 0` arm, or the trailing `ret n`.

**Fix.** Widen the size arg to the function's real byte length (its next-symbol
boundary), rebuild, and the function usually flips EXACT immediately.

Six were found tree-wide, every one an instant EXACT once resized:

| function | size arg |
|---|---|
| `IsLoaded` (×3 sites) | `0x1a` → `0x1d` |
| `CAnimatedMenuItem::SetState` | `0x14` → `0x17` |
| `GetFrameWidth` | `0xa` → `0xe` |
| `GetFrameHeight` | `0xa` → `0xe` |

Because the symptom is a stable plateau, these are easy to mis-file as `@early-stop`
codegen walls. Check the prefix property first — it costs one `llvm-objdump -dr` diff.
See also `docs/gotchas.md` (measurement traps) and the label conventions in
`CLAUDE.md`.

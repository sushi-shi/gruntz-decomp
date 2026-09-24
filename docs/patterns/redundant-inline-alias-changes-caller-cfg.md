# A redundant local in a header inline can change its caller's CFG

tags: cpp:inline cpp:local cpp:alias cpp:branch | asm:jcc asm:lea | topic:codegen-idiom topic:wall
symptoms: equal calls and ordered referents, but a large caller gains two branches after an inline helper duplicates one pointer identity
confidence: 8/10

`CGrunt::StepCompassMove` expands `CGrunt::CanCommitMove` twice. The helper
already held a `BrickzCell* tgt`, but it made a second local `tg = tgt` solely
to pass the same pointer to `RETURN_IF_DIAGONAL_ROUTE_BLOCKED`. The alias did
not model another object, owner, or lifetime.

In the pinned VC5 `gruntsteps` unit, deleting `tg` and passing `tgt` directly
changed only `StepCompassMove`: 62.6506% to 63.0219%. Calls stayed 22/22,
returns 2/2, relocations 151/151, and all 48 ordered external referents
remained in sequence. Branches fell from 131 to retail's 129; the base shrank
from 0xd08 to 0xcf0 against retail's 0xd1c. The frame remained 0x4c against
retail's 0x54, so the alias was one structural cause, not the complete match.
No other function in the unit changed score, and the full build reported no
fresh MAX regression.

Three separate controls constrain the inference. Giving the caller explicit
`sourceX/sourceY` snapshots, although those values are needed by both inline
expansions, was byte-flat: C2 folded the copies. Restoring two old movement
output initializers left the frame at 0x4c and dropped the score to 54.5728%.
Changing one arrow update from subtraction to addition of a negative constant
was also byte-flat. The caller's missing frame bytes therefore cannot be
attributed to those spellings.

When an inline-expanded caller has the right operations and referents but a
different branch skeleton, inspect helper-internal locals that merely rename
an existing value. A real-TU A/B can show whether the extra name changes
front-end lifetime/statement state even if its machine value is identical.
Keep the simpler identity only when it is the honest source model; do not add
or remove useful locals just to steer a score. Recompare from the first real
divergence because later register and tail changes can be secondary.

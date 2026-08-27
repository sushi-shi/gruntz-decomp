# Row-dispatch macro ownership controls mirrored-loop CFG

- **confidence** c9
- **tags** `cpp:macro` `cpp:loop` `cpp:branch` | `asm:jmp` `asm:mov` | `topic:codegen-idiom` `topic:cfg` `topic:regalloc`

## Symptom

Two large directional arms perform the same straight and indexed pixel
operations in opposite order. Each arm contains one runtime format dispatch.
The reconstruction has the right calls, returns, relocations, and inner loop
bodies, but cl 5.0 emits one extra unconditional jump at the first outer-loop
entry. Loop-form, declaration-order, and operation-level helper probes do not
remove it.

The missing source boundary can be the complete format dispatch. A macro that
owns only one inner loop is too low-level: its expansion changes that loop's
local identities but leaves the caller's row-entry CFG unchanged.

## RenderWarpTile evidence

`CFaderShape::RenderWarpTile` 0x00181e50 has mirrored arms which copy a straight
segment and a warp-table segment in opposite order for 8-, 16-, and 24-bit
pixels. The 88.962720% direct-expansion base emitted 83 branches against
retail's 82. Its first row carried the computed byte base in a register and
jumped over a backedge-only reload block; retail entered the common reload
block directly.

The abstraction ladder was controlled on the same unsigned-`arcSpan` source:

| ownership tested | fuzzy | result |
|---|---:|---|
| direct per-arm loops | 88.962720 | 83 branches; extra first-entry trampoline |
| macro for only the 24-bit warp loop | 81.82 | exact 24-bit bodies, outer CFG unchanged |
| macros for individual straight/warp operations at all depths | 86.75 | loop schedules converge, outer CFG unchanged |
| one row macro with a constant direction argument | 91.222046 | 82 branches; outer CFG converges |
| one row macro token-dispatched by operation order | 91.286870 | best state; 82 branches |

The final state and retail both have 82 branches, 2 returns, 3 calls, and 4
relocations. Both also contain the same two divergent 22-instruction prefixes
ending in the shared `[esp+0x14]` warp index. `gruntz walls diagnose` therefore
changes the wall from CFG to regalloc/scheduling and reports the duplicated
runs as symmetric retail structure.

The token-dispatched form matters. It expands the same row-format macro twice
with `STRAIGHT/WARP` and `WARP/STRAIGHT` operation order. A Boolean parameter
inside the macro is semantically constant and C2 removes its branch, but C1
still produces a measurably different compiler state.

## Reverse-use rule

1. Require mirrored arms that contain the same runtime dispatch and the same
   operations in opposite order. Do not infer a row macro from duplicated loop
   bodies alone.
2. Confirm the structural feature is absent from the baseline: here it was the
   sole first-entry trampoline and the 83-versus-82 branch count.
3. Test abstraction levels separately: one loop, one operation, then the whole
   dispatch. A lower score at the lower levels does not falsify the macro prior.
4. Keep the macro only when the higher boundary removes a specific retail CFG
   mismatch and preserves the operation order and semantics.
5. Re-diagnose after the change. Once calls, branches, returns, relocations, and
   repeated-prefix topology agree, stop treating the residue as CFG.

This mechanism does not settle frame allocation. The 91.286870% state has a
`0x20` frame against retail's `0x24`; its remaining unsigned x87 conversion and
slot-order residue is a separate regalloc/front-end ownership question.

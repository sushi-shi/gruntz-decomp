# OVER-MERGE is decided BEFORE layout: two identical exits merge iff both are `jcc` targets
tags: cpp:branch cpp:return cpp:if cpp:goto | asm:jcc asm:ret asm:mov | topic:wall topic:codegen-idiom
symptoms: `exit_merge_sieve --over` (base rets < target rets) on a function whose block skeleton is otherwise exact; retail has one INLINE copy of `return K` reached by fall-through plus one sunk copy reached by `jcc`, we have a single shared copy with both predecessors jumping to it; retail's inline copy stores the value from a register (`mov [esi+0x98],eax`) where our merged copy uses an immediate (`mov [esi+0x98],0`)
confidence: 9/10
variants: goto-fail-shares-one-exit-block.md, retail-duplicates-small-return-epilogues.md

The DUP-EXIT direction is steered by the source construct
([goto-fail-shares-one-exit-block](goto-fail-shares-one-exit-block.md)). The
OVER-MERGE direction is **not**, and this file says exactly why, so the next lane
does not re-derive it.

## Two facts, both measured

**1. The merge decision runs BEFORE the register peephole, so it is not a
byte-level cross-jump.**

`CGameObject::ResolveLinkedObject` 0x151b90 has two source-identical exits:

```cpp
if (m_carrierId != 0) {
    void* found = 0;
    if (MapLookupById(..., m_carrierId, found) == 0) { m_carrier = NULL; return 1; }  // A
    m_carrier = (CWwdGameObject*)found;
    return 1;
}
m_carrier = NULL; return 1;                                                            // B
```

Retail emits A as `mov [esi+0x98],eax` (6 B — eax is provably 0, it is the
fall-through of `test eax,eax; jne`) and B as `mov [esi+0x98],0` (10 B, a `je`
target). Different bytes, so a byte-comparing cross-jumper would decline anyway.
Our cl emits ONE block with the immediate.

Diagnostic: add one throwaway statement to B (`m_carrierId = 7;`). A instantly
becomes `mov [esi+0x98],eax` — retail's exact encoding — and B keeps the
immediate. **The eax reuse is a CONSEQUENCE of not merging, not a cause**, and cl
merged while both blocks still carried the immediate. So the decision is made on
the IL, before the peephole and before layout. No spelling that only changes the
final ENCODING can reach it.

**2. cl merges only when BOTH copies would be `jcc` targets.**

| cell | shape | rets | merged? |
|---|---|---|---|
| V0 | nested: `if (id != 0) { …A… } B` | 3 | **yes** — both copies are `je` targets |
| V1 | flat: `if (id == 0) { B } …A…` | **4** (= retail) | **no** — both copies are fall-throughs |
| V2 | V0 + `void* found = 0;` hoisted above the guard | 3 | yes |
| V5 | flat + `goto` to a trailing label | 4 | no, but cl flattens the label inline |

V1 splits the exits and reproduces retail's ret count exactly, *and* both copies
pick up the `mov […],eax` peephole. What it cannot do is retail's actual
arrangement — **one fall-through copy plus one SUNK `jcc`-target copy** — because
the moment the guard is nested enough to sink B, A becomes a `jcc` target too and
the pair merges. That is the whole wall, in one sentence.

Scores: V0 74.71 (baseline), V1 62.94, **V2 80.59** (kept — the hoist fixes the
argument-setup schedule, `push ecx; push eax; mov ecx,[edx+8]; add ecx,0x48`),
V5 59.76.

## Rejected spellings — measured byte-identical, do not retry

| function | edit | result |
|---|---|---|
| `CPlay::OnRButtonDown` 0xceae0 | `A && B && C && D` -> four nested positive-gate `if`s | 89.98 -> **89.98** (byte-identical) |
| `CWwdGameObjectA::BltDirtyEx` 0x1506b0 | `else if` chain -> separate `if`s each ending `return;` | 73.77 -> **73.77** |
| `CWwdGameObjectA::BltDirtyEx` | `RECT ir` hoisted to function scope ahead of `i32 rc[4]` | no change (slot order is not declaration order) |
| `CWwdGameObjectA::BltDirtyEx` | `rc[2] = rc[0] + w` instead of `ir.left + w` | no change (cl forwards the store, still folds to `right+1`) |
| `CProjectile::ScanTargets` 0xe0b10 | explicit trailing `return;` after the `do/while` | no change |

The first row also settles the regime table's "positive-gate nest" row from
[goto-fail-shares-one-exit-block](goto-fail-shares-one-exit-block.md): nesting an
`&&` chain is the SAME codegen as the `&&`, so it is not an exit-splitting lever
in either direction.

## `goto fail;` DOES reach part of this bucket — the mirror is not wholly unsolved

[goto-fail-shares-one-exit-block](goto-fail-shares-one-exit-block.md) records the
OVER-MERGE direction as "NOT solved" on the strength of two `goto` attempts that
scored worse. That is too strong. **When the site is a single guard whose
`return K` retail parks at the END of the function and ours emits inline as the
test's fall-through, the `goto` form flips the branch polarity to retail's and is
worth real points**, even though it does not sink the block:

`CProjectile::SerializeMove` 0xe0d40, `if (CMovingLogic::SerializeMove(...) == 0)
return 0;` -> `goto fail;` + a trailing `fail: return 0;`: **94.39 -> 94.82**, and
blocks B32-B38 go from permuted to `==`. Base had `jne <continue> | fall <ret0>`,
retail has `je <sunk ret0> | fall <continue>`; the goto buys the polarity.
Residue: the block is still inline rather than sunk, which is why the SERIAL_LOAD
arm still falls into the trailing `return 1` (12i + a shared 7i) instead of
carrying its own epilogue the way retail's 19i block does.

Folding the same two guards into `||` instead scores **92.08** — worse than doing
nothing, because the TOTAL regime also swallows the entry `reg == NULL` and the
`m_shadow` guard, both of which retail keeps inline. Screen by counting retail's
`return 0` copies before reaching for `||`.

## Where the cost actually shows up

Because the merge happens first, everything downstream re-flows from it:

- **branch polarity inverts.** `CPlay::OnRButtonDown` retail: `test eax,eax; je
  <skip>` then the `return 1` inline; ours `test eax,eax; jne <shared ret1>`.
- **block ORDER changes.** `CProjectile::ScanTargets` retail lays out
  `[loop][epilogue A][outlined tail][epilogue B]`; merging A into B lets cl put
  the outlined tail directly after the loop, which forces the loop's exit branch
  from `jl <head>` (fall through to A) to `jge <B>; jmp <head>` — two jumps for one.
- **a "return `<reg>`" tail appears.** `CStatusBarMgr::UpdateStatusBarTabHighlight`
  0xfe910 merges the trailing `return 1;` with a register-held result: the shared
  block is `mov eax,ebx; pop…; ret` and each of the twelve `break` sites pays a
  `mov ebx,1`. Retail has one `mov eax,1; pop…; ret` shared by all fifteen.

None of these are separately steerable; fix the merge or accept all three.

## Screening trap: the sieve over-counts target rets on switch functions

`gruntz.core.branches.table_stop()` cuts the decode at the LAST `ret`, but the
delinked target packs a switch's jump table as raw addresses that disassemble as
code — and an address whose third byte is `0xc2` decodes as `ret imm16`. So
`table_stop` lands PAST the table start and the target reads one ret too many.
`CPlay::LoadWarlordSprites` 0xd65d0 is reported OVER-MERGE `1 -> 2`; the second
"ret" is `retl $0x2` at +0x7a4, exactly where the base obj puts `$L44364` (the
table). Real counts are 1 == 1 and the function is a NON-hit. Check the ret
ADDRESS against the base's `$L` table symbol before believing an off-by-one on a
function that contains an indirect `jmp`.

related: [goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md),
[retail-duplicates-small-return-epilogues.md](retail-duplicates-small-return-epilogues.md),
[if-else-both-arms-return-is-the-or-regime.md](if-else-both-arms-return-is-the-or-regime.md),
[trailing-statement-blocks-arm-tail-sink.md](trailing-statement-blocks-arm-tail-sink.md)

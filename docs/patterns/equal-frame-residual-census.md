# The equal-frame residual census: what 505 near-matching walls actually are

tags: cpp:branch cpp:local | asm:mov asm:and | topic:wall topic:regalloc topic:tooling
symptoms: a reconstruction whose frame already equals retail's and whose diff is
small, with no obvious source difference to attack
confidence: 9/10 (whole 579-row todo queue, 2026-08-23)
variants: frame-size-sieve-and-its-false-positives.md,
arm-result-temp-controls-copies-and-shared-store.md

[`frame-size-sieve-and-its-false-positives`](frame-size-sieve-and-its-false-positives.md)
drained the `sub esp,N` vein and left the real question: of the ~500 rows whose
frame ALREADY matches, which carry a source defect and which are allocation
coins? `gruntz walls residue` answers it by reducing each diff twice before it
classifies — cancel every instruction that only MOVED, then strip register
names — and naming what survives.

## The census

| kind | rows | what it means |
|---|---:|---|
| `regname` | 149 | register rotation only |
| `displacement` | 84 | a member offset differs |
| `immediate` | 74 | a constant differs |
| `selection` | 67 | the mnemonic multiset differs |
| `schedule` | 36 | a pure permutation |
| `operand` | 32 | same mnemonics, different operands |
| `arm-copy` | 30 | retail has callee-saved `mov r,r` the base lacks |
| `extra-copy` | 13 | the inverse |
| `referent` | 15 | a relocation names another symbol |
| `none` | 5 | nothing survives the mask |

**185 of 505 (37%) are `regname` + `schedule` + `none`** — nothing a source edit
reaches. Restricted to residual <= 4 the coin share is 72/117 (62%): the closer a
row is, the more likely the remainder is a coin. Rank by KIND, not by residual.

**The arm-result-temp defect is 43 rows (30 + 13)** measured on the diff chunks,
and the direct whole-stream screen (`--arm`) finds 78 rows whose member-store
COUNT differs and 113 whose callee-saved copy count does. Both cases are real and
both are steerable — see the worked examples below.

## The false positives, i.e. what a raw diff calls a difference and is not

Each of these was measured mislabelling real rows; the tool normalizes the first
four and cannot normalize the last two.

**1. A relocated call's addend.** `call 0xe4` vs `call 0xe0` against the same
`|?GetAt@CStringArray@@...` referent is position state — anything inserted above
the call moves it. Eight rows read `immediate` on this alone, and the immediate
they "differed" on was the rel32.

**2. cl's accumulator encoding.** `and al,0xe0` IS `and ecx,0xffffffe0`: the
2-byte form when the value lands in EAX and the immediate's high bits are ones.
Seven ctors (`CTeleporter`, `CSecretTeleporterTrigger`, `CSecretLevelTrigger`,
`CInGameText`, `CExitTrigger`, `CGruntPuddle`, `CVoiceTrigger`) read `immediate`
on this one mirror; all seven are register rotations.

**3. The function's own jump table.** It decodes as junk instructions with huge
displacements and a relocation back to the function. One table (`CMapMgr::
LoadAttributes`, 0x810f0) contributed 91 residual lines by itself.

**4. Register-stripping that FORGETS the referent.** The inverse mistake: strip
registers off `mov ebx,0x0|?g_a@@3HA` and `mov ebx,0x0|?g_b@@3HA` and the two
become one tuple, so a global bound to the wrong symbol reads as a rotation.
`reg_key` keeps the `|ref` suffix; the control that caught this is in
`gruntz verify selftest -k Residue`.

**5. A negative addend into the PRECEDING array.** `&g_rasterVtxA[n - 1]`
compiles to `lea edx,[ecx*4 + g_rasterVtxA - 0x1c]`; the delinker resolves the
same address against the array that CONTAINS it and reports
`g_rasterEdgeR + 0x1bff4`. The code bytes are identical — check the arithmetic
(`g_rasterVtxA - 0x1c == g_rasterEdgeR + 0x1bff4`, both 0x2a16ec) before
believing a referent row. `ImagePolyClipRect` 0x1461b0 is the standing example.

**6. A pooled `.rdata` section hash.** `_kMsToSeconds$Sdata_rdata_<sha>` names
the SECTION's content, and the section pools other constants, so the two sides
hash differently for the same float. Two `CFader` rows.

**7. A commutative operand pair.** `mov edx,[esp+N]; imul edx,[ebp+0x1c]` against
`mov edx,[ebp+0x1c]; imul edx,[esp+N]` looks like a source term-order difference
and is not: **cl 5.0 canonicalizes the operand order of a commutative binary op
and the source spelling cannot reach it.** Measured on `imul`
(`CMapMgr::ResetCells` 0x9f5d0 `m_height * m_width` -> `m_width * m_height`, and
`CNetSession::Tick` 0xbf9e0 `seq * m_period` -> `m_period * seq`) and on `fmul`
(`CMovingLogic::AdvanceMotion` 0x16ea90, three spellings: `x * k`, `k * x` and
`x *= k` all emit `fld <k>; fmul <x>`). Every one of the three swaps was
byte-identical. That retires most of the `operand` bucket: what differs is which
value is ALREADY in a register, not which term the dev wrote first.

A related shape that also is not what it looks like: a byte-splitting run where
each side pairs a different `shr` amount with a different store slot
(`SFManager_SelectBestDevice` 0x0f8970 — `shr eax,0x10 ... mov ds:g+9,al`
against `shr eax,0x18 ... mov ds:g+0xa,al`). Follow each register through to its
STORE before reading it as a transposition: both sides put byte N at slot N.

## Worked example: the memory case, twice, both a whole score

`CSBI_ImageSetAni::Init` 0xe7980, flagged `missing-store [r+0x4c] base 1
target 2`. Retail writes `m_frameEnd` inside each `b4` arm and cross-jumps only
two of the three; the ternary shares one store after the merge. The sibling
`SetRange` (0xe7c30) is EXACT and spells both range selections as an explicit
if/else, so the idiom is the class's own. Converting both blocks: **96.43 ->
99.86**, residue a two-instruction rotation.

`CTileTriggerContainer::AddLogic` 0x116610, flagged `[r] base 1 target 2`. The
receiver itself was the ternary —
`(logicType == TRIGID_TIME_TRIGGER_23 ? m_list2 : m_list1).AddTail(obj)` — so cl
inlines `AddTail` once after the merge. An if/else over two statements (which is
what a dev writes for a container anyway): **83.86 -> 97.84**.

The screen that finds more: `walls residue --arm`, then grep the flagged
function's body for a member-assigning ternary. Over the 62 memory-case rows in
the todo queue only 3 carried one, so the population is small — but each is
worth double digits.

## Reading it

    gruntz walls residue --todo                     the census
    gruntz walls residue --todo --kind immediate,displacement,referent
    gruntz walls residue --arm --todo               the arm-result worklist
    gruntz walls residue --show <rva>               NET and EXACT residual

A row that lands in `regname`, `schedule` or `none` is PARKED: its source is not
what differs. A row in `immediate`, `displacement` or `referent` gets the
false-positive list above applied by hand before it is called a bug.

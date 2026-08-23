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

The first census (left column) was taken before four of the mirrors below were
normalized; the right column is the same 505 rows after. Every row that moved
moved from an actionable bucket into a coin bucket, and each move was
adjudicated by hand against the retail bytes first.

| kind | first | now | what it means |
|---|---:|---:|---|
| `regname` | 148 | 153 | register rotation only |
| `displacement` | 84 | 75 | a member offset differs |
| `selection` | 66 | 77 | the mnemonic multiset differs |
| `immediate` | 75 | 72 | a constant differs |
| `schedule` | 36 | 36 | a pure permutation |
| `operand` | 32 | 32 | same mnemonics, different operands |
| `arm-copy` | 30 | 30 | retail has callee-saved `mov r,r` the base lacks |
| `none` | 6 | 15 | nothing survives the mask |
| `extra-copy` | 13 | 13 | the inverse of `arm-copy` |
| `referent` | 15 | **2** | one side names a symbol the other never names |

**204 of 505 (40%) are `regname` + `schedule` + `none`** — nothing a source edit
reaches. Restricted to residual <= 4 the coin share is 62%: the closer a row is,
the more likely the remainder is a coin. Rank by KIND, not by residual.

**The `referent` bucket is DRAINED**: 15 rows down to 2, and both survivors are
the `.rdata` section-hash artifact of §6. Thirteen were false positives (§8, §9)
and one was a real defect — the only wrong claim in the whole bucket, and it was
in the MODEL rather than the C++ (§ below).

**The arm-result-temp defect is 43 rows (30 + 13)** measured on the diff chunks,
and the direct whole-stream screen (`--arm`) finds 78 rows whose member-store
COUNT differs and 113 whose callee-saved copy count does. Both cases are real and
both are steerable — see the worked examples below.

## The false positives, i.e. what a raw diff calls a difference and is not

Each of these was measured mislabelling real rows. The tool normalizes §1-4 and
§8-11; §5-7 and §12-13 it cannot see, so apply them by hand before calling a row
a bug.

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

**8. A `$E` dynamic-init helper, under two non-names.** `push 0x0|$anon_data_
<sha>_0` against `push 0x0|FUN_004183b0` is ONE address that NEITHER side has a
name for: objdiff names our obj's unnamed symbol by content hash, and the
delinker, with no channel claiming the address, prints `FUN_<va>`. Every one is
the `atexit` registration of a function-local static's destructor — which the
label rules deliberately never name, because `_$E<n>`'s suffix is emission-order
state. **This was ELEVEN of the fifteen `referent` rows** (`CBattlezDlgCustom::
DoDataExchange`, five `CButeMgr` getters, both `CImage::RenderFrame*`,
`CDDrawChildGroup::TickKillCues`, `CWarlord::NotifyFortUnderAttack`). Confirm by
`gruntz sema rva <target addr>`: the row reads `src_dyninit <owner>` as a losing
claim. Now normalized to `?unnamed`.

**9. A referent COUNT difference on a symbol both sides name.** Retail reads
`g_gameReg` twice back-to-back where cl keeps one copy
(`CTriggerMgr::LoadGruntResurrectTuning` 0x7be60: `mov ecx,ds:g_gameReg; mov
edx,ds:g_gameReg`), and retail reads `g_p01` five times inside the k-loop where
cl hoists it (`CShadeTableCache::FlashTable` 0x14df40). That is CSE and
rematerialization, not identity: **both sides NAME the symbol.** The test now
compares symbol SETS over the whole stream, so only a symbol the other side
never names anywhere is reported.

**10. The forced zero displacement.** `[ebp+0x0]` addresses what `[ecx]`
addresses — EBP as a ModRM base cannot encode without a displacement byte, and
neither can a scaled index with no base (`lea edi,[ebp*8+0x0]`). Register-
stripping erases which base register it was, so the forced zero read as a member
offset. **About a quarter of the `displacement` bucket was this one encoding**
(`CGrunt::BeginAttack`, three `CImage::Blit*`, `CKitchenSlime::LoadSprites`'s
`lea ecx,[ecx+0x0]` 3-byte NOP, `CRezImage::DecodePcxData`, ...). Now folded in
`reg_key`, where the register choice is already what is being erased.

**11. `add r,-K` against `sub r,K`.** Same value, same three bytes, and cl picks
between them on its own: measured going BOTH ways against retail in one tree
(`CSpotLight::CSpotLight` 0xb1200 has `add eax,0xffffffe0` where retail has `sub
eax,0x20`; `CStaticHazard::CStaticHazard` 0xfb7a0 has `sub ecx,0x7` twice where
retail has `add ecx,0xfffffff9`). Bidirectional, so it is not even a source
lever. Now mirrored.

**12. A sub-object base pointer against a flat offset.** `mov ecx,[esi+0x38];
add ecx,0x1a0; mov eax,[ecx+0x28]` against `mov eax,[esi+0x38]; lea ecx,[eax+
0x1a0]; mov eax,[eax+0x1c8]` — 0x1a0 + 0x28 == 0x1c8, the SAME field. The tool
cannot normalize this (register-stripping cannot prove the sum), so do the
arithmetic: `CMenuSparkle::AdvanceAnim` 0xae2a0 and `CStaticHazard`'s
`[eax+0x1cc]` vs `add eax,0x1a0; [eax+0x2c]` are both this.

**13. cl folding a parameter inside the arm that tested it.** `CPlay::
LoadCursorSprites` 0xd0120 stores `mov [esi+0x2f8],0x66` where retail stores the
`frame` PARAMETER — because that arm is guarded by `cursor == CURSOR_FLAILING
GRUNT`, and `CURSOR_FLAILINGGRUNT` is 0x66. The store is semantically identical;
what differs is that retail's arms share one `m_levelId = frame; return 1;`
tail, so `frame` is not a known constant there. Before reading a base-only
constant as a wrong value, check whether it EQUALS the arm's own guard.

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

## Worked example: the one real defect, and it was in the MODEL

`CButeMgr::Save` 0x171640 read `referent`: our obj calls
`??1strstream@@UAE@XZ`, the delinked target calls `??1iostream@@UAE@XZ`. The C++
was right — `strstream source(...)` IS a stack local there — and the LABEL was
wrong. 0x169be0 is 0x20 bytes that write `??_7strstream@@6B@` (0x5f0394) into the
virtual `ios` base and then tail-`jmp` the real `??1iostream` at 0x16c950, which
is separately anchored HIGH; its scalar-deleting caller 0x169aa0 runs the same
vbase teardown; and it is the exact structural twin of `??1istrstream` (0x1697c0,
stamps 0x5f0374, tails `??1istream`) and `??1ostrstream` (0x1699c0, stamps
0x5f0384, tails `??1ostream`), both already labelled with that very note.

`config/retail/functions_static_libs.tsv` carried BOTH names on the row, and
`gruntz.model.pick` breaks a same-channel tie alphabetically, so `??1iostream`
won and the correct `??1strstream` sat as its alias. Dropping the wrong row is
the whole fix. **The signature to reuse: a `referent` row whose two names are a
DERIVED class and its BASE is a FID ambiguity, not a source bug — read
`gruntz sema rva` for a losing claim on the same address before touching C++.**

## Reading it

    gruntz walls residue --todo                     the census
    gruntz walls residue --todo --kind immediate,displacement,referent
    gruntz walls residue --arm --todo               the arm-result worklist
    gruntz walls residue --show <rva>               NET and EXACT residual

A row that lands in `regname`, `schedule` or `none` is PARKED: its source is not
what differs. A row in `immediate`, `displacement` or `referent` gets the
false-positive list above applied by hand before it is called a bug.

The hit rate is the thing to plan around: **forty rows of the three actionable
buckets were adjudicated against the retail bytes and ONE was a defect** — and
that one was a label, not C++. The bucket's value is not that it finds many
bugs; it is that a row it clears is a row nobody needs to open again, so every
false positive removed from it is worth more than another pass over it.

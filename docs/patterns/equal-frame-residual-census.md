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
normalized; the middle column is the same rows after; the right column is the
current todo queue with every mirror below normalized - the §12 arithmetic and
§14-16 landed in two separate lanes and only the right column has both. Every
row that moved moved from an actionable bucket into a coin bucket, and each move
was adjudicated by hand against the retail bytes first.

| kind | first | +§8-11 | +mirrors | +§20 | what it means |
|---|---:|---:|---:|---:|---|
| `regname` | 148 | 153 | 159 | 177 | register rotation only |
| `displacement` | 84 | 75 | 61 | 61 | a member offset differs |
| `selection` | 66 | 77 | 90 | 87 | the mnemonic multiset differs |
| `immediate` | 75 | 72 | **68** | **68** | a constant differs |
| `schedule` | 36 | 36 | 39 | 40 | a pure permutation |
| `operand` | 32 | 32 | 32 | **14** | same mnemonics, different operands |
| `arm-copy` | 30 | 30 | 31 | 31 | retail has an extra `mov r,r` the base lacks - 21 of 31 are REGALLOC, see below |
| `none` | 6 | 15 | 19 | 21 | nothing survives the mask |
| `extra-copy` | 13 | 13 | 13 | 13 | the inverse of `arm-copy` - 7 of 13 are REGALLOC |
| `subobject` | - | - | 2 | 2 | one side splits an address, the other folds it |
| `referent` | 15 | **2** | **2** | **2** | one side names a symbol the other never names |

(the `first` column was 505 rows and `now` is the larger todo queue, so read the
columns as a shape, not a subtraction.)

**238 of 516 (46%) are `regname` + `schedule` + `none`** - nothing a source edit
reaches. Restricted to residual <= 4 the coin share is 62%: the closer a row is,
the more likely the remainder is a coin. Rank by KIND, not by residual.

**The `referent` bucket is DRAINED**: 15 rows down to 2. Thirteen were false
positives (§8, §9) and one was a real defect — the only wrong claim in the whole
bucket, and it was in the MODEL rather than the C++ (§ below). The two `.rdata`
section-hash rows of §6 are now normalized too, and the two survivors are §14
boundary pointers whose target symbol is an unclaimed file-local static.

It briefly read 4 while §20 was landing, and chasing those two arrivals found a
false positive in the bucket itself (§21): `canon_ref` folds a referent to an
absolute address only when the operand carries exactly one absolute token, so
our one-token `mov eax,ds:0x2bf674` became `@0x2bf674` while retail's two-token
`cmp DWORD PTR ds:0x0,0x0` kept `?g_logicTypesRegistered@@3HA`. One datum, two
spellings, reported as an identity difference by the bucket whose whole job is
identity. Referents are now compared by ADDRESS, and both rows moved to
`selection`, where their real difference - a load-and-compare against a direct
memory compare - is what they are.

**The coin classification was AUDITED, and it holds.** `regname` is the biggest
bucket and the campaign parks all 177 rows on the tool's word, so the word was
checked: every row was re-read with `walls semdiff`, which is order-sensitive and
compares referent SEQUENCES rather than the multiset the classifier uses. Fifteen
rows flagged. All fifteen resolve, and none is a defect:

* **six are pure MOVES** - delete-and-insert of the SAME symbol, i.e. one global
  read scheduled at a different point (`LoadScrollSpeedOptions`,
  `LoadGruntDecayConfig`, `AnnounceVersion`, `ApplyGameOptions`, and both
  `CLightFxRender` palette builders on `g_rDown`);
* **seven are already catalogued** - `BuildBootyWalkingGruntz` is §8's `$E`
  helper under two non-names plus §14's one-past-the-end addend, and the six
  ctors (`CTeleporter`, `CSecretTeleporterTrigger`, `CSecretLevelTrigger`,
  `CInGameText`, `CExitTrigger`, `CVoiceTrigger`) are §2's accumulator mirror;
* **one needed reading by hand**: `CMulti::AdvanceGameFrame` reports 39 referents against
  38, which looks like a count difference and is not. Both sides reference
  `g_frameTicks` exactly twice, load and store. cl hoists the NEXT global's load
  into the middle of the increment (`mov edx,g_frameTicks; mov eax,g_timer32;
  inc edx; cmp eax,ebp; mov g_frameTicks,edx`) where retail completes the `++`
  first and loads afterwards. Interleaving, not identity.

So a `regname` row is a coin in the strong sense: same instructions, same
symbols, same values. What this audit does NOT prove is store PAIRING - register
stripping erases which value reaches which field - and that question has its own
sieve: `walls storescan --values` over the same queue returns 2 transposition
candidates in 595 rows, and both are a base-pointer shift (every offset moves by
one constant, 0x2c0 and 4) rather than a swap. Check the SUM before believing a
storescan row.

**The `arm-copy`/`extra-copy` buckets are NOT an arm-result-temp worklist —
they are a regalloc bucket with a misleading name.** An earlier revision of
this file said "the arm-result-temp defect is 43 rows (30 + 13)"; that claim is
withdrawn. The whole bucket was read on 2026-08-23 (44 rows: 31 `arm-copy` +
13 `extra-copy`), and the tool's own printed evidence disqualifies most of it
before any hand-reading:

| what the row prints | arm-copy | extra-copy | what it is |
|---|---:|---:|---|
| a callee-saved `mov r,r` in the exact residual | 10 | 6 | the documented signature |
| **`target has []` / `base has []`** | 20 | 5 | the deficit is a move whose DESTINATION is scratch (or an 8-bit register) — `raw()` filters to 32-bit callee-saved destinations, so an empty list means the signature is absent |
| a SELF-move (`mov edi,edi`) | 1 | 2 | cl's 2-byte encoding used as a pad, or the zero-carrier register |

So **28 of 44 rows (64%) never carried the signature at all.** The three empty
rows read by hand are unambiguous, and none is an arm result:

* `?GetGruntzDriveLetter@@YADXZ` 0x01ffe0 — retail keeps the byte in `al` and
  copies `mov bl,al`; base loads straight into `bl`. Which register the byte
  lives in.
* `?OnExit@CPlay@@UAEXXZ` 0x0cb400 — retail `mov eax,ds:<global>` then
  `mov ecx,eax`; base loads straight into `ecx`.
* `?GetTilePos@CGrunt@@QAE?AUCoord@@XZ` 0x031c70 — the whole 7-instruction
  residual is `eax`/`edx` swapped plus the copy that swap needs.

Three of the sixteen that DO carry the signature were read too, and they are
allocation as well: `?Draw@CChatBox@@...` 0x182f90 is a load-compare-keep whose
compare operand retail puts in EAX; `?RebuildSelectionList@CTriggerMgr@@...`
0x07cc60 is a whole-body `ebp`↔`ebx` rotation with the rotation's entry copy;
`?LoadFinishLevelSprite@CTriggerMgr@@...` 0x07c3d0 is §-zero-carrier (retail
holds zero in `edi` and spells `cmp x,edi` / `push edi` for the immediate).
With the eight rows sampled earlier that is **11 of 44 hand-adjudicated and 0
confirmed arm-result-temp defects.**

The real arm-result worklist is the whole-stream screen (`--arm`): 78 rows whose
member-store COUNT differs and 113 whose callee-saved copy count does, and its
two worked examples below are real and steerable. Rank from there, not from
`--kind arm-copy`.

## The false positives, i.e. what a raw diff calls a difference and is not

Each of these was measured mislabelling real rows. The tool normalizes §1-4,
§8-12, §14-16, §20 and §21 (§5 and §6 fall out of §14's arithmetic); §7, §13
and §17-19 it cannot see, so apply them by hand before calling a row a bug.

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
0x1a0]; mov eax,[eax+0x1c8]` — 0x1a0 + 0x28 == 0x1c8, the SAME field.
Register-stripping cannot prove the sum, so the tool now runs the ARITHMETIC:
it draws every K from a pointer-materializing `lea r,[…+K]` / `add r,K`
anywhere in either stream and asks whether shifting one side's displacements
by K reproduces the other's. A run of two or more reclassifies to
`subobject`; a LONE pair only gets the arithmetic printed in the note,
because one offset against one other offset is exactly the wrong-field
signature this bucket exists to find and some constant always reconciles two
numbers. Reclassified: `CImage::RenderImage` 0x153470 (a four-dword RECT at
+0x20 written through `lea eax,[esi+0x20]`), `CGruntzMgr::RecomputeViewScale`
0x8f7f0 (the same shape READ, `[esi+0x10..0x1c]` against `lea`+`[0,4,8,c]`),
`CAniAdvanceCursor::Deserialize` 0x15ca70. Noted-only, all hand-confirmed:
`CNetSession::SendRecord` 0xbfeb0 (`lea [r+r+0x3b0]` then `[r+0x4]` against
`[r+r+0x3b4]`), `CBattlezMapConfig::ClaimCellFromRow` 0x30730
(`lea [r+r*8+0x188]` then `[r+0xd4]` against `[r+r*8+0x25c]`),
`CSBI_WellGoo::Setup`, `CTriggerMgr::CellHitTest`,
`CGruntzMgr::SyncOptionsState` (the 0x238-stride walk),
`CImageSet1/2::Parse`. `CMenuSparkle::AdvanceAnim` 0xae2a0 and
`CStaticHazard`'s `[eax+0x1cc]` vs `add eax,0x1a0; [eax+0x2c]` were the
original two.

**14. LEA standing in for the arithmetic instruction.** cl takes the
three-operand `lea` for exactly one reason: the destination is not the source,
i.e. the source register stays live. That is the register pick `reg_key`
already erases, so `lea esi,[eax+0x1]` IS `inc esi` and `lea ecx,[eax+eax*1]`
IS `add eax,eax` — but un-mirrored the lea's displacement read as a member
offset the other side never touched (`CGruntzMgr::RandRange`,
`UpdateMgrScroll`, `CRandomAmbientSound::Update`,
`CLightFxRender::DrawBorderRaw`, `_zvec::GrowTo`,
`CTileTriggerLogic::Classify`). The flag side effect does NOT fold: `lea` sets
none, so retail's extra `test r,r` survives and the row reads `selection`,
which is honestly what it is.

**15. cl's 3-byte `lea r,[r+0x0]` alignment pad** is the same padding a `nop`
is (`CKitchenSlime::LoadSprites`, `CGrunt::ArrivalRecycle`,
`CMultiStartDlg::Watchdog`, `CPlay::LoadWarlordSprites`). Rule 10 has already
shortened it to `lea r,[r]`; it now folds to `nop`.

**16. The accumulator form of an absolute memory operand.** `a1 <addr>` prints
`mov eax,ds:0x0`; the general `8b 0d <addr>` prints
`mov ecx,DWORD PTR ds:0x0`. Same load of the same global, and cl takes the
short form only when the value lands in EAX — the §2 mirror again, on a
different operand (`CGrunt::ArrivalRecycle`, `CMultiStartDlg::Watchdog`,
`ScrollDialog`, `CPlay::ResetPlayState`).

Together 14-16 moved **21 rows out of `displacement` (112 -> 91)** and 15 out
of `operand` (32 -> 17); every one landed in `regname` or `selection`.

**13. cl folding a parameter inside the arm that tested it.** `CPlay::
LoadCursorSprites` 0xd0120 stores `mov [esi+0x2f8],0x66` where retail stores the
`cursorId` PARAMETER — because that arm is guarded by `cursor == CURSOR_FLAILING
GRUNT`, and `CURSOR_FLAILINGGRUNT` is 0x66. The store is semantically identical;
what differs is that retail's arms share one `m_cursorId = cursorId; return 1;`
tail, so `cursorId` is not a known constant there. Before reading a base-only
constant as a wrong value, check whether it EQUALS the arm's own guard.

**14. A one-past-the-end array pointer, i.e. §5 in the FORWARD direction.** A
loop's end sentinel `&g_lut16[0x100]` is an ADDRESS, and the delinker resolves
an address against whatever symbol STARTS there while cl names it against the
array it came from: `cmp esi,0x200|g_lut16` against `cmp esi,0x0|g_rUp`, and
0x283ca0 + 0x200 == 0x283ea0 exactly. The immediate the two sides "differ" on
is the relocation's ADDEND, which is half of the reference, not a program
constant. **Seven rows of the `immediate` bucket were this one shape**
(`CDDSurface::Blit168` 0x13fbb0, `DecodeRun` 0x143cf0, `Decode` 0x144b30,
`DecodePcxData` 0x1457a0, `CBootyState::ShowLevelCompleteMessage` 0x1c9d0,
`LoadGameAssetNamespaces` 0x18830, `BuildBootyWalkingGruntz` 0x1b450) and every
one sat at 99.5-99.98% — one instruction from exact, with nothing in the C++ to
fix. Now normalized: a DATA reference whose symbol the Model can resolve is
canonicalized to the absolute address `symbol + addend` names, so §5's negative
addend folds by the same arithmetic. Two names for one address can only cancel
references that ARE the same address, so the fold cannot hide a wrong claim.
Two survivors remain, where retail's side names an unclaimed file-local static.

**15. cl's accumulator encoding on a HIGH byte register.** §2's twin: `and
dh,0xef` masks bits 8..15 and touches nothing else, so it IS `and edx,
0xffffefff`. `CGruntzMapMgr::LoadAttributes` 0x810f0 read `immediate` on this
one instruction. (`and edx,0xf` is NOT `and dh,0xf` — the byte form is only a
mirror of the 32-bit mask that leaves the OTHER three bytes alone.)

**16. `lea r,[r+K]` against `add r,K`.** §11's twin, and the same argument
holds: identical value, and the only thing that distinguishes them is that LEA
does not write flags, which is cl's choice and not the source's. Measured going
BOTH ways against retail in one tree — `CTriggerMgr::Load` 0x7abc0 and
`RemoveCellRecord` 0x78260 carry the `lea` where retail has the `add`, while
`CBootyState::EnterState` 0x18d30, `MoveLettersByDir` 0x19b90,
`CSBI_GruntMachine::BuildResourceTabStatusBar` 0xe8a70 and `CPlay::
OnRButtonDown` 0xceae0 carry the `add` where retail has the `lea`. The doubling
form `lea esi,[esi+esi*1]` / `add esi,esi` is the same mirror (`CFecFile::
ReadArchive` 0x17b5f0, `CStatusBarMgr::UpdateChipGrinderStatusBar` 0x1076a0).
**Only the dst == base form is mirrored**, and that restriction matters more
than it sounds: `lea ecx,[eax+0x10]` KEEPS eax, which `add ecx,0x10` cannot, so
folding it would erase a real lifetime distinction under register stripping.
`CBootyState::EnterState` 0x18d30 and `BuildResourceTabStatusBar` 0xe8a70 are
exactly that shape and stay in the bucket; only rows where cl picked the same
register on both sides fold (`CTriggerMgr::Load` 0x7abc0). `lea ecx,[ecx+0x0]`
is cl's 3-byte NOP and is left alone too.

**17. A jump table whose arms OVERLAP.** cl lets one case enter another arm's
BODY: in `CRollingBall::Update` 0xb0140 the S arm's table entry points at
0xb05fa, the second instruction of the SW arm (`add [esi+0x78],-0x10; add
[esi+0x7c],+0x10`), so eight logical directions live in six code blocks. The
base emits each arm separately and therefore materializes the constants more
times than retail does — read as an `immediate` count difference. Verified
against the table itself (36-byte index at 0xb0ca4, 15 dwords at 0xb0c68): our
`MovingDeathTileSetAId` case labels and their (dx, dy) pairs match retail arm
for arm. The signature: the surviving constants are the SAME values, only the
COUNT differs, and the function has an indirect `jmp`.

**18. Constant re-association across an induction variable.** `CStatusBarMgr::
LoadTabSprites` 0x102250 has `add eax,0x12c` and `add eax,0xf` where retail has
`add eax,0x13b` — and 0x12c + 0xf == 0x13b. Before reading two base constants
as wrong, check whether they SUM to the target's one.

**19. Loop strength reduction turning a trip count into a byte bound.**
`CMultiStartDlg::DoDataExchange` 0xc20a0 counts `ebx` 0..4 with a separate
`add ebp,0x238` cursor where retail counts `ebx` 0, 0x238, ... and tests `cmp
ebx,0x8e0` — 4 * 0x238 == 0x8e0. Same loop, one induction variable instead of
two.

**20. A zero parked in a register, spent where an immediate would go.** cl
materializes `xor ebx,ebx` ONCE when a function needs a zero repeatedly across
calls - a callee-saved register survives the call, an immediate would have to be
re-encoded at every site - and then spends it: `cmp eax,ebx` for `test eax,eax`,
`mov DWORD PTR [edi+0x4],ebx` for `mov DWORD PTR [edi+0x4],0x0`, `push ebx` for
`push 0x0`. Whether it bothers is register PRESSURE, and it was measured going
both ways in one tree - `CStaticHazard::LoadAttributes` 0xfc1a0 and
`RunCustomWorldDialog` 0x3ad90 carry the immediate where retail carries the
register, `CMulti::PollSession` 0xb95f0 and `CPlay::LoadPlayState` 0xd8060 the
reverse - so it is noise, not a source lever. Folding it took `operand` from 32
rows to 14 and `selection` from 90 to 85.

The fold is a DATAFLOW fact and cannot be done on the text: register-stripping
turns a real `cmp esi,edi` between two live values into the same `cmp r,r`. The
tool proves the register zero with a fixpoint over the function's own control
flow. A linear walk is wrong here and was measured being wrong - an early-return
epilogue's `pop ebx` retires a zero that the blocks after it still hold, because
they are not reached through that epilogue - and an instruction whose only
predecessor is an indirect jump starts from the empty set, so a jump-table arm
never inherits a zero the other arms happen to hold.

`CStaticHazard::LoadAttributes` is the whole class in one byte: 264 of its 265
instructions already agreed, including every OTHER use of retail's zero
register, and the single survivor was the `!= NULL` after `HitTestCell`, where
retail spends `cmp eax,ebx` and we emit `test eax,eax`. Residual 1 to 0.

**21. One datum under two referent spellings.** `canon_ref` folds a DATA
reference's addend into the referent as an absolute address, but only when the
operand carries exactly ONE absolute token - a guard that exists so a constant
stored THROUGH a relocated address stays in `immediate` instead of moving to
`referent`. The guard makes the fold asymmetric between two spellings of the
same access: `mov eax,ds:0x2bf674` has one token and becomes `@0x2bf674`, while
`cmp DWORD PTR ds:0x0,0x0` has two and keeps `?g_logicTypesRegistered@@3HA`.
0x2bf674 IS that symbol. Referents are compared by address for exactly this
reason; a bucket that reports identity must not be sensitive to which
instruction form reached the datum.

## Worked example: the memory case, twice, both a whole score

`CSBI_ImageSetAni::Init` 0xe7980, flagged `missing-store [r+0x4c] base 1
target 2`. Retail writes `m_frameEnd` inside each `b4` arm and cross-jumps only
two of the three; the ternary shares one store after the merge. The sibling
`SetRange` (0xe7c30) is EXACT and spells both range selections as an explicit
if/else, so the idiom is the class's own. Converting both blocks: **96.43 ->
99.86**, residue a two-instruction rotation.

`CTileTriggerContainer::AddLogic` 0x116610, flagged `[r] base 1 target 2`. The
receiver itself was the ternary —
`(logicType == TRIGID_TIME_TRIGGER_23 ? m_timedLogics : m_idleLogics).AddTail(obj)` — so cl
inlines `AddTail` once after the merge. An if/else over two statements (which is
what a dev writes for a container anyway): **83.86 -> 97.84**.

The screen that finds more: `walls residue --arm`, then grep the flagged
function's body for a member-assigning ternary. Over the 62 memory-case rows in
the todo queue only 3 carried one, so the population is small — but each is
worth double digits.

## Worked example: the one real defect the DISPLACEMENT bucket has produced

`CMultiStartDlg::CommitLatencyOption` 0xc5020 read `displacement`: retail has
`mov ecx,[ecx+0x60]` — `m_slotList` — that our obj never emits, immediately
before a call, and never uses the value again. A dead load is not something
cl 5.0 emits: it is the receiver of a **`__thiscall` member whose body reads
no member**. 0x38220 takes its four arguments off the stack
(`mov eax,[esp+8]; mov ecx,[esp+4]`) and `ret 0x10`, which a member with four
stack args does exactly as a free `__stdcall` does — so the callee's own bytes
are identical either way and only the CALLER could show the difference.
`CLatencyList::SelectItem`, the write side of the same packed combo item data,
was already a member. Making `GetSelItemData` one took the caller
92.08 -> EXACT and left the callee EXACT.

**The signature to reuse: a member load whose value is never consumed, sitting
immediately before a `call`, is a `this` we are not passing** — i.e. a free
function in our model that retail declares inside a class. It is invisible to
the call-set test (same callee), to the CFG test (same skeleton) and to the
frame test (same frame); only the displacement bucket sees it.

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

A row that lands in `regname`, `schedule`, `subobject` or `none` is PARKED: its
source is not what differs. A row in `immediate`, `displacement` or `referent`
gets the false-positive list above applied by hand before it is called a bug.

The hit rate is the thing to plan around: **sixty-five rows of the three
actionable buckets have been adjudicated against the retail bytes and TWO were
defects** - one a label, one a calling convention, and neither a wrong member
offset. A second full pass over the `immediate` bucket alone (25 further rows
read against the disassembly, 2026-08-23) found no semantic defect at all and
six more false-positive classes, §14-§19. The bucket's value is not that it
finds many bugs; it is that a row it clears is a row nobody needs to open again,
so every false positive removed from it is worth more than another pass over it.

What the `displacement` bucket is actually made of, after §10, §12 and §14-16
have been taken out, is three recurring shapes that no source edit reaches:

* a **rematerialization count** - both sides read the same member set and only
  which copy stays in a register differs (`CEyeCandy`'s ctor, `CreateDemoMover`,
  `CUFO`'s ctor before it was fixed, `FillSessionList`, `BoxesOverlap`,
  `LoadVehicleGruntSprites`, `CGrunt::LoadGruntDecayConfig2`, which recomputes a
  64-bit subtraction retail keeps and cl CSEs);
* a **cross-jump merge degree** - one side shares an exit the other duplicates,
  which renumbers every branch displacement (`CSBI_WellGoo::Setup`);
* a **spill-slot renumbering** from one extra 4-byte local
  (`CAniAdvanceCursor::Deserialize`, `CImage::RenderImage`).

**What `immediate` rows still look actionable, after §14-§19.** The residue that
survives is dominated by two shapes that are NOT constants: §12's sub-object
base pointer (`mov r,[r+0x40]` against `add r,0x40; mov r,[r]` - retail names
the sub-object with a pointer local where we spell the flat member), and the
`/GX` EH state variable, whose `mov DWORD PTR [esp+K],<small int>` run shifts by
one whenever the set or order of destructible temporaries differs
(`CGruntzMgr::ChangeState` 0x8fab0, `CStatusBarMgr::BuildTabzDialog` 0x10a340,
`CDDrawSurfaceMgr::SnapshotChildren` 0x156020 and `RestoreChildren` 0x156530,
`CGruntzMgr::TransitionState` 0x8b960). Neither is a wrong value: the first is a
type/locals question and the second is an EH-object-set question, and both are
better read from `walls diagnose` than from the constant census.

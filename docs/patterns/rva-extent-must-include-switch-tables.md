# An `RVA()` extent must include the function's own switch TABLES

tags: data:objdiff cpp:switch | asm:jmp | topic:scoring-artifact topic:tooling topic:wall
symptoms: a function whose disassembly is byte-identical to retail - same instruction
  count, every branch displacement equal - is stuck below 100%; the base `.text` COMDAT is
  16-256+ bytes longer than the `RVA()` size; the function contains a sparse/dense `switch`
  lowered to `jmp dword ptr [reg*4+<addr>]`; `llvm-objdump -t` on the base obj shows `$L…`
  LABEL symbols (storage class 6) at or just past the claimed size
confidence: 10/10

## Mechanism

MSVC5 emits a switch's **jump table** (and, for a sparse switch, the **byte index table**)
into the *function's own* `.text` COMDAT, immediately after the last instruction, 4-byte
aligned. They are part of the section, not of `.rdata`.

objdiff sizes our base symbol from the COMDAT, so it measures **code + tables**. The
delinker sizes the target symbol from the `RVA()` claim. If the claim stops at the last
instruction, objdiff lines our table bytes up against **retail's next function** and scores
them zero. Nothing about the reconstruction is wrong — the claim is short.

## The fix

Claim the tables:

```cpp
RVA(0x0013cff0, 0x35c)   // code only     -> 94.88%
RVA(0x0013cff0, 0x3a0)   // code + tables -> 100.00% EXACT
```

## Finding them mechanically

The safe test is *"does our code end where retail's does"*, and the `$L` labels answer it
without any disassembly:

    base COMDAT size  -  RVA() size  >  15          # more than alignment padding
    first `$L` label at offset >= RVA() size,
        and (that offset - RVA() size) <= 3         # only the table's 4-byte alignment

When both hold, our code length equals retail's, so the new extent is exactly the base
COMDAT size. Read the COMDAT size from the section header and the `$L` offsets from the
COFF symbol table (storage class 6, `IMAGE_SYM_CLASS_LABEL`); `llvm-objdump -t` renders
both. Watch two traps: a section can hold more than one function (bound the search at the
next function symbol in that section), and `RVA()` is sometimes **indented** inside a
`namespace`, so a `^RVA\(` grep misses it.

## Measured (2026-08-01 sweep, whole tree)

43 functions matched the test. **43 of 43 improved, nothing else in the tree moved**, and
`claim-extents` still reported no overlapping ranges. 23 went straight to EXACT:

| before | function |
|---|---|
| 33.33 | `CPlay::BuildGruntTypeNameTable` |
| 53.59 | `CMultiBootyState::BuildPowerupIconKeys` |
| 62.07 | `SetShadeDescr` |
| 68.18 | `CDDrawWorkerHost::SerializeDispatch` |
| 68.75 | `CDDrawShadeBlit::Select` |
| 69.57 | `winapi_118b50_OutputDebugStringA` |
| 70.59 | `CBattlezDlg::GetCtrlA..D`, `CMultiStartDlg::GetCtrlA..E` (9 fns) |
| 73.39 | `CVoiceManager::ResolveGruntVoiceGroup` |
| 79.31 | `CLatencyList::Dispatch` |
| 89.36 | `CMultiBootyState::GetWarlordName` |
| 90.91 | `CInGameIcon::HandleInput` |
| 92.55 | `CGameLevel::EditDispatch` |
| 94.77 | `CInputState::SelectDevices` |
| 95.05 | `CTileTriggerContainer::AddSwitchLogic` |
| 96.26 | `CDDrawDeviceManager::ReportError` |
| 97.50 | `CBootyState::FormatHudText` |
| 94.88 | `CGameApp::GameWindowProc` (the one that surfaced it) |

Overall 3214 -> 3240 exact, MAX fuzzy 80.59 -> 80.91.

**A 33% function reading as byte-exact the moment the extent is right** is the measure of
what this artifact costs. Do not read a low score on a `switch`-heavy function as evidence
that the body is wrong until you have checked the extent.

## The one measured exception: check `assert_relocs` after the sweep

Extending a claim re-carves the whole unit, and on one function that made the **delinker
drop an unrelated relocation**. `CDroppedObject::ActA` @0xc7090 goes 92.55 -> 96.45 at the
correct extent (0x21b -> 0x230), but then `CDroppedObject::UserLogicVfunc5` @0xc7350 loses
the DIR32 on its `mov eax,ds:_g_engineFrameDelta` — the delinked byte becomes
`a1 00 00 00 00` with no reloc at all, so the reference vanishes and `assert_relocs`
reports our base as referencing a global *"retail never does"*, which is backwards.
Reproduced in both directions, nothing else in the unit changed.

So: **run `gruntz verify assert-relocs` after an extent sweep**, and if a unit you
touched grows a WRONG whose base side is provably right, revert that one claim. Reloc
fidelity outranks match %. It is a delinker bug, not a source one — the points are waiting
behind a fix.

## Bound

The `<= 3` slop is load-bearing *for the base-side test*. When the gap between the claim and
the first `$L` is larger, our code may be a different length from retail's, and extending the
extent would then claim the wrong retail bytes. This says nothing about `.rdata` string or
float constants, which live in their own sections and are scored separately.

## Read RETAIL instead — the base-side test has two blind spots

The `$L`-offset test asks *our* COMDAT where the table is. That fails whenever **our source
does not emit the table**, which is exactly the population most worth finding, and it stops at
the FIRST table. Scan the retail bytes directly instead — no base obj involved:

1. from the claim end, skip to the next 4-aligned offset;
2. count DWORDs that fall inside `[imagebase+rva, imagebase+rva+size]` — that run is the jump
   table (a pointer back into the function's own code is not a coincidence);
3. then count bytes `< run length` — the sparse index table;
4. **repeat**, because a function can carry several switches;
5. stop when step 2 finds nothing; the extent is where you stopped.

Confirm each hit before touching it: the function body must literally contain the table's VA
(the `disp32` of its own `jmp [reg*4+TBL]` / `mov r8,[reg+IDX]`), i.e.
`struct.pack("<I", imagebase + tbl_rva) in body_bytes`. That is a 4-byte needle in a few
hundred bytes and it turns "these DWORDs look like pointers" into proof of ownership. In the
2026-08-01 second sweep it confirmed **65 of 65** candidates and rejected none, and every
extent it produced landed exactly on `0x90`/`0xcc` padding or exactly on the next function
start (`CDDrawSurfaceChildA::SetGeometry`'s table runs right up to 0x164650 with no padding).

What the base-side test missed, measured on the same tree after its 43-function sweep:

* **`_EngStr_RenderText` @0x115930 — filed above as "21 bytes of slop, the residual really is
  code".** It is not. Retail has a 5-entry table at 0x115a8c followed by 31 index bytes;
  347 → 399 takes it **61.40 → 90.55**. The slop was large because our body is a different
  length, which is true and irrelevant — the table is still ours.
* **three multi-table functions**, invisible to a first-table test:
  `CStatusBarMgr::UpdateStatusBarTabHighlight` (3 tables, +158) and
  `CGruntzMgr::HandleCommand` (3 tables, +1551, 71.16 → 94.70).
* **functions emitting no table at all**, where the short claim was *inflating* the score:
  `CAniAdvanceCursor::Find` and `SerializeSwitchLogic` both went DOWN at the correct extent. That
  is the claim telling the truth, not the extension being wrong — see
  switch-empty-arms-dedup-before-jumptable.md, which took both to 100 EXACT.

Second sweep: 65 confirmed tree-wide, 38 applied (26 in units another lane held, 1 held back
by the `ActA` delinker bug above), started-units MAX 81.73% → 82.30%.

**A drop after extending is therefore not a signal to revert** when the body references the
table VA. Judge the extension by that reference, and judge the score afterwards.


## Third detector (2026-08-04): decode the `jmp` itself

Both sweeps above scan forward from the claim end and *guess* which dwords are a table. A
strictly cheaper and exact test decodes the dispatch instruction instead: MSVC5 always emits
`ff 24 <mod=00,rm=100,SIB scale=2>` + `disp32`, i.e. `jmp DWORD PTR [reg*4+TABLE]`, so a scan
of the claimed BYTES for `ff 24 8d/85/95/9d/b5/bd` yields the table's VA directly. Any table
VA `>= rva+size` is a short claim, with no false positives and no base obj needed - it also
catches the multi-table case for free (collect every match, not the first).

Residue after the two sweeps: **28 more functions** (10 in the Bute/Gruntz lane, 18 elsewhere),
including four the "slop" heuristic could not see because the table starts within 2 bytes of
the claim end (`CTileTriggerLogic::Tick` @0x110c10, table at 0x111a50, claim ended 0x111a4f).
Measured on the ten: CRollingBall::Update 54.9 -> 83.1, CTileActionEvent::Process 58.9 -> 89.8,
CTileTriggerLogic::Tick 84.1 -> 94.9, CTriggerMgr::WireTileSwitchLogic 76.8 -> 88.4,
CButeValue::CopyValue 91.3 -> 100 EXACT.

related: bss-symbol-size-inference-hole.md, delinker-jumptable-dup-symbol-undercount.md

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
| 73.39 | `CGruntSpawnConfig::GetButeSlot` |
| 79.31 | `CLatencyList::Dispatch` |
| 89.36 | `CMultiBootyState::GetWarlordName` |
| 90.91 | `CInGameIcon::HandleInput` |
| 92.55 | `CGameLevel::EditDispatch` |
| 94.77 | `StateMgrBZ::Build` |
| 95.05 | `CTileTriggerContainer::AddSwitchLogic` |
| 96.26 | `CDDrawPtrCollections::GetErrorString` |
| 97.50 | `CBootyState::FormatHudText` |
| 94.88 | `CGameApp::GameWindowProc` (the one that surfaced it) |

Overall 3214 -> 3240 exact, MAX fuzzy 80.59 -> 80.91.

**A 33% function reading as byte-exact the moment the extent is right** is the measure of
what this artifact costs. Do not read a low score on a `switch`-heavy function as evidence
that the body is wrong until you have checked the extent.

## Bound

The `<= 3` slop is load-bearing. When the gap between the claim and the first `$L` is
larger, **our code is a different length from retail's** — that is real matching work, and
extending the extent then claims the wrong retail bytes. Those stay on the ordinary
worklist (`_EngStr_RenderText` @0x115930 is one: 21 bytes of slop, and the residual really
is code). Equally, this says nothing about `.rdata` string or float constants, which live
in their own sections and are scored separately.

related: bss-symbol-size-inference-hole.md, delinker-jumptable-dup-symbol-undercount.md

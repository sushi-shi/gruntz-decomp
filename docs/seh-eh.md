# Exception handling (SEH / C++ EH) in GRUNTZ.EXE

The one-shot `seh_extract.py` analysis script that produced these findings has been
retired (it was a discovery aid, not part of the matching loop); the results below are
preserved here. Recover it from git history (`git log --diff-filter=D -- scripts/seh_extract.py`)
if you need to re-run the extraction.

## The x86 / VC5 EH model
Gruntz is x86, so there is **no `.pdata`/`.xdata`** (that's x64). MSVC 5.0 C++ exception
handling is the `__CxxFrameHandler` + **FuncInfo** scheme:

- `_s_FuncInfo` (0x1C): `magic@0, maxState@4, pUnwindMap@8, nTryBlocks@C, pTryBlockMap@10,
  nIPMap@14, pIPtoState@18`. Magic = **`0x19930520`** (only this value present → pure VC5;
  no 0521/0522).
- `_s_UnwindMapEntry` (8): `int toState; void(*action)()` — destructor/unwind funclets.
- `_s_TryBlockMapEntry` (0x14): `tryLow, tryHigh, catchHigh, nCatches; HandlerType* pHandlerArray`.
- `_s_HandlerType` (0x10): `adjectives; TypeDescriptor* pType; dispCatchObj; void* addressOfHandler`.

Struct layouts taken from the VC5 media (`EXSUP.INC` pins `MAGIC_NUMBER1=0x19930520`;
`EXCEPT.INC`, `TRNSCTRL.H`) — `ehdata.h` itself isn't on the disc (only `frame.obj`) —
and empirically verified: every `pUnwindMap`/`pTryBlockMap` field lands in the `.reloc`
HIGHLOW set, unwind actions resolve into `.text`, catch types resolve to readable RTTI.

> **Key limitation:** x86 VC5 `FuncInfo` has **`nIPMap`/`pIPtoState` = 0** — it carries
> **no IP-to-state map**, so EH data does **not** encode function extents. SEH is therefore
> *not* an independent function-boundary source for this target (boundaries come from
> `.reloc` targets + RTTI vtables + the Ghidra coverage-recovery pass). Owning-function
> attribution is best-effort: 790/973 (81%) via a `push offset <thunk>` prologue xref → an
> *approximate* extent `[start, next func start)`.

## Findings
- **973** valid `_s_FuncInfo` structures.
- **2,638** funclets recovered (2,618 unwind/destructor + 20 catch). All 972 reachable
  thunks (`mov eax,&FuncInfo; jmp __CxxFrameHandler`) converge on `__CxxFrameHandler`
  at RVA `0x11eea0`.
- **Cross-validation: 100% match** with Ghidra's 2,638 `Unwind@`/`Catch@` funclets
  (0 extras, 0 misses) — independent bit-for-bit confirmation of both methods.
- **`__except_handler3`** (C-style `__try/__except` scope tables): **not used** in this
  build (no recurring handler-push pattern; zero `except_handler3` refs in Ghidra's export).

## Where `try`/`catch` is used (the useful part)
**17 of 973** EH frames carry explicit `try/catch` blocks; the other **956 are
unwind-only** (objects with destructors needing stack unwind, no catch). Every catch is an
**MFC exception class**:

| Catch type | Sites | Note |
|---|--:|---|
| `CException*` | 16 | the MFC base exception |
| `CArchiveException*` | 1 | MFC serialization (`CArchive`) |
| `CUserException*` | 1 | paired with `CException*`; MFC user-cancel |

The 17 `FuncInfo` records sit contiguously in `.rdata` (RVAs `0x200158`–`0x202290`) — an
MFC-heavy region (app init / file / serialization error paths). Two of them have **2** try
blocks; the rest have 1. (Per-site detail in `build/seh/seh_segments.json`.)

**Decomp use:** a function whose `FuncInfo` has a try block must be written with the
matching `try { … } catch (CException*) { … }` structure; the 956 unwind-only frames just
have stack objects with destructors (no source `try`). This is a structural constraint that
helps match those functions. **TODO:** attribute the 17 try/catch frames to their owning
`.text` functions (they're in the 183 unattributed set — resolve via the FuncInfo-pointer /
`__CxxFrameHandler` xref).

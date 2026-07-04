# Caller audit (WS5) — target vs base call-graph diff

`python -m gruntz.analysis.caller_audit` builds **two** call graphs over the
~3209 `RVA()`-labelled functions and diffs them to surface edges that cap the
match %.

- **Target graph** — the retail truth. One linear scan of `GRUNTZ.EXE`'s `.text`
  for direct `call`/`jmp rel32` (E8/E9) into a labelled RVA, each site resolved
  to its containing function (ghidra fstarts) + unit. (`xref.py`'s machinery run
  over every function at once.)
- **Base graph** — what our reconstruction emits. Each src TU carrying `RVA()`
  functions is compiled to LLVM IR with clang (`labels.emit_ir`, the label
  build's clang-cl invocation), and every `call`/`invoke` to a mangled symbol we
  know is an edge. clang's MS mangling reproduces the VC5 symbols, so the IR
  `@"?callee@..."` names key straight into `symbol_names.csv`; both endpoints map
  to RVAs, name-independently comparable to the target graph.

Diffing yields three worklists (written to `config/caller-audit-{promote,orphan,access}.md`):

| worklist | meaning | fix |
|---|---|---|
| **promote** | a fn with a reconstructed cross-TU (different unit) retail caller, whose class is declared only in a `.cpp` (free fn: no `include/` prototype) | move the class to a shared header so the caller can make a real typed call |
| **orphan** | a retail edge caller→callee the base graph does **not** emit — the analysed caller makes no such call in our IR (stub/@early-stop, or fakes it via a view/raw-offset) | wire the real call; `[real]` marks a matched body (not `src/Stub/`) that emits other calls but not this one — fix first |
| **access** | a fn with a reconstructed cross-**class** cross-TU retail caller whose symbol access (mangled name: A–H private / I–P protected / Q–X public) is not public | make it public — a non-public target mangles to a symbol the caller never emits, so the reloc never pairs (the `CLightEffect::Setup` = Q lesson) |

`promote` and `orphan` are two views of the same block: a class stuck in a `.cpp`
forces its cross-TU callers to omit the call. `access` is a lint that currently
passes (0) — the access lesson is already applied binary-wide.

## Usage

```
python -m gruntz.analysis.caller_audit                 # all 3 -> config/*.md + summary
python -m gruntz.analysis.caller_audit --kind orphan   # one list to stdout
python -m gruntz.analysis.caller_audit --csv           # machine-readable to stdout
python -m gruntz.analysis.caller_audit --no-ir         # skip the clang base pass (fast; promote+access)
python -m gruntz.analysis.caller_audit --check 0x1804a0 # explain one function (rva or mangled name)
```

Run inside `nix develop` (needs clang / `llvm-undname` + `$GRUNTZ_EXE`). The base
IR pass compiles ~506 TUs in parallel (~7 s). `src/Stub/ApiCallers.cpp` is not in
the clangd compdb, so its callers are not IR-analysed (their edges are reported as
unknown, never as false orphans).

Spot-checks: `CLightEffect::Setup` (0x1804a0, caller `CFaderMgr::Add` cross-TU),
`PaletteLerp::Tick` (0x1480a0, caller `PalCtx` cross-TU), `ProjectWallQuad`
(0x1471d0, caller `CFileImage::DecodeThunk` cross-TU) all land in **promote** +
**orphan**.

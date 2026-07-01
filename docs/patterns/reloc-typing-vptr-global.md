# Reloc-typing: vptr/global/IAT store operands differ (code bytes match) — scoring artifact
tags: cpp:ctor cpp:member | asm:mov asm:call | topic:scoring-artifact topic:tooling
symptoms: ~99.5% fuzzy with 0 structural diffs, diffs only in 4-byte operand slots, ??_7…/__imp__/DAT_ names
confidence: 9/10

**UPDATE (2026-07, measured — this is largely OBSOLETE):** The vtable/global **DIR32** half is
FIXED — a NAMED DIR32 data referent (vtable via `config/vtable_names.csv`, global via `DATA()`,
pooled string via the `coff_oracle`) now scores exact; an unnamed one is fixed by naming it. The
**`__imp__`/import + `REL32`-call half is a NON-issue**: objdiff MASKS `REL32` call/branch reloc
names and `call [disp32]` import operands, so those never cap the score (measured: renaming 5909
call/thunk relocs + adding 788 `__imp__` relocs → 0.0% delta). Net: this is **no longer a standing
wall**. A code-byte-exact function that still caps is an UNNAMED DIR32 data referent (→ name it) or
a REAL codegen diff — never a call/import/thunk-name artifact. See `docs/matching-patterns.md` §215
and `docs/wall-instructions.md`.

A function that stores a vtable pointer (`mov [this], offset ??_7Class@@6B@`), a global, or
calls an import (`FF15 [IAT]`) plateaus at ~99.5% **fuzzy** but is byte-exact in CODE. The
delinker emits the target operand against a differently-named symbol (`DAT_*`/`__imp__*`/the
Ghidra name) than `cl`'s real symbol, so the 4-byte operand slot mismatches even though the
instruction bytes are identical. CONFIRM by `llvm-objdump -dr` base vs target: the only diffs
are reloc operand slots. Then it is MATCHED (orchestration §6/§8) — do not chase the phantom.

WALL (scoring artifact, code already correct). Our analog of vostok's
`delinker-masks-base-immediates`. Evidence: every ctor/error-formatter; CDirectDrawMgr 96.2% (10 reloc slots).

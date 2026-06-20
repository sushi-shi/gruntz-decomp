# Reloc-typing: vptr/global/IAT store operands differ (code bytes match) — scoring artifact
tags: cpp:ctor cpp:member | asm:mov asm:call | topic:scoring-artifact topic:tooling
symptoms: ~99.5% fuzzy with 0 structural diffs, diffs only in 4-byte operand slots, ??_7…/__imp__/DAT_ names
confidence: 9/10

A function that stores a vtable pointer (`mov [this], offset ??_7Class@@6B@`), a global, or
calls an import (`FF15 [IAT]`) plateaus at ~99.5% **fuzzy** but is byte-exact in CODE. The
delinker emits the target operand against a differently-named symbol (`DAT_*`/`__imp__*`/the
Ghidra name) than `cl`'s real symbol, so the 4-byte operand slot mismatches even though the
instruction bytes are identical. CONFIRM by `llvm-objdump -dr` base vs target: the only diffs
are reloc operand slots. Then it is MATCHED (orchestration §6/§8) — do not chase the phantom.

WALL (scoring artifact, code already correct). Our analog of vostok's
`delinker-masks-base-immediates`. Evidence: every ctor/error-formatter; CDirectDrawMgr 96.2% (10 reloc slots).

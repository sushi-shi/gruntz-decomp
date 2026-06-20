# Pattern index (MSVC 5.0 /O2 — Gruntz)

One line per pattern: `- [title](file.md) — cN — tags — symptoms` (cN = confidence /10).
Sorted by primary tag so families cluster. Grep this file by tag (`cpp:switch`,
`cpp:eh`, `topic:wall`) or by symptom token, then read only the hits. Schema +
how-to-search: [`README.md`](README.md). Authoring: new pattern = new file + one
line here, SAME commit.

WALL vs STEERABLE: `topic:codegen-idiom`/`topic:flags` = a source spelling closes the
diff; `topic:wall`/`topic:scoring-artifact` = code already correct / no source form —
stop chasing (orchestration §2a).

- [Tiny inline ctor (bare vptr store) has no source-recoverable standalone COMDAT](comdat-inline-ctor-no-standalone.md) — c7 — cpp:ctor cpp:inline | asm:mov | topic:comdat topic:wall — 7-byte mov [this],&vftable; ret, no mov eax,ecx, no null guard
- [Reloc-typing: vptr/global/IAT operands differ, code bytes match](reloc-typing-vptr-global.md) — c9 — cpp:ctor cpp:member | asm:mov asm:call | topic:scoring-artifact topic:tooling — ~99.5% fuzzy 0 structural diffs, ??_7/__imp__/DAT_ operand slots
- [Multi-member ctor EH state-numbering base shifts on incomplete TU](eh-state-numbering-base.md) — c7 — cpp:ctor cpp:eh | asm:mov | topic:wall topic:eh — body byte-identical, residue in __ehfuncinfo state ids, ~89-95%
- [Destructible stack local forces the /GX frame → flags="eh"](gx-frame-destructible-local.md) — c9 — cpp:eh cpp:local | asm:mov | topic:flags topic:codegen-idiom — whole __except frame MISSING under base flags, CString/CFileIO/RegistryHelper local
- [Lookup out-param zero-init SINKS past arg pushes — scheduling wall](outparam-zeroinit-scheduling.md) — c8 — cpp:local | asm:mov asm:push | topic:wall topic:scheduling — identical multiset 2-3 instrs permuted per Lookup(name,&out), 80-95%
- [Retail pins 0/1 in a callee-saved register — regalloc wall](zero-register-pinning.md) — c7 — cpp:local | asm:xor asm:mov | topic:wall topic:regalloc — 1-instr phase shift through every =0/=1 store, ebx/edi/ebp zero-reg
- [Subsystem GetErrorString/ReportError is static __cdecl, not __thiscall](geterrorstring-static-cdecl.md) — c9 — cpp:method cpp:static cpp:switch | asm:push asm:call | topic:codegen-idiom topic:archetype — push×3; call; add esp,0xc; ret, Ghidra says void __thiscall(void)
- [rand() % var won't reproduce MSVC5's divisor-guard modulo-peel](rand-modulo-peel.md) — c8 — cpp:modulo cpp:rand | asm:idiv asm:test asm:jcc | topic:wall — ~180 extra TRGT instrs, test divisor,divisor; je guard before idiv
- [Inline .rdata jump-table data scored mismatched, code byte-exact](jumptable-data-overlap.md) — c7 — cpp:switch | asm:jmp | topic:scoring-artifact topic:tooling — low % on big switch, jump-table base reloc, $L labels
- [Sparse switch → cmp/je tree (100%); dense → jump table (~96%)](switch-cmpje-tree-vs-jumptable.md) — c8 — cpp:switch | asm:cmp asm:je asm:jmp | topic:codegen-idiom — same archetype, one sibling 100% other ~96%, cmp/je ladder vs jmp [tbl+eax*4]
- [switch case BODIES emitted in source-case order — reorder to retail layout](switch-cases-source-order.md) — c8 — cpp:switch | asm:jmp | topic:codegen-idiom — structure matches but case blocks in wrong .text order, fuzzy jumps after reorder
- [Per-function codegen needs the TU's COMPLETE fn-set in ascending-RVA order](tu-completeness-rva-order.md) — c8 — topic:tu-layout | topic:wall topic:codegen-idiom — whole unit 70-99% 0 exact, unit spans scattered RVAs

# Dense `switch` jump-table: MSVC separate `$Lnnn` symbol vs delinker inline-at-fn

**Symptom.** A dense `switch` (a contiguous run of cases) that MSVC lowers to a
jump table — `jmp DWORD PTR [eax*4 + <table>]` — plateaus around **75–80%** even
when the function's *code* is byte-for-byte identical to retail. `gruntz status`
shows the function stuck; nothing in the code differs.

**Cause (confirmed `llvm-objdump -dr` base vs target).** The instruction stream
matches exactly. The mismatch is the **jump-table layout**, two parts:

1. MSVC emits the table as its **own symbol** (`$L209`) placed after the function
   body in the same section, and the `jmp`'s operand is a `DIR32` reloc to that
   `$Lnnn` symbol (displacement `0x00`).
2. The **delinker normalizes** the retail table (which lived in a far-off
   switch-data region, e.g. `0x43796c`) by **copying it inline into the function
   symbol** at `fn+0x5c`, so the target `jmp`'s reloc is `DIR32 → <fn>` with
   displacement `0x5c`.

So the `jmp`'s reloc targets differ (`$L209` vs `fn`), the table bytes attach to a
different symbol (unpaired `$L209` vs inline), and the 2-byte align filler before
the 4-aligned table differs too — MSVC writes `8b ff` (`mov edi,edi`), retail/the
delinker shows `90 90`. Together ≈ the missing 20%.

**Verdict: `topic:wall` — the code is correct; stop chasing.** This is a
delinker/objdiff symbol-layout artifact, not a source-recoverable difference; no
C spelling makes MSVC fold the switch table into the function symbol or change the
filler bytes. `@early-stop` at the plateau once `llvm-objdump` confirms the code
bytes match.

**Don't confuse with a real fix.** Before blaming the table, make sure the *code*
is byte-exact — the common real bug is the bool tail: `return Fn() != 0;` lowers to
the `neg/sbb/neg` idiom, but a multi-case switch whose cases each abort wants the
shared-`return 1` branch form `case k: if (Fnk()) break; return 0; ... } return 1;`
(jumps every case's success to one `mov eax,1; ret`). That fix alone took
`CBzKindDispatch::Dispatch` (0x37910) 42% → 79%; the remaining 21% is this artifact.

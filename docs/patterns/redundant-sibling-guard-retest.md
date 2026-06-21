# Redundant `if(flag)` re-test across calls — write SIBLING blocks, not nested
tags: cpp:branch cpp:local | asm:test asm:jcc | topic:codegen-idiom
symptoms: structure matches but recompile DROPS a second `test reg,reg; je` the target keeps; flag held in a callee-saved reg (esi/edi/ebx) across intervening calls; ~84% plateau that jumps to 100% on de-nesting

When a boolean local `flag` is computed once, then the function does work guarded by it,
calls into externals, and guards a LATER block by the SAME `flag`, MSVC5 /O2 keeps `flag`
pinned in a callee-saved register (esi/edi/ebx) across the calls and **re-emits the
`test reg,reg; je`** for the second block. It does NOT prove the second test redundant —
because the value lives in a register across opaque calls.

The trap: if you write the second guard NESTED inside the first (so the compiler statically
knows `flag != 0` there), MSVC drops the inner `test/je`, the flag migrates to a caller-saved
reg (eax), the whole register allocation shifts, and you plateau (~84%):

```cpp
if (mismatch) {            // gate
    m_570 = 1;
    if (wasConnected) { Report(...); PostMessageA(...); }
    if (mismatch) { SendStat(0x418,1); Sleep(250); }   // BAD: inner test eliminated
}
```

Write the two guards as SIBLINGS so the second test survives — the dev wrote it that way and
it pins `mismatch` in esi across the report calls (CNetMgr::HandleVersionCheck @0xbd0b0 →
100% byte-exact):

```cpp
if (mismatch) {
    m_570 = 1;
    if (wasConnected) { Report(...); PostMessageA(...); }
}
if (mismatch) { SendStat(0x418,1); Sleep(250); }   // GOOD: separate test reg,reg; je
```

STEERABLE. Read the target: two `test <samereg>,<samereg>; je <end>` bracketing a call block,
the reg being callee-saved (pushed in prologue) = de-nest the guards in source.

related: zero-register-pinning.md (the reg the flag lands in is a separate coin-flip);
switch-cases-source-order.md (also "source statement structure drives .text layout").

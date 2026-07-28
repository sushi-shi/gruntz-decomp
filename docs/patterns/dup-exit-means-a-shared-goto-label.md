# `--branches` says DUP-EXIT: the guards that reach retail's merged exit share ONE `return` — spell it as `goto fail;`
tags: cpp:branch cpp:return | asm:jmp asm:ret | topic:codegen-idiom
symptoms: `gruntz sema disasm <rva> --branches --diff` prints `base N ret(s) | target M ret(s)  DUP-EXIT`; the asm diff shows an extra `xor eax,eax; pop…; ret` block on our side and a branch whose polarity is inverted (`jns` where retail has `js`)
confidence: 9/10
variants: positive-gate-enables-shrink-wrap.md, identical-return-epilogue-tailmerge.md

`positive-gate-enables-shrink-wrap.md` covers the case where wrapping the body in a
positive `if` merges the exit. The other half: when several *guards scattered through the
body* all reach the SAME retail exit while our early `return`s each get their own
epilogue, the source shared one return statement — and in C that is a label.

```cpp
// retail: two `js` guards and the last range test all jump to ONE `xor eax,eax; ret`,
// while the `name == 0` and `len > 16` guards keep their own - which no arrangement of
// plain `return 0;` statements reproduces.
if (name == 0) return 0;
i32 len = static_cast<i32>(strlen(name));
if (len < 0)  goto fail;          // -> the shared exit
if (len > 16) return 0;           // -> its own
if (name2 != 0) {
    i32 len2 = static_cast<i32>(strlen(name));
    if (len2 < 0)  goto fail;
    if (len2 > 64) goto fail;
}
/* body */
return 1;
fail:
    return 0;
```
Read the merge structure straight off the target: every branch that lands on the same
address is one `goto`; every branch with its own epilogue is a `return`.

STEERABLE. `CGameInfo::SetNames` @0x118040: the plain two-`if` spelling emits the right
guards but 6 rets against retail's 4 and scores **86.1**; the same guards with a shared
`fail:` label are **100 EXACT**. `GameOptionsDlgProc` @0x36410 97.6 -> 99.1 from the
same reading (its last checkbox block does not `return FALSE` - it falls through into the
unrouted-notification `return FALSE`, which is why retail has one exit fewer).

# Near-consecutive switch → running-subtract chain; an if-else-if on `==` won't reproduce it
tags: cpp:switch cpp:branch | asm:sub asm:dec asm:je asm:jcc | topic:codegen-idiom
symptoms: retail has `sub imm,eax; je; dec; je; sub imm,eax; jne` where your source emits three independent `cmp imm,eax; je`; ~45-50% on an otherwise-correct dispatch on a returned status code

When a function dispatches on a value with a handful of case labels that are CLOSE together
(e.g. 0x4cd, 0x4ce, 0x4ea — i.e. +0, +1, +0x1c), MSVC5 /O2 lowers a **`switch`** to a
running-subtract chain that consumes `eax` in place:

```
sub  $0x4cd, %eax      ; case 0x4cd: eax==0
je   case_4cd
dec  %eax              ; case 0x4ce: eax now 0
je   case_4ce
sub  $0x1c,  %eax      ; case 0x4ea: eax now 0
jne  default
```

An **if-else-if** chain written on equality (`if (r==0x4cd) … else if (r==0x4ce) … else if
(r==0x4ea) …`) instead emits three INDEPENDENT `cmp imm,eax; je` against the original value —
structurally different, ~half-matching even when every handler body is byte-correct.

FIX: spell it as a `switch (r) { case 0x4cd: … case 0x4ce: … case 0x4ea: … }`. Distinct from
[switch-cmpje-tree-vs-jumptable](switch-cmpje-tree-vs-jumptable.md) (sparse→cmp/je tree on the
ORIGINAL value, dense→jump table): the subtract-chain is the in-between lowering MSVC picks for a
small set of near-consecutive labels, and it is what an `if/else if` on `==` fails to reproduce.

WALL→STEERABLE: a source spelling (`switch`) closes it. Evidence: CNetMgr::OnDropPlayer
(0xbc110) went 47%→100% the instant the `if/else-if` dispatch on the MULTI_DROPPLAYER result
(0x4cd/0x4ce/0x4ea) was rewritten as a `switch` — same handler bodies, only the dispatch
lowering changed. (Watch also that each case's call receiver is the RIGHT object: there the
0x4ea handler calls FindPlayerById on `m_524` and ResetCmdBuffers on `m_520`, not `this`.)

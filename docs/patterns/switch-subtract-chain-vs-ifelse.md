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

**Single-constant rule of thumb (the type-tag getter family):** for a one-value test on a tag,
the SPELLING dictates polarity + block layout —
- **`== 0`** (or `== 4`) → write `if (rec->type == 0) return v;` → `cmp [eax],0; jne mismatch`
  (success INLINE, mismatch jumped to tail).
- **single NON-ZERO constant** (`== 1`) → write `switch (rec->type){ case 1: return v; }` →
  `mov ecx,[eax]; dec ecx; je success` (loads type to a reg, success AT TAIL). An `if (==1)`
  emits the WRONG-polarity `cmp [eax],1; jne` (~57%).
- **multi-type** (0|3, 0|2) → `switch` (loads type ONCE into a reg, sub/cmp ladder).
Rule: single non-zero or multi-way ⇒ `switch` (load-to-reg, case-at-tail); `==0`/`==4` ⇒ `if`
(cmp-mem, success-inline). Evidence: CButeMgr Get{Int,Dword,Float,Double,String}[Def] getters.

WALL→STEERABLE: a source spelling (`switch`) closes it. Evidence: CMulti::ShowDropPlayerDialog
(0xbc110) went 47%→100% the instant the `if/else-if` dispatch on the MULTI_DROPPLAYER result
(0x4cd/0x4ce/0x4ea) was rewritten as a `switch` — same handler bodies, only the dispatch
lowering changed. (Watch also that each case's call receiver is the RIGHT object: there the
0x4ea handler calls FindPlayerById on `m_netMgr` and ResetLatencies on `m_session`, not `this`.)
Also CNetMgr::OnOutOfSync 3-way result `switch(r){0x4cc;0x4cd;default}` = the same sub/dec ladder.

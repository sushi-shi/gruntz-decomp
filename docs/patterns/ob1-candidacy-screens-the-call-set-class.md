# An INLINE/CALL-SET class with an UNDEFINED callee is a duplicated CALL SITE, not a budget
tags: cpp:inline cpp:call | asm:call asm:jmp | topic:codegen-idiom topic:tooling
symptoms: `walls diagnose` says INLINE/CALL-SET with a REPEATED-SITE DELTA, one
side calls the same callee N+1 times, inline-model, /Ob1, budget deficit,
finish-the-caller, `spec JSON missing`
confidence: 10/10 (derived, not estimated - the obj's own symbol table)

`gruntz walls diagnose` reports `INLINE/CALL-SET` whenever the two sides' call
multisets differ, and CLAUDE.md points that class at
`gruntz walls inline-model --gap`. But a call-count delta has TWO causes and
only one of them is an inline decision:

1. the callee was **expanded** on one side and **called** on the other - a real
   /Ob1 budget question;
2. the same callee is **called twice on one side and once on the other** - a
   late **tail-merge / cross-jump** decision, where two arms end in an identical
   suffix on one side and not the other. Nothing about inlining is involved.

`REPEATED-SITE DELTA: target 14, base 15` is shape (2) far more often than (1),
and reading it as (1) sends you into budget arithmetic that cannot move it.

## The screen is exact and takes no measurement

`/O2` implies `/Ob1`: cl 5.0 never auto-inlines an unmarked function at any
definition position. So a callee that is an **UNDEFINED external in the obj that
calls it** is not an inline candidate at ANY budget - there is no body in the
TU to expand. Only a callee this TU already emitted as **its own COMDAT** is a
live candidate the budget declined.

    gruntz walls inline-model --gap <rva>

names the delta from the same normalized pair `diagnose` reads and screens each
site against our own base obj's symbol table:

    [budget-gap] ?PlaceObjectFull@CTriggerMgr@@QAEHHH@Z  [triggermgr]  rva 0x078a50
      ?LoadCursorSprites@CPlay@@QAEHHH@Z
          target 14, base 15   (base calls MORE)
          NOT A CANDIDATE: UNDEFINED external in this obj - out of line, and /O2
                           implies /Ob1, so no budget expands it.

    [budget-gap] ?TransitionState@CGruntzMgr@@QAEHW4GameStateId@@HHH@Z  [gruntzmgr]
      ??0ClockInterval@CPlay@@QAE@XZ
          target 0, base 7   (base calls MORE)
          CANDIDATE: this TU emits its own COMDAT for it ...

Two rows, two different answers, neither of which needs a `cb` estimate.

## What to do with shape (2)

Find which two arms retail shares and we duplicate (or the reverse), and read
the ONE instruction that makes the tails differ - the merger is exact, so a
single mismatched instruction in the suffix is enough for it to decline.
`CTriggerMgr::PlaceObjectFull` 0x78a50: retail shares the vehicle preview's two
`LoadCursorSprites`, both arms ending `push <arg>` and jumping to a common
`mov ebp,<world>; mov ecx,ebp; call`. Ours materialises `world` in EBP inside
the true arm BEFORE its call and loads the receiver straight into ECX in the
false arm, reloading EBP after - so the suffixes are not identical and cl
correctly emits both.

The terminator lever from
[retail-duplicates-small-return-epilogues](retail-duplicates-small-return-epilogues.md)
works the other direction: giving a group of arms ONE trailing `return` stops cl
cross-jumping their tails together. It moved this same function's
`pfk >= 0xdf` arm from a `push 0x1; jmp <shared>` back to three full sites.

## Why the tool could not be invoked before

`--gap` only accepted a spec JSON of front-end `cb` estimates - numbers nobody
has for a real row - so `--gap 0x08b960` answered `spec JSON missing` while
CLAUDE.md and every matcher brief named it as THE lever for this class. The
address form does the derivable half and deliberately refuses to guess `cb`: a
guessed deficit printed as a model output is indistinguishable from a measured
one. `--measure-cb` still titrates the real number for the spec form.

# Diff the REFERENT SETS to catch a hand-rolled `static __inline` standing in for a real call
tags: cpp:inline cpp:expr | asm:call asm:mov | topic:codegen-idiom
symptoms: a function 10-20 points low whose logic reads right; the base obj references a global the delinked target obj NEVER references (or vice versa); the base has no `call` where retail has one to a CRT/engine symbol; the instruction count is short by a multiple of the helper's body
confidence: 10/10

## The screen

Compare the two objs' **relocation symbol sets**, not their disassembly:

```sh
llvm-objdump -dr build/objdiff/base/<unit>.obj   | grep -c '<symbol>'
llvm-objdump -dr build/objdiff/target/<unit>.c.obj | grep -c '<symbol>'
```

A symbol with a **non-zero count on one side and zero on the other** is not
scheduling. It means the two sides call different code.

## Measured

`include/Gruntz/GameRand.h` defines a `static __inline i32 GameRand()` that
lazily seeds from `timeGetTime()` and then runs MSVC's LCG. `CGrunt::StepHitAndRunnerBehavior`
0xed9f0 used it at five sites:

| unit | base `g_randSeed*` | target `g_randSeed*` | target `_rand` |
|---|---|---|---|
| `gruntwanderstep` | **9** | **0** | **5** |
| `gruntspawnconfig` | 9 | 9 | 0 |
| `dialogs` | 6 | 6 | 0 |
| `multistartdlgroster` | 6 | 6 | 0 |

`GameRand` is a REAL device - three units match with it. It was simply the wrong
one in `StepHitAndRunnerBehavior`, where retail calls the plain CRT `rand()`. Swapping the five
sites: 50.97 -> 60.55, instruction count 698 -> 607 against retail's 661, reloc
sequence 52/52 and byte-identical in order.

## Rule

Any `static __inline` in a header is a MODELLING CLAIM about what retail inlined.
Before you trust one at a new call site, count the globals it touches on both
sides of that unit. The check costs one `grep` and it is decisive: a hand-rolled
expansion leaks its private globals into the base's relocation table, and the
delinked target either has them or it does not.

The same screen catches the inverse - a real out-of-line helper you spelled as a
`static` that cl declined to inline (MSVC5 `/O2` implies `/Ob1`, so a plain
`static` is NOT inlined). `CBattlezMapConfig::StepDefenderUnit` 0x33520 called a
`static i32 iabs()` eight times where retail has the `cdq/xor/sub` intrinsic
expansion; using `abs()` from `<stdlib.h>` removed all eight calls.

## Related

- [`abs-intrinsic-cdq-xor-sub-vs-hand-rolled-negate`](abs-intrinsic-cdq-xor-sub-vs-hand-rolled-negate.md)
- [`reloc-sequence-diff-finds-wrong-referents`](reloc-sequence-diff-finds-wrong-referents.md)
  - the ordered form of the same diff, which also finds missing/extra STATEMENTS.
- [`static-helper-must-be-inline`](inline-boundary-is-readable-off-the-callsite.md)

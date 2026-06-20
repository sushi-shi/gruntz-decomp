# A flag/style that's mostly one value: spell `x = DEFAULT; if(cond) x = OTHER;` — not 0-then-set or if/else select
tags: cpp:branch cpp:local | asm:mov asm:je | topic:codegen-idiom
symptoms: branch polarity (`je`↔`jne`) and a flag's init are wrong; or a 2-way constant select folds BRANCHLESS to `neg/sbb/and; add` instead of the target's `mov K; je; mov K2`
confidence: 8/10

When the target initializes a flag/style/found-bit to its COMMON value early and overwrites it
only on the off-path, reproduce it as **default-then-conditional-override**, not 0-then-set-1
and not a symmetric `if/else` select:

```cpp
found = 1;  ...  if (!FileExists(p)) found = 0;     // NOT: found=0; ... 5x conditional =1
style = 0xCF0000; if (flags & 2) style = 0xCA0000;  // NOT: style = (flags&2)?0xCA0000:0xCF0000;
```

Two failure modes it fixes: (a) a `found=0`-first form with conditional `=1`s cascades the
branch polarity and the flag's init (`je`↔`jne`); (b) when the two constants differ by a single
bit-run (0xCF0000 vs 0xCA0000 differ by 0x50000), an `if/else` two-way select folds to a
BRANCHLESS `neg eax; sbb eax,eax; and 0x50000; add 0xca0000` — the default-override emits the
target's `mov 0xcf0000; je; mov 0xca0000` branch. (For two locals like x/y = CW_USEDEFAULT/0,
keep them as TWO separate locals or the same branchless fold appears.)

STEERABLE. Evidence: MakeRezPath `found` 87→92%; InitializeDefaultCreateStruct style 14→99% &
x/y 14→78%; same idiom across InitializeDefaultWindowClass.

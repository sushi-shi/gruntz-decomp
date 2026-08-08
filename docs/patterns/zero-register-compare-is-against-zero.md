# `cmp <reg>,<zeroreg>` is a comparison against 0 - do not read the register as a value
tags: cpp:branch cpp:enum | asm:cmp asm:test asm:xor | topic:regalloc topic:codegen-idiom
symptoms: cmp eax,ebx | test eax,eax | xor ebx,ebx | switch arm compares against the wrong enumerator
confidence: 10/10

A function whose prologue contains a bare `xor <callee-saved>,<callee-saved>` has
pinned 0 in that register for the whole body (see
[redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md)
for what creates one). Every later `cmp <r>,<zeroreg>` is then `<r> == 0`, and
`push <zeroreg>` is `push 0`. The trap: a reader who does not notice the pin
reads `cmp eax,ebx` as "compare against some variable" and invents an enumerator
to explain it.

The tell that the pin is real is the *inconsistency between sibling arms*: arms
that spell the same test `test eax,eax` are the ones where the zero register has
been clobbered locally (a `lea <zeroreg>,[this+off]` for an inlined struct fill
is the usual culprit), so cl falls back to `test`. Two spellings of one source
comparison, not two comparisons.

```asm
  xor    ebx,ebx                   ; prologue: 0 is pinned in ebx
  ...
  mov    eax,DWORD PTR [esi+0x2d0]
  cmp    eax,ebx                   ; == 0.  NOT "== 1", NOT "== <some local>"
  jne    ...
  ...
  lea    ebx,[esi+0x2a0]           ; sibling arm clobbers the pin ...
  mov    eax,DWORD PTR [esi+0x2d0]
  test   eax,eax                   ; ... so the SAME source test reads differently
```

Reading rule, not a codegen lever. `CGrunt::LoadGruntTypeTable` 0x4dd50 had ten
switch arms transcribed as `m_arrivalState == AI_DUMBCHASER` (1) purely because
their `cmp eax,ebx` was read as a value compare while the twenty-three tool arms
next door - identical source, but with `ebx` taken by the reach-rect `lea` -
correctly read `test eax,eax` as `AI_NONE` (0). Fixing the ten took the function
87.12 -> 90.20 as part of one change.

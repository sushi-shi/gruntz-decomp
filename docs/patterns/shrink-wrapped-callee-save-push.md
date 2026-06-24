# Shrink-wrapped callee-save push — retail defers `push edi`/`push ebx` past a cheap null guard
tags: cpp:branch cpp:return | asm:push asm:pop | topic:wall topic:regalloc

symptoms: a method with an early null-guard return — retail saves only the registers the GUARD path
needs at entry (`push ebp; push esi`), runs the `xor eax,eax; pop esi; pop ebp; ret` early-out with
just those, then `push edi; push ebx` AFTER the guard passes; your recompile pushes ALL callee-saved
regs in the prologue (`push ebx; push ebp; push esi; push edi`) and the guard exit pops all four.
~85-90%, body otherwise byte-exact.

```asm
; RETAIL — partial save, edi/ebx deferred past the guard
push ebp
push esi
mov  esi,ecx
mov  ebp,[esi+0xb0]
test ebp,ebp
jne  keep
  xor eax,eax
  pop esi
  pop ebp
  ret
keep:
push edi            ; saved only on the path that uses them
push ebx
; …body uses ebx (flags), edi (a temp)…

; RECOMPILE — all four saved upfront, guard pops four
push ebx
push ebp
push esi
mov  esi,ecx
push edi
mov  ebp,[esi+0xb0]
test ebp,ebp
jne  keep
  xor eax,eax
  pop edi
  pop esi
  pop ebp
  pop ebx
  ret
```
WALL. MSVC 5.0 sometimes shrink-wraps the callee-save pushes around a cheap early-return so the guard
path saves/restores fewer registers; whether the recompile does it is a whole-function prologue/
regalloc decision with no source-level lever (the int-return `return 0` guard, the field reads, and
the call all match — only the push placement differs). Declaring the real `int` return first
(void-vs-bool-return-epilogue-split.md) recovers the inline epilogues + `xor eax,eax` and is worth
doing; the residual push placement is the wall. Evidence: CPlaneRender::CenterScrollA/B (@0x163300/
0x163370) 83.5% → 87.9% on the int-return fix, then parked — only the deferred edi/ebx push + an
adjacent member-load order remain.
related: void-vs-bool-return-epilogue-split.md, identical-return-epilogue-tailmerge.md

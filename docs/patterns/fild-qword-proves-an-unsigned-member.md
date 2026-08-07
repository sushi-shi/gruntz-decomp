# `fild QWORD` with a zeroed high dword proves the member is UNSIGNED (and kills our `fimul`)
tags: cpp:member cpp:cast | asm:fild asm:fmul | topic:codegen-idiom
symptoms: `insn_seq --hist` shows the matched pair `fimull 1 -> 0` against `fildll 0 -> 1` (plus a stray `fmulp`/`popl` delta); retail spills the value to a stack pair, zeroes the high dword and does `fild QWORD PTR [esp+N]` where we emit a single `fimul DWORD PTR [reg+off]`
confidence: 9/10

x87 has no unsigned-32 load. So `(double)someUnsigned` forces MSVC to widen to a
64-bit temporary first:

```asm
mov  edx,DWORD PTR [ebx+0x74]      ; the u32 member
xor  edi,edi
mov  DWORD PTR [esp+0x10],edx      ; lo
mov  DWORD PTR [esp+0x14],edi      ; hi = 0
fild QWORD PTR [esp+0x10]          ; (double)(u32)
```

For a **signed** `i32` in the same expression cl instead folds the conversion into the
arithmetic and emits `fimul DWORD PTR [ebx+0x74]` - one instruction, no spill. That
`fimul`-vs-`fild QWORD` pair is therefore a **member-type oracle**, and it is one of the
few places where a signedness slip is directly visible instead of hiding behind a
`jl`-vs-`jb`.

**Evidence.** `CBattlezMapConfig::StepRowSpawn` @0x26470. Retail:
`fild DWORD [combo] / fild QWORD [esp+0x10] / fmul DWORD ds:g_diffScale / fmulp`. Ours:
`fild DWORD [combo] / fimul DWORD [ratio] / fmul DWORD [scale]`. Retyping
`CBattlezMapConfig::m_gruntRatio` from `i32` to `u32` took it **81.79 -> 84.41**. The
retype is independently corroborated at the store site - the field is filled from
`g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25)`, which returns `DWORD`.

## Do NOT then "fix" the operand order with parentheses

Retail's residual difference is the last two ops (`fmul` then `fmulp` vs our `fmulp`
then `fmul`), i.e. retail groups `a * (b * c)`. Spelling that grouping explicitly is a
**regression**: cl re-evaluates the parenthesised product first and then folds the
remaining `a` back into an integer `fimul`, so you lose the `fild` you just won
(84.41 -> 82.27, measured). Leave the natural left-associative expression.

Related: [`mnemonic-histogram-diff-finds-the-wrong-idiom`](mnemonic-histogram-diff-finds-the-wrong-idiom.md)
- the `fildll`/`fimull` pair is exactly the matched over/under row that method looks for.

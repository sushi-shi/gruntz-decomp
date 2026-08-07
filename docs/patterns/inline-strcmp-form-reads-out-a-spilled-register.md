# The inline-strcmp FORM tells you a callee-saved register got spilled

- **confidence** c9
- **tags** `topic:triage` `topic:wall` | `asm:cmp` `asm:mov` `asm:sub-esp` | `cpp:locals`
- **measured** `CInGameIcon::CInGameIcon` @0x95b10 - 52 of 57 inline `strcmp`s take the
  3-register form, which is 102 of the +128 instruction surplus by itself.

## The two forms

MSVC 5.0's `/Oi` inline `strcmp` compares two bytes per iteration and comes out two ways.

**2-register (retail here)** - one byte register, the other operand stays in memory:

    mov bl,BYTE PTR [eax]        mov bl,BYTE PTR [eax+1]
    mov cl,bl                    mov cl,bl
    cmp bl,BYTE PTR [esi]        cmp bl,BYTE PTR [esi+1]

**3-register** - both operands loaded first:

    movb (%eax),%dl              movb 0x1(%eax),%dl
    movb (%esi),%bl              movb 0x1(%esi),%bl
    movb %dl,%cl                 movb %dl,%cl
    cmpb %bl,%dl                 cmpb %bl,%dl

Six `movb` per comparison instead of four. In a `strcmp` if/else ladder - the shape of
every `CInGameIcon`/`CWwdGameObject` factory ctor in this codebase - that difference is
the whole diff.

## Diagnosis

Count the `movb` operand SHAPES rather than reading the body:

    llvm-objdump -d build/objdiff/base/<unit>.obj      # then group `movb` operands

`(%eax),%dl` and `(%esi),%bl` appearing in equal numbers = the 3-register form.

## Root cause, and it is NOT the strcmp

The form is a readout of register pressure, so reconcile the PROLOGUE
([frame-size-mismatch-dominates-the-40-65-band](frame-size-mismatch-dominates-the-40-65-band.md)):

    base    sub esp,0x1c   push ebx / esi / edi          mov ebx,ecx     (this)
    retail  sub esp,0x18   push ebx / ebp / esi / edi    mov ebp,ecx     (this)

Retail took ONE MORE callee-saved register and FOUR FEWER stack bytes - the classic
one-local-spilled signature. What it spent the extra register on is visible three
instructions in: `xor ebx,ebx` and then `push ebx` / `mov [esp+0x30],ebx` where cl emits
`push $0` / `mov $0,[esp+0x30]`. Retail PROMOTED THE CONSTANT ZERO into ebx, which pushed
`this` up into ebp and left one whole byte register free for every `strcmp` in the body.

So the lever is the local set / constant usage in the function head, not anything near the
comparisons. Do not hand-tune the ladder.

related:
[frame-size-mismatch-dominates-the-40-65-band.md](frame-size-mismatch-dominates-the-40-65-band.md),
[known-zero-reload-before-call.md](known-zero-reload-before-call.md)

# `cmp reg,reg` where ours has `cmp [mem],reg`: the comparison's LEFT operand was a NAMED TEMPORARY, evaluated before the call it is compared against

tags: cpp:expr cpp:local cpp:call cpp:if | asm:cmp asm:mov | topic:codegen-idiom
symptoms: `x->field >= f(...)` compiles to `call f / cmp [reg+disp],eax` in ours and `mov edi,[reg+disp] / call f / cmp edi,eax` in retail; the function is 1-2 instructions SHORT per comparison and sits 92-95%; the masked diff reads as regalloc because the compare is the only visibly different line
confidence: 9/10

The order in which cl5 evaluates the two operands of a relational operator is
**not fixed by the source** — but a named temporary *is* a statement, and a
statement is sequenced. Written as one expression, cl5 evaluates the CALL first
and then reads the other operand straight out of memory at the compare. Retail's
compilands consistently have the LEFT operand already in a callee-saved register
across the call, which only happens when the source loaded it into a variable
first.

```cpp
// ours - 2 instructions short, the compare reads memory AFTER the call
if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
    || m_object->m_frameImage->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {

// retail - each LHS is materialised before its call
i32 bigW = aux->m_width;
i32 bigH;
if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
    || (bigH = m_object->m_frameImage->m_height)
        >= g_buteMgr.GetInt("World", "BigActHeight")) {
```
```asm
mov  edi,[eax+0x10]        ; LHS materialised
push 0 / push "BigActHeight" / mov ecx,g_buteMgr
call ?GetInt@CButeMgr@@...
cmp  edi,eax               ; reg,reg  <- the fingerprint
jge  <body>
mov  edx,[esi+0x10]        ; second arm: the whole LHS chain, again before the call
push 0 / push "BigActHeight"
mov  eax,[edx+0x198]
mov  edi,[eax+0x14]
call ?GetInt@CButeMgr@@...
cmp  edi,eax
```

**The second operand of a `||` needs the assignment-in-condition form** — there is
no statement position between the short-circuit branch and the second call, so
`(bigH = expr) >= f(...)` is the only spelling that sequences the load ahead of
the call. (`i32 bigH = expr;` hoisted above the `if` evaluates it
unconditionally and lands the load before the FIRST call, which is a different,
wrong shape.)

**Diagnose it with instruction COUNTS, not with the masked diff.** Per comparison
the memory form is exactly one instruction short, so
`llvm-objdump -d` base-vs-target counts disagree by the number of affected
comparisons — a signal the address-masked `--diff` view never shows, because the
only differing LINE is the `cmp` and it looks like an operand-mode choice.

STEERABLE, and it is a family sweep: the identical big-act guard appears verbatim
in seven candy/animation constructors. CFrontCandy 95.15 -> **99.83**,
CBehindCandy 92.47 -> **99.83**, CDoNothing 94.68 -> **99.53**, CSimpleAnimation
95.15 -> **98.72**, CEyeCandy 94.51 -> 97.85, CEyeCandyAni 93.58 -> 96.60,
CBehindCandyAni 55.19 -> 61.81.

related: [redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md)
(the opposite direction — a local that should NOT be there),
[sortkey-flag-rmw-needs-local-receiver.md](sortkey-flag-rmw-needs-local-receiver.md)
(a local that fixes an addressing mode rather than an evaluation order).

# Early-return zero-guard: spell `if(x <= 0)` not `if(x == 0)` to get unsigned `jbe`/`ja`
tags: cpp:branch cpp:return cpp:type | asm:ja asm:jbe asm:je asm:jne | topic:codegen-idiom
symptoms: leading `if(count==0) return 0;` guard on a u32 param; body byte-exact but retail `test;ja`/`test;jbe` (or `cmp;ja/jbe`) where recompile emits `test;jne`/`test;je`; ~98.7-99.1% plateau, one compare-jump opcode flipped
confidence: 9/10
variants: switch-key-unsigned-ja-vs-jg.md

A leading early-return zero-guard on an **unsigned** operand, written as
`if (x == 0) return …;`, lowers to an **equality** test (`test reg,reg; je/jne`).
Retail often emits the **unsigned-relational** form instead (`ja`/`jbe`) because
its source spelled the guard relationally. For an unsigned `x`, `x <= 0` is
semantically identical to `x == 0`, but MSVC5 keeps the unsigned comparison and
emits `jbe` (bail-jump) / `ja` (body-jump) — matching retail. The scalar sibling
of switch-key-unsigned-ja-vs-jg. Crucially this respelling keeps the **early-return
structure** (body stays at top level), so it does NOT cascade the way wrapping the
body in `if (x > 0) { …body… }` does (that nests the body and reschedules
everything).

```cpp
i32 Read(i32 off, i32 base, u32 count, void* buf) {
    if (count <= 0) {       // u32 `count`: `<= 0` == `== 0` but emits jbe/ja, not je/jne
        return 0;
    }
    /* body stays at top level — no nesting, no cascade */
}
```
```asm
test edi,edi
ja   <body>        ; unsigned (u32); `== 0` source gives `jne` here
```
Steerable: change `== 0` → `<= 0` on the unsigned guard (matching-neutral; pure
opcode flip, body untouched). Closed CRezItm::Read (0x13c600) 99.07%→99.77% and
CRezItm::Write (0x13c6c0) 98.77%→99.69% (both then at the reloc-masked plateau —
only the delinker's bare CRT-wrapper names remain). Note the prior `if(count>0){body}`
wrap was rejected for cascading (99.3%→83.4%); the `<= 0` early-return form does not.

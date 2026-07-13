# signed-char `& 1` — MSVC5 narrows the load to `movb`; retail keeps `movsx`
tags: cpp:int | asm:mov | topic:wall
symptoms: movsx eax byte and eax 1, movb al and eax 1, signed char mask, flag byte & 1
confidence: 7/10

A method that loads a one-byte (signed `char`) flag field and returns `flag & 1`.
Retail loads it sign-extended (`movsx eax, byte ptr [reg+N]`) before the mask;
MSVC5 /O2 unconditionally NARROWS `signed-char & 1` (and `& 1` generally, since
only bit 0 is live) to a plain byte load `movb al, [reg+N]; and eax, 1`. Every
source spelling tested narrows to `movb`: plain `f->b & 1`, `(int)f->b & 1`,
`(signed char)f->b & 1`, an `int v = f->b; return v & 1;` temp, an inline
`char Get()const{return b;}` accessor, and `(f->b + 0) & 1`.

```cpp
return ((Host*)m_4)->m_flag & 1; // m_flag is `signed char`
```
```asm
movsx eax, byte ptr [eax+0x20]   ; RETAIL — sign-extend the byte first
and   eax, 1
; MSVC5 base instead:  movb al,[eax+0x20] ; and eax,1   (one byte short)
```
WALL — no natural MSVC5 source produces the standalone `movsx ...; and eax,1`
when the only use is the `& 1` mask (the peephole always narrows to `movb`); a 1-
instruction residual. CMenuPage::CanWrap @0x183e30 plateaus ~95.4%, logic exact.

2026-07-13 addendum (Fable lane): the `(char)`-of-an-`i32`-field arm is ALSO
narrowed — `return (char)m_host->m_wrapFlag & 1;` with `i32 m_wrapFlag` (the
field's true width: the writer 0x182ab0 stores the whole DWORD) still emits
`mov al,[eax+0x20]; and eax,1`. The wall is spelling-independent of the FIELD
width, not just of the char-typed-field spellings originally tested.

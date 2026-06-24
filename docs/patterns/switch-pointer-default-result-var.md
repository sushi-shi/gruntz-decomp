# Pointer-returning `switch` whose default is null — pre-zero eax via an explicit result var
tags: cpp:switch cpp:return | asm:xor asm:mov | topic:codegen-idiom
symptoms: switch dispatch loads the index into eax (not edx), no leading xor eax,eax; ~50% on a tail-call-per-case switch returning a pointer

A `switch(i)` where each case `return f(i)` and the default returns a null pointer. Written as
`switch{ case: return f(); } return 0;`, MSVC5 keeps the index in **eax** and omits the default
pre-zero, diverging from retail (`mov edx,[esp+4]; xor eax,eax; cmp edx,3; ja; jmp [edx*4+tbl]`).
Spell it with an explicit `T* result = 0; … result = f(); break; … return result;` — the leading
`result = 0` forces `xor eax,eax` up front, which pushes the index into **edx** so it survives the
pre-zeroed return reg. The per-case `result = f(); break;` still tail-calls (`push; call; ret`).

```cpp
CWnd* result = 0;
switch (index) {
    case 0: result = GetDlgItem(0x500); break;
    case 1: result = GetDlgItem(0x50e); break;
    /* … */
}
return result;
```
```asm
mov edx,[esp+4]
xor eax,eax            ; the pre-zeroed null default (forced by `result = 0`)
cmp edx,3
ja  default
jmp [edx*4+tbl]        ; index in edx, not eax
```
STEERABLE (closes the CODE; the inline jump-table data still reloc-masks, see
[[jumptable-data-overlap]]). Evidence: CBattlezDlg::GetCtrlA/B/C/D 51→70% (code byte-exact, the
residual is the jump-table-data artifact, not the dispatch).

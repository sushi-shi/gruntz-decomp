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
[[jumptable-data-overlap]]). Evidence: CBattlezDlg::GetPlayerTypeControl/B/C/D 51→70% (code byte-exact, the
residual is the jump-table-data artifact, not the dispatch).

## Measured (i32 member-call form, 2026-08-17)

CAreaMgr::InitializeLevel 0x99d40 (40-arm QuestLevel switch, every arm a
member call) 97.59 -> 100.00 EXACT with `i32 result = 0;` before the switch,
`result = InitX(); break;` per arm and one trailing `return result;`:

- cl replicates the trailing return into EVERY arm (41 rets - each arm ends
  `call; pop; pop; ret`, the call result already in eax);
- the initializer's zero materializes ABOVE the dispatch
  (`lea ecx,[idx-1]; xor eax,eax; cmp ecx,0x27; ja`), which is what frees the
  index into ecx;
- the `ja` default jumps PAST the range-guard exits' own `xor eax,eax`
  straight onto `pop/pop/ret` - the guard `return 0`s and the switch-default
  zero are separate statements. A `default: return 0;` or a plain `return 0;`
  after the switch merges them onto one xor and keeps the index in eax.


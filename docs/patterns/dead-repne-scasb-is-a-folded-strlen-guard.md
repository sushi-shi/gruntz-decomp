# A dead `repne scasb` with no branch is a FOLDED `strlen(...) == 0` guard
tags: cpp:if cpp:string cpp:eh | asm:scasb asm:not asm:dec | topic:codegen-idiom
symptoms: retail inlines `strlen` — `or edx,-1` / `mov ecx,edx` / `xor eax,eax` / `repne scasb` / `not ecx` / `dec ecx` — and then never reads ECX; the recompile omits the whole sequence and carries one fewer callee-saved push
confidence: 9/10

cl 5.0 deletes a bare `strlen(s);` outright, so a dead inline strlen in retail is not a
dead statement in the source. It is a **guard whose branch was folded**: the value is
live when DCE runs, and only the later block merge removes the compare — because both
arms do exactly the same thing (destroy the locals and return).

```cpp
CString s;
GetPlayerNameControl(index)->GetWindowText(s);
if (strlen(s) == 0) {      // `strlen(s);` alone is DELETED and matches nothing
    return;                // ... and the rest of the body is empty
}
```
```asm
call    ?GetWindowTextA@CWnd@@QBEXAAVCString@@@Z
mov     edi,DWORD PTR [esp+0x8]   ; s.m_pchData
or      edx,0xffffffff
mov     ecx,edx
xor     eax,eax
repne   scas al,BYTE PTR es:[edi] ; the guard's strlen
not     ecx
dec     ecx                       ; ...and ECX is never read again
mov     DWORD PTR [esp+0x14],edx  ; edx doubles as the -1 unwind state
lea     ecx,[esp+0x8]
call    ??1CString@@QAE@XZ
```

Steerable and exact: `CBattlezDlg::ReadPlayerName` 0x17340 went **69.14 -> 100.00
EXACT**. The tells that it is a guard and not a copy are that no `rep movs` follows (so
it is not an inlined `strcpy`) and that retail carries an extra `push edi` the recompile
does not — the string cursor needs a callee-saved register. Note `edx` is shared between
the strlen seed and the `-1` unwind-state store, which is why the sequence also shifts
the EH state slot by four; `gruntz walls eh-frame --states` is what surfaced this row.

# `pWnd->InvalidateRect(0,1)` (MFC inline member), not `::InvalidateRect(pWnd->m_hWnd,…)`
tags: cpp:mfc cpp:inline cpp:method | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: one instruction pair differs on a Win32 import call, `mov eax,[eax+0x1c]; push eax` where retail has `mov ecx,[eax+0x1c]; push ecx` (or edx), a hoisted `HWND h = GetDlgItem(id)->m_hWnd;` local, ~99.84%
confidence: 9/10
variants: win32-import-decl-stdcall.md

MFC's thin wrappers (`CWnd::InvalidateRect`, `SetWindowText`, …) are inlines in
`afxwin*.inl` that expand to the global import on `m_hWnd`. Calling the global
directly on a hoisted handle frees the `CWnd*` in eax, so cl loads the handle into
eax too. Calling the **member** keeps the `CWnd*` live in eax as the inline's
`this`, so the handle load goes to the next register — which is what retail shows.

```cpp
// NO - hoisting the handle frees eax; the m_hWnd load lands in eax
HWND h = GetDlgItem(0x501)->m_hWnd;
::InvalidateRect(h, 0, 1);

// YES - the MFC inline member: the CWnd* stays the inline's `this` in eax
GetDlgItem(0x501)->InvalidateRect(0, 1);
```
```asm
push 0x501
mov  ecx,esi
call ?GetDlgItem@CWnd@@QBEPAV1@H@Z
mov  ecx,[eax+0x1c]        ; eax is still the CWnd* -> handle goes to ecx
push 0x1
push 0x0
push ecx
call [__imp__InvalidateRect@12]
```
STEERABLE. `CMultiStartDlg::OnPlayerColor0/1/2/3` 99.84 -> **100.00 EXACT** all four
(previously `@early-stop`'d as an "eax/ecx regalloc coin-flip"), and the identical
fix earlier took `CBattlezDlg::OnPlayerColor0..3` from 91.1 to exact — the same
four-sibling shape appears wherever a dialog refreshes numbered swatch controls.

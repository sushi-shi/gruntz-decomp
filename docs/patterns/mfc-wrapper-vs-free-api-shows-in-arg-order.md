# The receiver load's POSITION says whether retail called the MFC wrapper or the free Win32 API
tags: cpp:call cpp:method cpp:inline | asm:push asm:mov asm:call | topic:codegen-idiom
symptoms: every `SendMessage`/`GetWindow` site is 0 instructions off but the operand ORDER differs - retail evaluates the control-fetch call FIRST and loads `m_hWnd` before the `push`es, the recompile pushes the message/wParam/lParam literals first and fetches the control last; the import is `call *reg` in retail and `call *__imp__X` here
confidence: 9/10

`::SendMessageA(GetCtrlA(i)->m_hWnd, CB_ADDSTRING, 0, (LPARAM)"None")` and
`GetCtrlA(i)->SendMessageA(CB_ADDSTRING, 0, (LPARAM)"None")` are the same four
arguments in the same order and produce the same relocations, so `insn_seq` and
the masked diff both call them equal. They are NOT the same code.

* **Free function.** `m_hWnd` is just argument 1, so MSVC5 pushes right-to-left -
  lParam, wParam, message - and only then evaluates `GetCtrlA(i)` and loads
  `m_hWnd`. The control fetch lands *after* the literal pushes.
* **MFC member wrapper.** `CWnd::SendMessage` (AFXWIN2.INL:
  `{ return ::SendMessage(m_hWnd, message, wParam, lParam); }`) makes the control
  fetch the OBJECT expression, which is sequenced before the argument list. So the
  call comes first, `mov reg,[eax+0x1c]` follows immediately, and the literals are
  pushed after it.

```asm
; retail - the member wrapper
push edi                       ; i
call ?GetCtrlA@CBattlezDlg@@QAEPAVCWnd@@H@Z
mov  eax,[eax+0x1c]            ; m_hWnd, BEFORE the pushes
push offset ??_C@_04COF@None   ; lParam
push 0                         ; wParam
push 0x143                     ; CB_ADDSTRING
push eax
call ebx                       ; __imp__SendMessageA cached in a register

; ours - the free function
push offset ??_C@_04COF@None
push 0
push 0x143
push edi                       ; i
call ?GetCtrlA@CBattlezDlg@@QAEPAVCWnd@@H@Z
mov  eax,[eax+0x1c]            ; m_hWnd, AFTER
push eax
call ebp
```

The same test names `CWnd::GetWindow` (`{ return FromHandle(::GetWindow(m_hWnd,
nCmd)); }`): retail's `push 0x4ff; call GetDlgItem; mov edx,[eax+0x1c]; push 5;
push edx; call GetWindow` is the wrapper, while
`CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD))` pushes
`GW_CHILD` before `GetDlgItem` even runs. `SetWindowText`, `GetWindowText`,
`EnableWindow` and `SendMessage` all have the wrapper; prefer it wherever the
receiver is itself a call.

Corollary: once several sites use the wrapper, cl caches `__imp__SendMessageA` in
a callee-saved register and every site becomes `call ebx` - so a lone
`call *__imp__SendMessageA@16` in a function that otherwise uses a register is a
site you have not converted yet.

STEERABLE. CBattlezDlg::DoDataExchange 0x14d00 68.31 -> 91.19 on the
SendMessage conversion alone, and 91.19 -> 93.63 on the four GetWindow walks.
Companion (opposite direction, when the receiver is a plain local):
`chained-call-arg-eval-order.md`.

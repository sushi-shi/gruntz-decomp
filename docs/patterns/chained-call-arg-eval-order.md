# Chained `f(a)->g(b)` hoists the outer arg early — split to a temp to defer it
tags: cpp:method cpp:call | asm:push asm:mov | topic:codegen-idiom
symptoms: outer-call arg pushed BEFORE the inner call (kept live across it); retail reloads it from the stack AFTER; ~50% on a two-call one-liner

`f(a)->g(b)` — call `f(a)`, then call `g(b)` on the result. Written as one expression, MSVC5
evaluates `b` early and pushes it BEFORE the `f(a)` call (carrying it across), whereas retail
loads `b` fresh from its stack slot AFTER `f(a)` returns. Introduce a named temp so the inner call
is its own statement; the sequence point makes MSVC schedule `f(a)` first (only `a` pushed), then
load+push `b` for `g`.

```cpp
CWnd* w = GetCtrlB(index);   // inner call alone: push index; call
w->SetWindowTextA(text);     // text loaded + pushed AFTER, then mov ecx,result; call
```
```asm
mov eax,[esp+4]        ; index
push eax
call GetCtrlB
mov ecx,[esp+8]        ; text reloaded AFTER the call (not hoisted)
push ecx
mov ecx,eax            ; this = result
call SetWindowTextA
```
STEERABLE. Evidence: CBattlezDlg::SetCtrlBText 49→100% after splitting the chained
`GetCtrlB(index)->SetWindowTextA(text)` into a temp.

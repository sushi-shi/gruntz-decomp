# Chained `f(a)->g(b)` hoists the outer arg early — split to a temp to defer it
tags: cpp:method cpp:call | asm:push asm:mov | topic:codegen-idiom
symptoms: outer-call arg pushed BEFORE the inner call (kept live across it); retail reloads it from the stack AFTER; ~50% on a two-call one-liner

`f(a)->g(b)` — call `f(a)`, then call `g(b)` on the result. Written as one expression, MSVC5
evaluates `b` early and pushes it BEFORE the `f(a)` call (carrying it across), whereas retail
loads `b` fresh from its stack slot AFTER `f(a)` returns. Introduce a named temp so the inner call
is its own statement; the sequence point makes MSVC schedule `f(a)` first (only `a` pushed), then
load+push `b` for `g`.

```cpp
CWnd* w = GetPlayerNameControl(index);   // inner call alone: push index; call
w->SetWindowTextA(text);     // text loaded + pushed AFTER, then mov ecx,result; call
```
```asm
mov eax,[esp+4]        ; index
push eax
call GetPlayerNameControl
mov ecx,[esp+8]        ; text reloaded AFTER the call (not hoisted)
push ecx
mov ecx,eax            ; this = result
call SetWindowTextA
```
STEERABLE. Evidence: CBattlezDlg::SetPlayerName 49→100% after splitting the chained
`GetPlayerNameControl(index)->SetWindowTextA(text)` into a temp.

## A returned stream reference can require more than one split

The same rule applies to a chain of overloaded calls returning `ostream&`. In
`ButeMgr::ParseAttributeFile` 0x170750, this natural chain left the order of the
first stream call and `GetBuffer` to the front end:

```cpp
(*m_pText) << static_cast<unsigned char>('"') << tmp.GetBuffer(0)
           << static_cast<unsigned char>('"');
```

Our baseline called `GetBuffer` first. Retail calls the first `operator<<`, saves
its returned reference in ESI, calls `GetBuffer`, then performs the remaining two
insertions. Naming only the first result reproduced that order but still left the
last hand-off as a scheduling choice. Naming both returned references made the
entire function byte-exact:

```cpp
ostream& output = (*m_pText) << static_cast<unsigned char>('"');
ostream& stringOutput = output << tmp.GetBuffer(0);
stringOutput << static_cast<unsigned char>('"');
```

The controlled source matrix kept the 0xa04-byte extent, 750 instructions, 87
calls, 83 branches, two returns and 124 relocations fixed. Its raw campaign scores
were 98.470350% for the chain, 99.634770% after the first split, and 99.907005%
after both splits; the ordinary rebuilt and normalized pair for the last form is
100.00% exact with an identical relocation stream. A 256-trial TU-state sweep was
flat at one compiler island, and naming the stream before any insertion was a
negative control (98.354450%).

**Reading rule:** when retail executes a nested call's receiver-producing call
before evaluating the next argument, name the returned object/reference at that
boundary. If the following returned reference is also carried into another call,
split that boundary independently rather than assuming one temp sequences the
whole chain.

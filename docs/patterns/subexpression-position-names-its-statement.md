# Where cl COMPUTES a value names the statement it was written in
tags: cpp:call cpp:local cpp:string | asm:push asm:add asm:call | topic:codegen-idiom
symptoms: identical instruction multiset, diagnose says REGALLOC/SCHEDULING, one `push <imm>` or one `add reg,K` sits on the wrong side of a `call`
confidence: 10/10
variants: class-return-temp-read-through-eax-not-its-slot.md

cl 5.0 emits a call's arguments right-to-left AT the call, so a value retail
computes EARLIER than the call that consumes it was written in an EARLIER
STATEMENT. Two readable forms, both worth one edit when the rest of the
function is byte-identical.

**A constant argument pushed BEFORE an inner call** means the inner call is
written inline as the outer argument; retail pushing it AFTER the inner call
means retail bound the inner result to its own local.

```cpp
// ours: push $5 / push $0x4ff / call GetDlgItem / push eax / call GetWindow
CWnd* child = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD));
// retail: push $0x4ff / call GetDlgItem / mov eax,[eax+0x1c] / push $5 / push eax
HWND owner = GetDlgItem(0x4ff)->m_hWnd;
CWnd* child = CWnd::FromHandle(::GetWindow(owner, GW_CHILD));
```

**An `add reg,K` sitting next to the value's PRODUCER instead of at the
consuming call** means the `+K` is in the variable's initializer, not at the
call site.

```cpp
// ours: repne scasb / not / dec ... rep movs ... add edx,0xd / push edx / call
i32 n = strlen(line);
Network()->BroadcastFrom(LocalPlayer(), 1, &g_chatPacket, n + 0xd);
// retail: repne scasb / not / dec / mov edx,ecx / add edx,0xd ... push edx / call
i32 packetLen = strlen(line) + 0xd;
Network()->BroadcastFrom(LocalPlayer(), 1, &g_chatPacket, packetLen);
```

```asm
; the tell, both forms: an operand of the OUTER call materialised on the far
; side of the INNER call from where the recompile puts it, with every other
; byte of the function identical
```

STEERABLE. `CBattlezDlg::CopyComboSelToChild` 0x171b0 96.76 -> 100.00 EXACT,
`CMulti::BroadcastChatLine` 0xbb190 95.68 -> 100.00 EXACT, one edit each.
NEGATIVE CONTROL - do NOT apply it to a CLASS temporary: naming the result of a
by-value class return (or binding it to a `const&`) moves its stack slot ahead
of the sibling local and turns retail's `mov <r>,[eax]` into a slot reload
(class-return-temp-read-through-eax-not-its-slot.md).
`CAniRecordView::ResolveIndices` 0x168d00 96.86 -> 97.07 with the named local,
but the GetAt destination slot and the eax read both go wrong, so the anonymous
temporary is retail's shape and the named form is rejected on evidence.

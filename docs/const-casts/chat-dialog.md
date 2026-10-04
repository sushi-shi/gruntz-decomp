# Lobby chat: CString mutation before destruction

## Site and verdict

[CMultiStartDlg::OnChatSend](../../src/Gruntz/MultiStartDlg.cpp#L827) builds two mutable CString locals, then sends one through a writable pointer:

```cpp
CString message, inputText;
GetPlayerNameControl(GetLocalPlayerSlotIndex())->GetWindowTextA(message);
message += " says: ";
input->GetWindowTextA(inputText);
if (!inputText.IsEmpty()) {
    message += inputText;
    AppendChatLine(static_cast<const char*>(message));
    input->SetWindowTextA("");
    g_multiState
        ->BroadcastChatLine(const_cast<char*>(static_cast<const char*>(message)), 0, 0, NULL);
}
```

The remaining cast permits real in-place modification of CString storage without updating its metadata. This is a CString contract violation, **not a demonstrated write to a const-declared object**. Appending normally leaves `message` uniquely owned, and this caller destroys it immediately after broadcasting. There is no later length-dependent use of this local in the observed path. That limits the practical effect here; it does not convert a read-only CString borrow into the supported mutable interface.

## SDK types and retail writes

The actual VC5 SDK's `CString::operator LPCTSTR() const` returns its internal pointer (`msvc/include/AFX.INL`, line 142). `LPCTSTR` is `const char*` in this narrow build. The conversion does not provide copy-on-write protection or a length update. The same SDK provides `LPTSTR GetBuffer(int)` and `ReleaseBuffer(int)` for writable access; their implementations detach shared storage when necessary and reconcile length afterward. See the full [SDK source and owner analysis](chat-input.md#owner-and-sdk-contract), including the pinned toolchain and source-packaging links.

[BroadcastChatLine](../../src/Gruntz/Multi.cpp#L2348) takes `char*` because it really writes the argument: it truncates beyond 128 bytes and removes up to two trailing characters below a space using signed comparisons. Retail RVA `0xbb190` contains terminating stores at `+0x4f`, `+0x66`, and `+0x75`. A readonly signature for that function would be incorrect.

Retail `OnChatSend` at `0xc3f70` loads `message` data for the append operation at `+0x9d`, reloads the same local's data at `+0xb5`, then calls the broadcaster at `+0xc6`. CString destructors follow. Neither writable-buffer acquisition nor release nor a cached-length update occurs. This unsafe access pattern is inherited from the executable; it was not introduced by changing the readonly append function's signature.

Unlike the other cast, this site does not use `GetInputText`. [That getter](../../include/Gruntz/FontConfig.h#L36) returns a shared member copy by value and has a distinct lifetime/ownership analysis in [chat-input](chat-input.md).

## Provenance and reason

| Change | What it establishes |
| --- | --- |
| [6f42aeb0e6](https://github.com/sushi-shi/gruntz-decomp/commit/6f42aeb0e6fe96cad9732d1666c851921213a3f0) | Introduced `OnChatSend` with C-style removal of CString's pointer constness. The stated purpose was reconstruction of the two-CString send path; it reported an exact body, not an API-safety argument. |
| [a96482df14](https://github.com/sushi-shi/gruntz-decomp/commit/a96482df14f846b4e2fe39cee029dc26cc8eb960) | Converted the existing C-style casts to named casts in an AST-based string-cast sweep. This made the operation explicit; it did not originate the mutable access or justify its safety. |
| [7fb7b874af](https://github.com/sushi-shi/gruntz-decomp/commit/7fb7b874af263eea68af34563ed025cf5caf800a) | Later moved the roster implementation into `MultiStartDlg.cpp`. Blaming this move alone would give the wrong origin. |
| [57f133453e](https://github.com/sushi-shi/gruntz-decomp/commit/57f133453e9ac9d2b7128e21ebae2258bbbecb07) | Consolidated the mutable broadcaster declaration under its actual `CMulti` owner. Its parameter spelling is reconstructed; retail stores are the independent evidence for that spelling. |

The append and broadcast calls have different contracts. `AppendChatLine` only reads its argument while copying into another buffer, so its old mutable declaration and caller cast were unnecessary. `BroadcastChatLine` modifies its argument, so that same correction cannot be applied mechanically. The `const` conversion itself is the actual MFC SDK interface, not a reconstruction mistake.

## Correct mutable-access analogue and alternatives

Microsoft's [WordPad registry-formatting code, pinned at 9e1d447555](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpad.cpp#L811-L819), uses `GetBuffer` as the output argument to `FormatMessage`, then passes the returned length to `ReleaseBuffer`. The [file-dialog use in the same source](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpad.cpp#L581-L584) also brackets an external writer with the two methods. These are real Microsoft application examples of the mutation protocol, not evidence that Gruntz originally used it.

A repair for this local could call the broadcaster with `message.GetBuffer(0)` and then call `message.ReleaseBuffer()`. A separate mutable copy would also avoid casting the read-only borrow. Both change the operations observed in retail. Merely declaring the broadcaster's argument const would conceal real writes. The current decompilation preserves the confirmed access and documents the seam; no claim of general safety is implied.

## Separate buffer overflow

[CMulti::AppendEditLine](../../src/Gruntz/Multi.cpp#L2410) and [CMultiStartDlg::AppendChatLine](../../src/Gruntz/MultiStartDlg.cpp#L607) each concatenate into a **128-byte stack array**. When the edit already has text, they prepend two CRLF bytes, then copy the whole supplied string and its NUL terminator. An input length of 126 therefore needs 129 bytes and overflows by one. An input length of 128 overflows even without CRLF. Retail functions at `0xbb3e0` and `0xc2ce0` both reserve `0x80` stack bytes and perform the unbounded copies; this is not an array-size guess from reconstruction.

Ordinary lobby input is more constrained: [dialog initialization](../../src/Gruntz/MultiStartDlg.cpp#L255) limits names to nine characters and chat text to 100. The constructed line is at most `9 + 7 + 100 = 116` bytes, so that path fits with CRLF and NUL. This does **not** protect received messages. [DispatchRecvMsg's chat arm](../../src/Gruntz/Multi.cpp#L1613) passes received text directly to `AppendEditLine` when the chat-edit handle is present, without a string-length or termination check. Retail `0xb9750` adds the packet's text offset at `+0x189` and calls the appender at `+0x194`, with no intervening clamp. A sufficiently long, NUL-terminated received chat payload reaches a real out-of-bounds write; a missing terminator can additionally cause an out-of-bounds read. These are concrete conditional memory-safety defects, independent of the CString cast. No game execution or exploit trial is needed for this bounds calculation, and neither was performed.

The broadcaster's separate 300-byte `line` array must include any player-name prefix. With input capped at 128, a prefixed line needs `nameLength + 131` bytes including its terminator; normal nine-character names fit, but the function itself imposes no general name-length bound. All current local broadcaster callers pass a null edit handle. Finally, [CNetChatPacket's modeled text capacity](../../include/Net/NetMgr.h#L364) is explicitly unproven: its current 256-byte declaration alone does not establish a retail outgoing-packet overflow. The confirmed 128-byte receive/edit overflow does not depend on that uncertainty.

# Chat input: a writable pointer borrowed through CString's const conversion

## Current repair

The send path now keeps a named CString copy alive, acquires its writable
buffer, broadcasts, and releases the buffer:

```cpp
CString input = m_gameText->GetInputText();
multi->BroadcastChatLine(input.GetBuffer(0), 1, 1, NULL);
input.ReleaseBuffer();
```

`GetBuffer` detaches shared storage; broadcasting modifies the local copy and
`ReleaseBuffer` reconciles its length. The input owner's member is no longer
silently truncated through a const borrow. This deliberately repairs retail's
mutation behavior and changes emitted operations. The cast below has been
removed; its history remains evidence for the decision.

## Removed site and its defect

[CChatBox::HandleTextInputKey](../../src/Gruntz/ChatBoxOwner.cpp#L63) previously contained:

```cpp
char* input = const_cast<char*>(static_cast<const char*>(m_gameText->GetInputText()));
multi->BroadcastChatLine(input, 1, 1, NULL);
```

The cast bypasses CString's mutation contract. The caller's storage is mutable, so this is **not evidence of writing a const-declared object**. Retail nevertheless writes that storage without maintaining CString's cached length. The temporary is not automatically dangling: on the observed, unlocked path, the owning `CGameText` still holds the shared allocation after the temporary dies. Neither observation makes the API usage sound.

## Owner and SDK contract

[GetInputText](../../include/Gruntz/FontConfig.h#L36) returns `CString` **by value**, copying the mutable `m_inputText` member. It does not return a writable buffer or a reference to that member.

The project's [pinned VC5 toolchain](../toolchain-vc50-sp3.md) includes the actual MFC implementation; [the packager](../../scripts/create-toolchain-release.py#L211) preserves its source under `msvc/mfc-src`. These are the relevant SDK definitions, inspected in that package:

| SDK location | Actual behavior |
| --- | --- |
| `msvc/include/AFX.INL`, `CString::operator LPCTSTR() const`, line 142 | Returns the existing data pointer. In this narrow build, `LPCTSTR` is `const char*`; conversion neither detaches storage nor grants mutable access. |
| `msvc/mfc-src/STRCORE.CPP`, CString copy constructor, line 48 | Shares an unlocked allocation and increments its reference count. A locked source takes a separate copy path. |
| Same file, `CString::GetBuffer(int)`, line 412 | Returns mutable `LPTSTR` storage, detaching if shared and allocating more space if needed. |
| Same file, `CString::ReleaseBuffer(int)`, line 435 | Updates the stored length and terminator; the default length is determined from the first NUL. |

The complete current `CGameText` input-owner census contains no `LockBuffer` call. A locked-source copy would change the temporary-lifetime analysis; that is not the observed path. Modern [Microsoft CString documentation](https://learn.microsoft.com/en-us/cpp/atl-mfc-shared/cstring-operations-relating-to-c-style-strings?view=msvc-170#modifying-cstring-contents-directly) also specifies acquiring a writable buffer and releasing it after mutation. Its modern `CStringT` description is not evidence for VC5's sharing implementation; the SDK above supplies that evidence.

## What retail actually does

[BroadcastChatLine](../../src/Gruntz/Multi.cpp#L2348) genuinely mutates its `char*` argument:

- It terminates input longer than 128 bytes at index 128.
- It removes up to two trailing characters whose signed `char` value is below a space.
- It never receives the CString owner and never updates CString length metadata.

These are observed retail operations, not inferred from our parameter spelling. At RVA `0xbb190`, offsets `+0x4f`, `+0x66`, and `+0x75` contain the terminating stores. The trailing-character branches use signed comparisons.

Retail `HandleTextInputKey` at RVA `0x205c0` calls the getter at `+0x69`, obtains its data pointer at `+0x6e`, destroys the temporary at `+0x74`, and calls the broadcaster at `+0x82`. There is no `GetBuffer`/`ReleaseBuffer` pair. The getter at `0x20ef0` invokes CString copying on the member at owner offset `+0x1c`. Combined with the actual SDK, this establishes the surviving member ownership and the bypassed bookkeeping.

[HandleInputChar](../../src/Gruntz/FontConfig.cpp#L262) limits ordinary input to 80 bytes, so the 128-byte truncation does not occur through this UI. It accepts values through `0xff`; with the observed signed-char comparison, trailing high-bit bytes can still trigger removal. Enter sets input inactive before returning. [EndInput](../../src/Gruntz/FontConfig.cpp#L302) empties the member only when active, so it does not guarantee an immediate metadata repair after sending.

## Provenance and mistaken assumptions

| Change | What it establishes |
| --- | --- |
| [976212cfc7](https://github.com/sushi-shi/gruntz-decomp/commit/976212cfc7f288bc364c03fa5b1b3eaaaed58ef9) | Introduced the by-value input getter declaration while reconstructing the multiplayer key handler. Its declaration explicitly described member-copy ownership. |
| [d6cd75bfcb](https://github.com/sushi-shi/gruntz-decomp/commit/d6cd75bfcb9aee09f99185a8de2b9e123e2dcf12) | Introduced this named cast with the reconstructed chat-input body. The commit describes finishing bodies; it provides no separate CString safety justification. |
| [39b084e886](https://github.com/sushi-shi/gruntz-decomp/commit/39b084e8868d43c6de1cfaa55088cd00c7755278) | Established the current getter-temporary form while recovering input parsing. |
| [57f133453e](https://github.com/sushi-shi/gruntz-decomp/commit/57f133453e9ac9d2b7128e21ebae2258bbbecb07) | Put the mutable broadcaster declaration on canonical `CMulti` during owner consolidation. This is reconstructed declaration history, not surviving original C++ mangling. Actual stores independently justify mutability. |

The `const` pointer comes from **real SDK CString conversion**, not an invented `const` parameter in `BroadcastChatLine`. Making the broadcaster's input const would contradict its writes. Conversely, describing this solely as a harmless cast because the allocation is writable overlooks its stale-length behavior. Describing it solely as a dangling temporary overlooks the member's reference.

## Correct mutable-access analogue and alternatives

Microsoft's [WordPad sample, pinned at 9e1d447555](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpad.cpp#L581-L584), obtains writable CString storage for a file dialog and releases it after the dialog returns. This is an actual API-use analogue, **not original Gruntz source evidence**.

The current repair keeps a named CString copy alive, acquires its writable buffer, broadcasts, then releases it. Acquiring a shared copy's buffer detaches it: the member remains unchanged, unlike retail. Mutating the owner's buffer and releasing it would instead update the member's metadata, also unlike retail. A separately owned character copy is another explicit policy choice. These are behavior changes requiring a decision about ownership, not interchangeable cast removals.

The independent, confirmed 128-byte chat-edit overflow is documented in [chat-dialog](chat-dialog.md#separate-buffer-overflow). It must not be conflated with const-object undefined behavior at this cast.

# `m_str = CString()` empty-assign diverges: retail inlines m_pchData=0, our MFC calls ??0CString
tags: cpp:mfc cpp:ctor cpp:eh | asm:mov asm:call | topic:wall topic:eh
symptoms: ??0CString@@QAE@XZ call present in recompile / absent in retail; retail `mov [temp],0`; operator= from empty CString; ~49% on a small /GX CString setter; fs:0 read before push -1
confidence: 7/10

Assigning a default-constructed empty CString into a member —
`m_str = CString();` — that lowers to `operator=(const CString&)` from a freshly
empty temp. **Retail builds the empty temp INLINE** (`mov [temp.m_pchData], 0` —
its MFC `CString::Init()` seeds `_afxPchNil`/0), so there is NO ctor call. Our
packaged MFC's `CString::CString()` -> `Init()` instead sets
`m_pchData = afxEmptyString.m_pchData` and is emitted as an **out-of-line
`??0CString@@QAE@XZ` call**. The operation is correct; only the empty-init lowering
differs (a build/version divergence in the static MFC, not a source-spelling
choice). The same TUs also show the /GX prologue reading `fs:0` BEFORE `push -1`
where the recompile pushes first.

```cpp
m_str = CString();   // correct op; can't steer away the ??0CString call
```
```asm
; retail: empty temp built inline, no ctor
lea  eax,[esp+0x10]      ; &temp
add  ecx,0x59c           ; &m_str
push eax
mov  [esp+0xc],0         ; trylevel (temp.m_pchData already 0 inline)
call ??4CString@@...     ; operator=
; recompile: extra ??0CString@@QAE@XZ default-ctor call before operator=
```
WALL (MFC build divergence) — CMulti::ClearString59c/ClearString5a0 @0xb76c0/0xb7730 plateau ~49%; operation byte-correct apart from the inlined-vs-called empty init + /GX prologue order. Defer to the final sweep (would need a matching MFC `Init()` or a hand-inlined empty-CString temp).

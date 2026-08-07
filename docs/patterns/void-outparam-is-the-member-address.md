# A Win32 `void**` out-param takes the MEMBER's address - a local plus a copy costs the copy
tags: cpp:call cpp:member cpp:cast | asm:lea asm:mov | topic:codegen-idiom topic:mis-model
symptoms: lea ecx,[esi+0x42c] pushed as ppvBits, no post-call store, CreateDIBSection, QueryInterface
confidence: 10/10

The SDK types the out-parameter `void**`, so a naive transcription passes a local
and copies it into the member afterwards. Retail passes `&m_member` directly:
the tell is the pushed pointer being `[this+N]` (`lea ecx,[esi+0x42c]`) with **no**
`mov [esi+0x42c],reg` after the call, and a base that is exactly one load+store
long per call site.

```cpp
// PtrOut() (include/ComOutRef.h) feeds a typed T** through the ComOutRef union,
// so the site needs no cast and the reinterpret_cast ratchet does not move.
m_dibSection = CreateDIBSection(dc, &m_bmi, DIB_PAL_COLORS, PtrOut(&m_pixels), 0, 0);
```
```asm
lea    ecx,[esi+0x42c]       ; &m_pixels, straight into ppvBits
push   ecx
call   DWORD PTR ds:[__imp__CreateDIBSection]
mov    DWORD PTR [esi+0x428],eax     ; only the RETURN is stored
```

Steerable. CRezImage::DecodeBmpHeader 0x1757c0 98.01 -> 100.00 EXACT. Spelling
matters for cleanliness only, and only the inline is byte-neutral: a plain
`reinterpret_cast<void**>` also reaches 100 but raises the cast ratchet by one per
site, while a **named union local** (`ComOutRef<u8> bits; bits.m_asTyped = &m_pixels;`)
costs 4.6% because cl gives the union a frame slot.

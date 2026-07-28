# `push edx` with only `dl` defined proves a BYTE parameter — even when the callee masks

tags: cpp:param cpp:call asm:push asm:shl | topic:codegen-idiom topic:mis-model
symptoms: base has a caller-side `and reg,0xff` / `movzx` that retail does not,
retail `mov dl,[..]; shl dl,1; push edx` with edx's upper bytes stale, the callee
opens with `mov edx,[esp+N]; and edx,0xff`
confidence: 9/10

An argument that retail ships as `mov dl,<byte>; …; push edx` — with the upper 24
bits of `edx` left over from an earlier unrelated value — is **not** an `int`
argument that happens to be small. cl only ever leaves an argument's high bytes
undefined when the *parameter* is one byte wide, so the declaration is `u8`/`char`,
not `i32`. Do not be misled by the callee: a `u8` parameter read as an int lowers to
`mov edx,[esp+N]; and edx,0xff` — the same instructions an explicit
`i32 p; … p & 0xff` produces, so the callee alone cannot distinguish the two.
The CALLER is what settles it.

Modelling it as `i32` forces the caller to materialize a real 32-bit value
(`and edx,0xff`, or a `movzx`), which is one extra instruction at every call site,
and no amount of `static_cast<u8>(...)` in the argument expression removes it.

```cpp
// before - i32 param: the caller must widen
void ArmSlot(void* node, i32 parity);        // callee: parity & 0xff
… ArmSlot(node, static_cast<i32>(static_cast<u8>(static_cast<u8>(m_5a4) << 1)));
//   mov dl,[esi+0x5a4] / shl dl,1 / and edx,0xff / push edx

// after - u8 param: the caller ships dl and nothing else
void ArmSlot(void* node, u8 parity);         // callee: (m_tick + parity) % 128
… ArmSlot(node, static_cast<u8>(static_cast<u8>(m_5a4) << 1));
//   mov dl,[esi+0x5a4] / shl dl,1 / push edx
```

```asm
caller: mov dl,BYTE PTR [esi+0x5a4] | shl dl,1 | push edx
callee: mov edx,DWORD PTR [esp+0x8] | and edx,0xff | …
```

STEERABLE, and it pays on BOTH sides of the call. Retyping the parameter changes the
mangled name (`…@Z` gains `E` for `unsigned char`), which is byte-neutral here because
call relocs are masked — but check every other call site first. Evidence (2026-07-28):
`CNetSession::ArmSlot` @0x0c03f0 98.85 → **100 EXACT** (its residual had been filed a
"eax/edx role swap … not source-steerable") and its only caller `CMulti::Render`
@0x0b6890 98.65 → 99.26 in the same change.

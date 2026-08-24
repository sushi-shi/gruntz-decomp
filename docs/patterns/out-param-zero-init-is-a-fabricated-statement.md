# A `= NULL` on a pure out-param local is a statement retail never executed
tags: cpp:local cpp:pointer msvc5:win32 | asm:mov | topic:codegen-idiom topic:correctness
symptoms: one extra `mov DWORD PTR [esp+N],0` near the top; frame 4 bytes larger per initialiser; every later stack displacement shifts; `walls diagnose` says REGALLOC/SCHEDULING with a 1-4 instruction delta; `walls semdiff` shows no exclusive key
confidence: 10/10

A local that exists only to receive an API's out-parameter is written
`T* p;` in retail, not `T* p = NULL;`. The defensive initialiser is a
reconstruction habit, and it is not free: the address escapes into the call,
so the variable already has a frame home, and the extra store is emitted
verbatim. Worse, the extra `= 0` participates in I1's zero-hoist census, so
one initialiser too many can dedicate a callee-saved register to the literal
zero and re-colour the whole body.

```asm
; base                                   ; retail
lea  eax,[esp+0x1c]                      lea  eax,[esp+0x1c]
push 0x80                                push 0x80
push eax                                 push eax
mov  DWORD PTR [esp+0x20],0   ; <- EXTRA mov  ecx,[eax]
```

## Fix

```cpp
// NO
u8* audioPtr1 = NULL;
u8* audioPtr2 = NULL;
DWORD audioBytes1;
DWORD audioBytes2;
m_buffer->Lock(0, bytes, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);

// YES - the DWORD siblings were never initialised either; the pointers are
// exactly as much of an out-param as they are
u8* audioPtr1;
u8* audioPtr2;
DWORD audioBytes1;
DWORD audioBytes2;
```

STEERABLE, measured 2026-08-21:
`SoundBuffer::LockConvert` 0x135f40 90.09 -> **100.00 EXACT**,
`SoundBuffer::LoadFromFile` 0x135e10 97.73 -> **100.00 EXACT**,
`StreamFeeder::FillBuffer` 0x137f30 86.66 -> **100.00 EXACT**,
`WinMain` 0x11c860 99.59 -> **100.00 EXACT** (the whole `winmain` unit to
100%) - there the local is `char* pValue` feeding `VerQueryValueA`.

## Bounds - the initialiser is REAL at these sites, do not sweep

The tell is whether the callee's contract leaves the sink untouched on some
path. Where it does, retail zeroes it too. A tree-wide scan for the shape
found 11 sites; **five of them are already EXACT with the `= NULL` and must
keep it**: `CNetMgr::Init`, `EnumProviderCb`, `EnumSurfacesCallback`,
`CDDrawDeviceManager::WrapAttachedSurface` and `::GetGDISurface` all pass the
address to a DirectX/DirectPlay enumeration or attachment API that can return
without writing. The related
[typed-map-walk-helper-hides-the-key-out-param](typed-map-walk-helper-hides-the-key-out-param.md)
is the opposite case: `CMapPtrToPtr::GetNextAssoc` has retail zeroing BOTH
sinks every iteration, and dropping either one costs bytes.

So: check the byte evidence per site. The frame-size delta
`(target - base) / 4` from `walls diagnose --asm` counts the homed dwords, and
a `mov [esp+N],0` present on one side only names the offender directly.

# Stack-buffer SIZES are load-bearing for the frame size + slot offsets — match the buffer declarations to the target's `sub esp,N`
tags: cpp:local | asm:sub | topic:codegen-idiom
symptoms: `sub esp,N` is the wrong N, or a stack object/buffer lands at the wrong `[esp+M]`; slot NAMES are free at /O2 but sizes are not
confidence: 8/10

At /O2 stack-slot NAMES are free (renaming is a byte-identical no-op), but buffer SIZES and the
SET of declared locals drive the frame size (`sub esp,N`) and where each object/buffer lands.
Match the target by sizing each buffer to what the disasm implies: read the frame from `sub esp,N`
and the object landing offsets from the `lea [esp+M]` references, then pick buffer sizes that sum
to them. Two paths that SHARE a buffer get one slot; live-range-distinct buffers get separate
slots even if same-typed.

```cpp
char value[32], drivePath[32], exePath[256];   // 0x10 + 32 + 32 + 256 = 0x150 → reg obj at [esp+0x150]
```
STEERABLE. Evidence: GetGruntzDriveLetter `sub esp,0x460` + reg object at `[esp+0x150]` required
value[32]/drivePath[32]/exePath[256] + a second distinct scan buffer after reg; CFileIO::Open
szPath = `char[0x104]` (MAX_PATH) → `sub esp,0x110`; ShowError detail `char[0x20]` → `sub esp,0x20`;
WwdFile::CheckHeader `char[0x5F4]` + 0x10 stream obj → `sub esp,0x604`.

# Two reloads in the wrong order mean a RECYCLED spill slot - fix the value's creation index
tags: cpp:local cpp:loop cpp:init | asm:mov asm:rep | topic:codegen-idiom topic:regalloc
symptoms: two adjacent `mov <reg>,[esp+N]` reloads after an inlined `rep movs`, same two
instructions on both sides, transposed; every other byte of the function agrees; 99.9%
confidence: 9/10

cl 5.0 emits a group of reloads in the values' CREATION order, which normally equals
ascending frame-slot order because slots are handed out in that order. The two disagree
when a slot is RECYCLED from a dead value: the later variable inherits the earlier
variable's low slot but keeps its own later creation index, and the reload group comes
out descending. Declaring the recycling variable up front with an initializer moves its
creation index ahead; the initializer is dead-store eliminated, so the fix costs nothing.

```cpp
// NO - `snap` is created after pModuleNext, but recycles pCreateSnapshot's slot +0x10,
//      so the pair reloads edi(+0x14) then esi(+0x10)
PFNMODULEWALK pModuleNext = ...;                   // spilled to [esp+0x14]
HANDLE snap = pCreateSnapshot(TH32CS_SNAPMODULE, th32ProcessID);   // reuses [esp+0x10]

// YES - retail's own MSDN GetProcessModule shape; esi(+0x10) then edi(+0x14)
HANDLE snap = NULL;
...
PFNMODULEWALK pModuleNext = ...;
snap = pCreateSnapshot(TH32CS_SNAPMODULE, th32ProcessID);
```
```asm
    rep    movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
    mov    esi,DWORD PTR [esp+0x10]    ; ASCENDING slot order
    mov    edi,DWORD PTR [esp+0x14]
    test   ebx,ebx
```
Steerable. `Utils::WinAPI::LegacyFindModule` 0x00118f60 **99.90 -> 100.00 EXACT**.
Census: every post-`rep movs` esi/edi reload pair in the tree is 3 sites; retail is
ascending slot order at all three and we matched two - the third was the only recycled
slot. Sixteen other source shapes (union vs `reinterpret_cast`, flag before/after the
copy, `CopyMemory`, `BOOL` flag, MSDN's two-flag `bRet`/`bFound`, pointer/count/handle
locals, `continue`, `if (found) break;`, a `while` head, and the SAME declaration
WITHOUT an initializer) are byte-identical to the 99.90 baseline; all four spellings
WITH an initializer reach 0. Declaration-count probes (typedef 0..24, class 0..16) leave
the obj byte-identical, positive control `found = 1` -> `found = 2` moves it - so this
TU is not declaration-count steerable and the lever is the creation index alone.

related: [late-store-keeps-the-loaded-byte-in-its-own-register](late-store-keeps-the-loaded-byte-in-its-own-register.md)

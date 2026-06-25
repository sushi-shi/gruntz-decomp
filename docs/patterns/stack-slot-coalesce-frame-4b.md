# Two mutually-exclusive branch locals: retail keeps DISTINCT slots, cl COALESCES → frame 4 B short
tags: cpp:local cpp:branch | asm:sub asm:lea | topic:wall topic:regalloc
symptoms: body logic byte-identical, retail `sub esp,N` vs recompile `sub esp,N-4`, every `[esp+M]` shifted by 4, ~88%; a return-value spill instead of using `eax`
confidence: 6/10

A function has two locals that live in DIFFERENT, mutually-exclusive branches (e.g. a read path
declares a lookup-result `out`, a write path declares a `CString` temp). MSVC 5.0 /O2 sees their
live ranges never overlap and COALESCES them onto one stack slot, so the frame is one dword
smaller than retail (`sub esp,0x84` vs retail `sub esp,0x88`). Retail's allocator gave each its
own slot (`out@[esp+0x10]`, `CString@[esp+0x14]`, `buf@[esp+0x18]`); every subsequent `[esp+M]`
reference is then 4 bytes apart, and the coalescing also changes whether a callee's return value is
kept in `eax` (retail `mov edi,[eax]`) or re-loaded from the shared slot (`mov edi,[esp+M]`).

```cpp
// Both `out` (read branch) and `nm` (write branch) are branch-local and never
// co-live, so cl coalesces them → one slot, frame 4 B short. Retail keeps two.
if (mode == 7) { CObject* out = 0; map.Lookup(key, out); m_0c = out; }
if (mode == 4) { CString nm = leaf.KeyOfValue(m_0c); strcpy(buf, (const char*)nm); }
```
```asm
; retail: sub esp,0x88  (out@0x10, CString@0x14, buf@0x18) ; mov edi,[eax]
; cl:     sub esp,0x84  (coalesced)                        ; mov edi,[esp+0x10]
```
WALL. Not steerable under /O2: hoisting `out` to function scope, inner-block reshapes, and
swapping the branch order all regressed (`CSerialSub34::Chain` 0x8c00: 88.0%; the read/write
dispatch, Lookup/KeyOfValue chain, inline strlen/strcpy, and /GX-elided CString temp are all
exact — only the one coalesced slot + its cascade differ). Sibling of
gx-scoped-local-eh-frame-size.md (that is the /GX-frame 4 B variant; this is the frameless
coalesce variant). Deferred to the final sweep.

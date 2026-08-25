# Two `repnz scas` before ONE `rep movs` is inline `strcat`, not `strcpy(x + strlen(x), y)`

tags: cpp:string cpp:call | asm:scasb asm:movs asm:lea | topic:codegen-idiom
symptoms: an append site where retail scans TWICE and you scan once; retail has
`dec edi` before the copy where you have `lea edx,[esp+ecx+0xNNN]`; retail loads the
SOURCE pointer first (`mov edi,[<addr>]` for a global literal) and you load the
destination first; the two sides otherwise use the same registers and buffers
confidence: 9/10

`strcat(dst, src)` and `strcpy(dst + strlen(dst), src)` do the same thing and both
inline, but cl 5.0 expands them in the OPPOSITE order and they are trivially told
apart. `strcat` measures the SOURCE, then scans the destination to its NUL and
backs up one byte (`dec edi`). The `strcpy` spelling measures the DESTINATION with
its own `strlen`, materialises `dst+len` with a `lea`, and only then measures the
source.

```cpp
strcat(pattern, g_sepSlash);                        // retail
strcpy(pattern + strlen(pattern), g_sepSlash);      // ours - a different expansion
```
```asm
; strcat: source first, then walk the destination and step back over its NUL
mov  edi,[<addr>]                ; src
or   ecx,0xffffffff
xor  eax,eax
lea  edx,[esp+0x554]             ; dst - loaded, NOT scanned yet
repnz scas al,BYTE PTR es:[edi]  ; strlen(src)
not  ecx
sub  edi,ecx
mov  esi,edi
mov  ebx,ecx                     ; keep the SOURCE length
mov  edi,edx
or   ecx,0xffffffff
repnz scas al,BYTE PTR es:[edi]  ; walk dst to its NUL
mov  ecx,ebx
dec  edi                         ; ... and back up onto it
shr  ecx,0x2
rep movs ...

; strcpy(dst+strlen(dst), src): destination first, then a lea
lea  edi,[esp+0x554]
or   ecx,0xffffffff
xor  eax,eax
repne scas al,BYTE PTR es:[edi]
not  ecx
dec  ecx
mov  edi,[<addr>]                ; src loaded AFTER
lea  edx,[esp+ecx+0x554]         ; the tell: an indexed lea onto dst
```

STEERABLE, and it cascades: `CRezArchive::ImportDirectoryTree` 0x13b300 had five append
sites, and switching all five to `strcat` took the masked diff from 241 differing
rows to 60 and also settled the frame-layout order of two buffers that the hand
spelling had swapped (79.76 -> 95.08 on its own; 99.67 with the rest of the
function's fixes). Note the disassembler prints the same `F2 AE` prefix as `repne`
on our side and `repnz` on retail's - that pair is never a real difference.

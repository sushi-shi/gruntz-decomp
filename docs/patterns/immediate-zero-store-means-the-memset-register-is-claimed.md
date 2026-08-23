# An IMMEDIATE zero store after a `rep stos` means the load was ordered first

tags: cpp:local cpp:member cpp:call | asm:xor asm:rep asm:stos asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail writes `mov DWORD PTR [esp+N],0x0` right after a `rep stos`, ours
writes `mov DWORD PTR [esp+N],<reg>` reusing the register the stos expansion left
holding zero (and then compares against that register, `cmp r,r`, where retail has
`test r,r`); `walls residue` shows `mov [esp+?],0x0` against `mov [esp+?],r`
confidence: 8/10

cl 5.0 lowers `memset(buf, 0, N)` to `xor eax,eax / lea edi,buf / rep stos`. A
scalar `i32 zero = 0;` written next reuses that live zero register - unless the
statement between them already claimed it. Ordering the pointer read BEFORE the
zero initializer is what forces the immediate form.

```cpp
memset(tmp, 0, sizeof(tmp));
CImage* frame = m_frame;      // claims EAX first
i32 zero = 0;                 // -> mov DWORD PTR [esp+0x10],0
if (frame) {
    reg->AnyValueMatches(frame, tmp, &zero);
}
```
```asm
rep stos DWORD PTR es:[edi],eax
mov  eax,DWORD PTR [ebp+0x10]        ; the load first
mov  DWORD PTR [esp+0x10],0x0        ; immediate, not `mov [esp+0x10],eax`
test eax,eax                         ; not `cmp eax,<zero reg>`
```
STEERABLE, but PER FUNCTION - it depends on what else holds zero in that block.
`CActionOptionsMenuBar::Serialize` 95.87 -> 99.78 (residue 11 -> 1) at all three
sites; `CTimer::Serialize` is EXACT with the opposite order, so check the pair
before reordering.

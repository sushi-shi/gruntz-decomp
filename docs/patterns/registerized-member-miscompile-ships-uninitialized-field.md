# A kept-but-never-stored flag chain means the source wrote the STRUCT MEMBER, and cl's registerization miscompile ate the store

tags: cpp:struct cpp:local cpp:member | asm:or asm:mov asm:rep-movsd | topic:codegen-idiom topic:correctness
symptoms: retail computes a value chain (init + conditional ORs) into a callee-saved
register and then CLOBBERS it without ever storing it; one field of a by-value
struct argument is never written on the retail side; our reconstruction with a
local + final `field = local` store scores ~90 with the chain in eax (`or ah,1` /
`or al,-0x80` byte forms) and an extra spill before the call
confidence: 10/10 (CDDrawShadeBlit::Rebuild 0x1493b0, 89.88 -> 100.00 EXACT)

## The shape

Retail `Rebuild` fills a 8-dword `CImageFrameRebuildDesc` and passes it BY VALUE
to `DecodeFrame`. The disassembly shows stores to seven of the eight slots -
`f1`'s slot is never written - yet a full flag computation survives:

```asm
mov  esi, 0x3d          ; flags init
...
mov  esi, 0xbd          ; m_palette arm
or   esi, 0x100         ; m_colorKey arm
or   esi, 0x80          ; m_palette arm
...
lea  esi, [esp+0x2c]    ; argument-copy source pointer CLOBBERS the value
rep  movsd              ; the by-value copy reads f1's never-written memory slot
```

Shipped retail passes an UNINITIALISED `f1` (and `DecodeFrame` tests
`desc.f1 & 0x80` on that garbage). The arithmetic is dead in fact but live in
cl's model.

## The source that reproduces it

Write the field directly - no local, no final assignment:

```cpp
CImageFrameRebuildDesc desc;
desc.f0 = 0;
desc.f1 = 0x3d;
if (m_palette != NULL) desc.f1 = 0xbd;
...
if (m_colorKey != -1) { desc.f6 = (u8)m_colorKey; desc.f1 |= 0x100; }
if (m_palette != NULL) desc.f1 |= 0x80;
return DecodeFrame(name, desc);
```

cl 5.0 registerises `desc.f1` in a callee-saved register, keeps every OR (it
believes the member is live into the copy), and never writes it back before the
`rep movsd` argument copy - reproducing the miscompile byte-for-byte, callee-saved
seat and 32-bit OR forms included.

The `i32 flags = ...; desc.f1 = flags;` spelling can NOT reach this cell from
either side: with the store, cl emits a real f1 write (+ a caller-saved seat, so
the ORs become `or ah/al` byte forms and the value needs a spill across the
`CString` copy-ctor call); without it, the whole chain folds away as dead.

## Detection signature

A retail-side value chain whose result is provably never stored or consumed, in a
function passing a struct by value, with one struct field never written: suspect
the field itself was the accumulator. The reverse tell in the reconstruction is
the byte-half OR forms (`or ah,1`) - a caller-saved seat where retail has a
callee-saved one - plus one extra store retail lacks.

Two sibling levers closed the rest of the function (both plain statement order):
the `f0 = 0` init belongs FIRST (retail schedules its store into the first
compare's shadow), and within the colorKey arm the byte store precedes the `|=`.

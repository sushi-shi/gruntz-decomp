# A kept-but-never-stored flag chain means the source wrote the STRUCT MEMBER, and cl's registerization miscompile ate the store

tags: cpp:struct cpp:local cpp:member | asm:or asm:mov asm:rep-movsd | topic:codegen-idiom topic:correctness
symptoms: retail computes a value chain (init + conditional ORs) into a callee-saved
register and then CLOBBERS it without ever storing it; one field of a by-value
struct argument is never written on the retail side; our reconstruction with a
local + final `field = local` store scores ~90 with the chain in eax (`or ah,1` /
`or al,-0x80` byte forms) and an extra spill before the call
confidence: 10/10 (CDDrawShadeBlit::SavePid 0x1493b0, 89.88 -> 100.00 EXACT)

## The shape

Retail `SavePid` fills an 8-dword `PidWriteHeader` and passes it BY VALUE
to `WritePidFile`. The disassembly shows stores to seven of the eight slots -
`flags`' slot is never written - yet a full flag computation survives:

```asm
mov  esi, 0x3d          ; flags init
...
mov  esi, 0xbd          ; m_palette arm
or   esi, 0x100         ; m_colorKey arm
or   esi, 0x80          ; m_palette arm
...
lea  esi, [esp+0x2c]    ; argument-copy source pointer CLOBBERS the value
rep  movsd              ; the by-value copy reads flags' never-written memory slot
```

Shipped retail passes an UNINITIALISED `flags` field (and `WritePidFile` tests
`header.flags & 0x80` on that garbage). The arithmetic is dead in fact but live in
cl's model.

## The source that reproduces it

Write the field directly - no local, no final assignment:

```cpp
PidWriteHeader header;
header.formatTag = 0;
header.flags = 0x3d;
if (m_palette != NULL) header.flags = 0xbd;
...
if (m_colorKey != -1) { header.fill = (u8)m_colorKey; header.flags |= 0x100; }
if (m_palette != NULL) header.flags |= 0x80;
return WritePidFile(path, header);
```

cl 5.0 registerises `header.flags` in a callee-saved register, keeps every OR (it
believes the member is live into the copy), and never writes it back before the
`rep movsd` argument copy - reproducing the miscompile byte-for-byte, callee-saved
seat and 32-bit OR forms included.

The `i32 flags = ...; header.flags = flags;` spelling can NOT reach this cell from
either side: with the store, cl emits a real field write (+ a caller-saved seat, so
the ORs become `or ah/al` byte forms and the value needs a spill across the
`CString` copy-ctor call); without it, the whole chain folds away as dead.

## Detection signature

A retail-side value chain whose result is provably never stored or consumed, in a
function passing a struct by value, with one struct field never written: suspect
the field itself was the accumulator. The reverse tell in the reconstruction is
the byte-half OR forms (`or ah,1`) - a caller-saved seat where retail has a
callee-saved one - plus one extra store retail lacks.

Two sibling levers closed the rest of the function (both plain statement order):
the `formatTag = 0` init belongs FIRST (retail schedules its store into the first
compare's shadow), and within the colorKey arm the byte store precedes the `|=`.

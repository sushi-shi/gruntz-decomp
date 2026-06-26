# A global referenced by-address is a `char[]` (array), not a `char*` (pointer)
tags: cpp:member cpp:cast | asm:mov | topic:codegen-idiom
symptoms: `mov reg, OFFSET DAT_xxx` (opcode b8/b9, imm32+DIR32) vs your `mov reg, [g_x]` (opcode 8b, load); a default/fallback value that is the ADDRESS of a writable global buffer
confidence: 8/10

A field/default initialized from a writable global buffer whose **address** is
taken (`m_msg = msg ? msg : g_default;`) lowers to `mov reg, OFFSET g_default`
(a 5-byte `b8/b9 imm32` with a DIR32 reloc on the immediate). If you model the
global as a `char*` POINTER, the same source emits a 6-byte `mov reg, [g_default]`
LOAD instead — different opcode, size, and one real byte off. Declare the global
as an **array** so its name decays to its address.

```cpp
static char g_defaultErrMsg[24];          // by-address buffer (decays to &g_..[0])
// NOT: static char* g_defaultErrMsg;     // a pointer -> emits a LOAD
m_msg = msg ? msg : g_defaultErrMsg;      // -> mov ecx, OFFSET g_defaultErrMsg
```
```asm
b9 00 00 00 00   mov ecx, <imm32>   ; IMAGE_REL_I386_DIR32  g_defaultErrMsg
```
Steerable. The array SIZE is reloc-masked (only the symbol address is referenced),
so any plausible size matches; the gap to the next named global gives a good guess.
CContainerErr::CContainerErr @0x16d9c0 97.1%→100% (the lone non-reloc-masked diff).

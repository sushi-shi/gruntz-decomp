# A by-value class return read through `[eax]` means the temporary was NEVER named
tags: cpp:local cpp:class cpp:temporary | asm:call asm:mov | topic:codegen-idiom
symptoms: after a `call` that returns a class by value, retail reads the result as `mov <r>,[eax]` while the recompile reads the same field as `mov <r>,[esp+N]`; the frame is one dword off and the whole arm's `[esp+N]` operands shift
confidence: 8/10

MSVC returns a class by value through a caller-supplied buffer whose address is pushed as the
hidden first argument, and it also **returns that address in `eax`**. Which of the two the caller
then reads from is a source fact:

* the result assigned to a **named local** - cl knows the slot statically and reads `[esp+N]`;
* the result used **directly as a temporary** - cl reads back through the **returned `eax`**.

```cpp
// NO - a named local; cl reads [esp+N] and the frame gains a dword
CString nm = reg->KeyOfValue(m_value);
strcpy(name, static_cast<const char*>(nm));

// YES - the temporary is consumed in place; cl reads mov edi,[eax]
strcpy(name, static_cast<const char*>(reg->KeyOfValue(m_value)));
```

```asm
lea  ecx,[esp+0x10]           ; hidden return buffer
push ecx
call ?KeyOfValue@...
mov  edi,DWORD PTR [eax]      ; <-- the tell: through the RETURN VALUE, not [esp+0x10]
```

The frame size moves with it, so this shows up as a whole-arm `[esp+N]` shift plus one
instruction of delta - not as a localized diff. Measured: `CWapX::Chain` `0x8c00`
92.65 -> 93.26 (frame `0x84` -> retail's `0x88`), and the same edit at both
`CInGameIcon::SerializeMove` sites (`m_animRegistry->KeyOfValue`,
`m_soundRegistry->FindKeyOfValue`).

Screen for it with `rg 'CString \w+ = .*(KeyOfValue|GetName|GetString)'` next to a `strcpy` /
`Write` of the same buffer.

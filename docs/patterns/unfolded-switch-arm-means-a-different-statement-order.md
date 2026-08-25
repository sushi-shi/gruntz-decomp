# A switch arm retail does NOT fold into a 13/16-byte stub has its statements in a different order
tags: cpp:switch cpp:branch | asm:jmp asm:lea | topic:codegen-idiom
symptoms: lea reg,[this+off] / push imm32 / jmp - 16 bytes | reloc addresses 13 or 16 apart | one arm full where its twins are stubs

In a big value-dispatch switch, cl 5.0 cross-jumps every arm whose body is
byte-identical to a later arm's into a stub - typically
`lea <reg>,[this+off]; push <string>; jmp <that arm's body + 5>`, i.e. **16
bytes** with a `rel32` jump or **13** with a short one. So arm boundaries are
readable straight off the string relocations: a spacing of 13 or 16 is a stub,
anything larger is a real body.

That makes the *converse* a source oracle. If three arms have identical source in
your reconstruction but retail folds only two of them, the third's statements are
in a different order - the cross-jumper compares encoded bytes, so any reordering
defeats it.

```cpp
// GUNHAT (retail case 9): a full 186-byte body
if (m_arrivalState == AI_DEFENDER) { m_defenderRadius = 1; }
if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) { m_arrivalFlags |= 0x10; }
// NERFGUN / ROCK (cases 10, 11): the other order, and they fold into each other
```

Use the relocation list, not the disassembly: `gruntz sema disasm <rva>`
prints `@<site> -> <addr>  <name>`; diff consecutive sites against your base obj's
`llvm-objdump -dr` reloc offsets and every arm whose length differs is a lead.
Twenty-one of `LoadGruntTypeTable`'s twenty-three tool arms already matched to the
byte this way, which is what isolated GUNHAT.

Steerable. `CGrunt::LoadGruntTypeTable` 0x4dd50: swapping GUNHAT's two statements
stopped cl folding it into a 13-byte stub and took 90.20 -> 92.00, closing a
169-byte length error across three arms.

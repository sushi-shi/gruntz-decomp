# ILT jump-island to a __thiscall member: __asm can't name `Class::Method`, use a placeholder extern
tags: cpp:naked cpp:thunk | asm:jmp | topic:codegen-idiom topic:scoring-artifact
symptoms: 5-byte `e9 rel32` ILT thunk whose target is a C++ member; error C2420 'illegal symbol in first operand' / C2400 'inline assembler syntax error' on `jmp Class::Method`
confidence: 9/10

An incremental-link jump island (`jmp rel32` shell, RVA size 0x5) is modeled as a
`__declspec(naked)` body with one `__asm { jmp Target }` (see
naked-asm-funclet-reconstruction.md). When the target is a FREE function, declare
it with its exact retail signature (incl. namespaces via a `using`, and
`struct HWND__;` to spell `HWND` without a heavy header) so the jmp reloc resolves
to the retail symbol → exact. But MSVC5 inline asm CANNOT parse a qualified
`Class::Method` operand (C2420/C2400), so an island to a `__thiscall` member has no
way to name the real symbol; jump through a reloc-masked placeholder extern instead
— the `e9` opcode matches byte-exact and the rel32 is fully reloc-masked, so the
differing symbol name on the masked operand does NOT block the match (still scores
exact in the build).

```cpp
extern "C" void RunErrorDialog_0bc250(); // placeholder for CMulti::RunErrorDialog (member; un-nameable in __asm)
__declspec(naked) int __stdcall MultiDispatch(int, int, int) {
    __asm { jmp RunErrorDialog_0bc250 }
}
```
```asm
00403cab: e9 a0 85 0b 00   jmp 0x4bc250   ; -> CMulti::RunErrorDialog (reloc-masked)
```
Steerable→exact for both free-function targets (CheckExePath/Eng_RegionCueA/
StartupGate islands) and the member target via placeholder (_MultiDispatch@12
0x3cab); all in EngineExternFns.cpp.

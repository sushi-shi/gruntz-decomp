#include <rva.h>
// CGruntzCommand.cpp - residual engine-label stubs for CGruntzCommand.
//
// The real class (base + leaves) lives in src/Gruntz/GruntzCommand.cpp; its
// ~CGruntzCommand (??_G, 0x024330) and the two leaf allocators are matched
// there. These two 7-byte void-thiscall helpers are out-of-line base-vftable
// stamps: `mov [this],&vftable; ret` (void, no eax-return, no null guard). A
// plain void __thiscall method whose only body is the vftable store lowers to
// exactly `c7 01 <reloc> c3`; the operand reloc-masks against the base vftable
// (0x5e9674, RVA 0x1e9674) named via DATA, same idiom as SoundStream's vptr stamp.

// CGruntzCommand base vftable (0x5e9674). Reloc-masked DIR32 store.
DATA(0x001e9674)
extern void* const g_cmdBaseVtbl[];

class CGruntzCommand {
public:
    void CGruntzCommand_0242f0();
    void CGruntzCommand_024430();
};

RVA(0x000242f0, 0x7)
void CGruntzCommand::CGruntzCommand_0242f0() {
    *(void**)this = (void*)g_cmdBaseVtbl;
}

RVA(0x00024430, 0x7)
void CGruntzCommand::CGruntzCommand_024430() {
    *(void**)this = (void*)g_cmdBaseVtbl;
}

#include <rva.h>
// CGruntzCommand.cpp - residual engine-label stubs for CGruntzCommand.
//
// The real class (base + leaves) lives in src/Gruntz/GruntzCommand.cpp; its
// ~CGruntzCommand (??_G, 0x024330) and the two leaf allocators are matched
// there. These two 7-byte void-thiscall helpers remain unmatched: their retail
// bytes are a bare `mov [this],&vftable; ret` (void, no eax-return, no null
// guard) - an out-of-line materialization of the inlined base-vftable store
// that MSVC 5.0 will NOT emit as a standalone COMDAT from the canonical class
// under the locked flags (a real ??0 emits `mov eax,ecx`; placement-new emits a
// null guard). Kept here as the documented COMDAT-duplication backlog case.

class CGruntzCommand {
public:
    void CGruntzCommand_0242f0();
    void CGruntzCommand_024430();
};

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x0242f0, 0x7)
void CGruntzCommand::CGruntzCommand_0242f0() {}

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x024430, 0x7)
void CGruntzCommand::CGruntzCommand_024430() {}

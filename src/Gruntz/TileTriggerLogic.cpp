// TileTriggerLogic.cpp - Gruntz CTileTriggerLogic (C:\Proj\Gruntz).
// The constructor is matched byte-exact.
#include <Gruntz/TileTriggerLogic.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CTileTriggerLogic::CTileTriggerLogic()
// Zeroes the 24-dword m_block array (rep stosl) then m_1c, reusing the zero
// register for the trailing +0x1c store.
RVA(0x001107f0, 0x1c)
CTileTriggerLogic::CTileTriggerLogic() {
    // m_block initialised before m_1c so the optimiser emits the rep stosl
    // first and reuses the zero register for the +0x1c store afterwards.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_1c = 0;
}

// The single slot-0 virtual (0x110c10, reached via ILT thunk 0x402072) is DECLARED
// ONLY: its body lives in an unmatched engine TU, so the ctor-emitted ??_7 vftable
// references it as an external reloc-masked slot (exactly retail's shared slot 0).
// The derived logic classes (TileTriggerDerivedCtors.cpp) inherit this one slot.
// (No virtual destructor: retail's derived vtables share this slot value, proving it
// is a normal inherited virtual, not a per-class ??_G.)

// size 0x9c recovered from operator-new sites (gruntz.analysis.news)

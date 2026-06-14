// TileTriggerLogic.cpp - Gruntz CTileTriggerLogic (C:\Proj\Gruntz).
// Matched: ??0CTileTriggerLogic@@QAE@XZ @ RVA 0x1107f0 (byte-exact).
#include "TileTriggerLogic.h"

// ---------------------------------------------------------------------------
// CTileTriggerLogic::CTileTriggerLogic()
// Zeroes the 24-dword m_block array (rep stosl) then m_1c, reusing the zero
// register for the trailing +0x1c store.
//
// @address: 0x1107f0
// @size:    0x1c
// ---------------------------------------------------------------------------
CTileTriggerLogic::CTileTriggerLogic()
{
    // m_block initialised before m_1c so the optimiser emits the rep stosl
    // first and reuses the zero register for the +0x1c store afterwards.
    for (int i = 0; i < 24; i++)
        m_block[i] = 0;
    m_1c = 0;
}

// Out-of-line stubs anchor ??_7CTileTriggerLogic@@6B@ in this TU (not matched).
CTileTriggerLogic::~CTileTriggerLogic() {}
int CTileTriggerLogic::TileLogicVfunc0() { return 0; }

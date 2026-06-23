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
    for (int i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_1c = 0;
}

// Out-of-line stubs anchor the CTileTriggerLogic vftable in this TU (not matched).
CTileTriggerLogic::~CTileTriggerLogic() {}
// Scalar-deleting dtor (??_G, slot 0): a compiler-generated thunk wrapping the real
// ~CTileTriggerLogic cleanup (0x32c bytes - substantial; not reconstructed, so this
// only NAMES the retail function). MSVC synthesizes ??_G from the virtual dtor above.
// @rva-symbol: ??_GCTileTriggerLogic@@UAEPAXI@Z 0x00116610 0x32c
int CTileTriggerLogic::TileLogicVfunc0() {
    return 0;
}

// size 0x9c recovered from operator-new sites (gruntz.analysis.news)
SIZE(CTileTriggerLogic, 0x9c);

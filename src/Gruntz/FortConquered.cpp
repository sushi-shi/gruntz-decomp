// FortConquered.cpp - CExitTrigger::AdvanceAnim @0x03f5f0 (1318 B), the
// exit-trigger's per-frame fort-conquest check.
//
// original TU: filename unknown (@identity-TODO). Split out of FortressFlag.cpp
// (wave3-I): the retail body's BIRTH POSITION is the lone 0x3f5f0-0x3fb16 text
// interval between the WormholeActs block (0x3f210-0x3f57d) and the wormhole trio
// (0x3fc70+), and its three private .data cells (0x20d154/0x20d168/0x20d16c) sit
// BEFORE the wormhole trio's band (0x20d194) in the 98%-monotone .data
// contribution order - so it CANNOT belong to the fortressflag obj at 0x45d30
// (whose band is 0x20d384+). CExitTrigger::RegisterActs proves the class identity:
// each registry-insertion arm stores ILT 0x1938 as its "A" handler, and that thunk
// jumps here. The +0x54/+0x58 accesses also agree with CExitTrigger's two derived
// fields, which do not exist in the 0x54-byte CFortressFlag.
#include <Gruntz/ExitTrigger.h>
#include <rva.h>

// Structure decoded:
// the +0x1a0 sub-clock tick, the g_gameReg->m_134 mode gate, HitTestCell +
// dedup vs owner->m_124, the 5-CString "<A> was conquered by <B>!" HUD message,
// the config re-tag, the two handler-type re-home list walks + a g_freeList pop,
// and a per-object GAME_EXPLOSION3 eye-candy spawn.
// @confidence: high
// @source: pmf-xref:registeracts-ilt-0x1938+class-layout
// @stub
RVA(0x0003f5f0, 0x526)
i32 CExitTrigger::AdvanceAnim() {
    return 0;
}

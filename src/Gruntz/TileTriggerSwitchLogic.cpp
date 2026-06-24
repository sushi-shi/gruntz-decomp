#include <rva.h>
// TileTriggerSwitchLogic.cpp - Gruntz CTileTriggerSwitchLogic (C:\Proj\Gruntz).
// CTileTriggerSwitchLogic is a tile-trigger "switch" class that manages a linked
// list (anchor at +0x04) of owned sibling CTileTriggerSwitchLogic objects.  The
// the ctor zeroes m_block and seeds m_20; FindIndexByKey scans
// the 24-dword m_block for a matching key.  RemoveByKey is still a
// backlog stub.
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <Gruntz/TileTriggerSwitchLogic.h>

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()
// Constructor: stamps the vtable, zeroes the 24-dword m_block at +0x2c
// (rep stosd), then clears m_20 (+0x20).
// ---------------------------------------------------------------------------
RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {
    for (int i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_20 = 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::FindIndexByKey
// Linear scan of the 24-dword m_block; returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
RVA(0x00110820, 0x23)
int CTileTriggerSwitchLogic::FindIndexByKey(int key) {
    for (int i = 0; i < 24; i++) {
        if (m_block[i] == key) {
            return 1;
        }
    }
    return 0;
}

// Engine-label backlog stubs (moved from src/Stub/CTileTriggerSwitchLogic.cpp).

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x00115f60, 0x2de)
void CTileTriggerSwitchLogic::CTileTriggerSwitchLogic_115f60() {}

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x00116320, 0x66)
void CTileTriggerSwitchLogic::CTileTriggerSwitchLogic_116320() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001122a0, 0x241)
void CTileTriggerSwitchLogic::BuildRockBreakInGameText() {}

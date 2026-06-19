#include "../rva.h"
// TileTriggerSwitchLogic.cpp - Gruntz CTileTriggerSwitchLogic (C:\Proj\Gruntz).
// CTileTriggerSwitchLogic is a tile-trigger "switch" class that manages a linked
// list (anchor at +0x04) of owned sibling CTileTriggerSwitchLogic objects.  The
// ctor @0x110430 zeroes m_block and seeds m_20; FindIndexByKey @0x110820 scans
// the 24-dword m_block for a matching key.  RemoveByKey (0x116320) is still a
// backlog stub.
//
// vftable @0x5eae8c.
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include "TileTriggerSwitchLogic.h"

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()  @0x110430
// Constructor: stamps the vtable, zeroes the 24-dword m_block at +0x2c
// (rep stosd), then clears m_20 (+0x20).
// ---------------------------------------------------------------------------
RVA(0x110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()
{
    for (int i = 0; i < 24; i++)
        m_block[i] = 0;
    m_20 = 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::FindIndexByKey  @0x110820
// Linear scan of the 24-dword m_block; returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
RVA(0x110820, 0x23)
int CTileTriggerSwitchLogic::FindIndexByKey(int key)
{
    for (int i = 0; i < 24; i++) {
        if (m_block[i] == key)
            return 1;
    }
    return 0;
}

#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiConfig.h> // canonical config-host family (one shape)
#include <Gruntz/SbiDtorChain.h>
// SBI_WellGooEh.cpp - the /GX EH-framed CSBI_WellGoo destructor (C:\Proj\Gruntz).
// The split off the frameless sbi_wellgoo TU is matching-neutral (RVA-keyed).
// Chain: CSBI_WellGoo : CSBI_Image : CSBI_RectOnly : CStatusBarItem (bases shared
// via SbiDtorChain.h). ~CSBI_WellGoo frees its owned goo surface (base-region
// storage at +0x34) back to the surface pool reached via the +0x24 config host,
// then MSVC folds the three non-trivial base dtors in behind the /GX SEH frame.

// The config host + its +0x1c surface pool come from the shared canonical family
// (<Gruntz/SbiConfig.h>): CSbiConfigHost / CSbiSurfacePool.

// CSBI_WellGoo most-derived (vtable 0x5eadfc, 12 slots; overrides the vdtor). Adds
// no new storage the dtor touches - m_configHost/m_ownedSurface live in the base.
struct CSBI_WellGoo : CSBI_Image {
    virtual ~CSBI_WellGoo();
};

RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    if (m_ownedSurface) {
        ((CSbiConfigHost*)m_configHost)->m_1c->RemoveItemA(m_ownedSurface);
        m_ownedSurface = 0;
    }
}

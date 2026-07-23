#ifndef GRUNTZ_CGRUNTZMAPMGR_H
#define GRUNTZ_CGRUNTZMAPMGR_H

#include <Ints.h>
#include <Mfc.h>           // the real MFC CPtrArray (m_arr node table)
#include <Gruntz/MapMgr.h> // the ONE real CMapMgr base (was duplicated in this header)
#include <rva.h>

class CFileMemBase;

class CGruntzMapMgr : public CMapMgr {
public:
    ~CGruntzMapMgr(); // 0x85d10 (CMapMgr's dtor is non-virtual, so is this)
    // slot 0 (0x085480): free the node table back to g_coordPool, then chain the base
    // grid cleanup. The dtor runs the same teardown inline.
    virtual void Reset() OVERRIDE;
    // slot 1 (0x082430): stream the node table + m_90 through the archive, then chain
    // the base probe. CGruntzMgr::BroadcastCmd drives it as a 4-arg serialization dispatch
    // (`mov eax,[ecx]; call [eax+4]`).
    virtual i32 Visit(CFileMemBase* ar, i32 b, i32 c, i32 d) OVERRIDE;

    // The level-load terrain parser (0x0810f0): allocates the grid, rolls per-cell
    // brick colours off the "Brickz" bute section, packs the cell flags, then seeds
    // moving-object footprints from the free-node pool. Body in BrickzLoad.cpp.
    i32 LoadAttributes(i32 width, i32 height); // 0x0810f0

    // ::CPtrArray, not CObArray: ~CGruntzMapMgr's member teardown calls into
    // [0x1b4f0b, 0x1b527e) (ctor 0x1b4f0b stamps ??_7CPtrArray@@6B@), not CObArray's
    // [0x1b55e9, 0x1b59cc).  The elements are raw void* nodes, not CObject*.
    CPtrArray m_arr; // +0x7c  footprint array (m_pData@+0x80, m_nSize@+0x84)
    i32 m_90;        // +0x90  cleared at LoadAttributes start (object size 0x94)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTZMAPMGR_H

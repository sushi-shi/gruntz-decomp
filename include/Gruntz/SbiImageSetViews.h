// SbiImageSetViews.h - engine-referent views the CSBI_ImageSet method TU drives.
// Moved here from the per-TU inline def in SBI_ImageSet.cpp so the shape carries a
// single shared definition (matching-neutral: only touched members are load-bearing).
#ifndef GRUNTZ_SBI_IMAGESET_VIEWS_H
#define GRUNTZ_SBI_IMAGESET_VIEWS_H

#include <Ints.h>
#include <rva.h>

struct CResMgr; // full def in <Gruntz/ResMgr.h>; only a CResMgr* member is needed here

// The g_gameReg singleton (VA 0x64556c) viewed here: m_30 is the canonical resource
// manager (CResMgr) carrying the config map sub. Typed CResMgr* so the resolve path
// reaches it with no reinterpret cast.
struct CImageSetGameReg {
    char m_pad00[0x30];
    CResMgr* m_world; // +0x30  resource manager
};
SIZE_UNKNOWN(CImageSetGameReg);

#endif // GRUNTZ_SBI_IMAGESET_VIEWS_H

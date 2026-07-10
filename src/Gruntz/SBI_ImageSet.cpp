#define SBI_DTOR_CHAIN        // enable the inline base-dtor bodies (see StatusBarItem.h)
#define SBI_OWN_IMAGESET_DTOR // this TU supplies the out-of-line ~CSBI_ImageSet (0x102000)
#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/ResMgr.h> // canonical g_gameReg->m_world view (CResMgr + CImageRegistry + CSprite)
#include <Gruntz/SBI_ImageSet.h> // canonical CSBI_ImageSet + CImageSetStream (the frameless method view)
#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton (CSpriteFactoryHolder m_world)
// SBI_ImageSet.cpp - Gruntz CSBI_ImageSet (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_ImageSet@@; the most-derived of the SBI image chain
//   CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5eac4c. The /GX chain destructor (0x102000) is defined below - the
// former SBI_ImageSetEh.cpp companion split is collapsed (retail's one TU was
// /GX). Sibling/engine callees are ILT/vtable-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only touched members/methods are
// load-bearing; every call through them is reloc-masked). CSBI_ImageSet +
// CImageSetStream now live in the canonical <Gruntz/SBI_ImageSet.h>.

// The resolved config record (Lookup result) is the CSprite the image registry
// yields; its config name string lives at record+0x24. The config map is the image
// registry's embedded m_10map (CSpriteHashTable, Lookup 0x1b8008) - the same map
// shape SetupImage uses, reached as reg->m_10->m_10map.

DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The serialize-sequence counter bumped once per non-trivial pass.
DATA(0x00229ad0)
extern i32 g_serialCounter;

// ---------------------------------------------------------------------------
// vtable slot 1 (0xe74f0): serialize the config id + name. mode 7 = load (read id,
// read name, resolve record), mode 4 = save (write id, write name from the record);
// either way chain to the base serialize and return its normalized truth.
// @early-stop
// 99.2% (entropy tail): logic + control flow + inline strcpy/strlen + the typed
// vtable (Read/WriteBytes) + the switch dispatch are all byte-exact. The only
// residual is ONE extra `mov [esp+0x18],eax` (retail keeps the dead strlen result
// live before Lookup) + the consequent 1-byte branch-displacement shifts, plus the
// reloc-masked Lookup/Read/WriteBytes/BaseSerialize/g_* operands. Naming the strlen
// result to recover the dead store regresses it (98.4%) - a non-steerable /O2
// dead-store artifact (docs/patterns/reloc-typing-vptr-global.md). Effectively done.
RVA(0x000e74f0, 0x152)
i32 CSBI_ImageSet::Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    CSpriteFactoryHolder* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }
    char buf[0x80];
    switch (mode) {
        case 7:
            s->ReadBytes(&m_38, 4);
            g_serialCounter++;
            s->ReadBytes(buf, 0x80);
            if (strlen(buf)) {
                CSprite* out;
                ((CMapStringToOb*)&reg->m_10->m_10map)->Lookup(buf, (CObject*&)out);
                m_34 = out;
            } else {
                m_34 = 0;
            }
            break;
        case 4:
            s->WriteBytes(&m_38, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            if (m_34) {
                strcpy(buf, m_34->m_name);
            }
            s->WriteBytes(buf, 0x80);
            break;
    }
    return BaseSerialize(s, mode, a3, a4) != 0;
}

// ---------------------------------------------------------------------------
// ~CSBI_ImageSet (0x102000): the /GX chain destructor - stamp ??_7CSBI_ImageSet,
// run DtorImageSet (reloc-masked), then MSVC folds the three inline base dtors in
// (??_7CSBI_Image + DtorImage, ??_7CSBI_RectOnly + DtorRect, ??_7CStatusBarItem +
// DtorStatus - the SBI_DTOR_CHAIN device; this TU owns ~CSBI_ImageSet itself via
// SBI_OWN_IMAGESET_DTOR) behind the /GX SEH frame with 0/1/2/-1 trylevels.
// Collapsed from SBI_ImageSetEh.cpp (4-level case of
// docs/patterns/eh-dtor-multilevel-polymorphic-chain.md).
RVA(0x00102000, 0x7f)
CSBI_ImageSet::~CSBI_ImageSet() {
    DtorImageSet();
}

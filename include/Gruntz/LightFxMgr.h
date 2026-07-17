// LightFxMgr.h - the light-FX / translucency shade-table manager (tracer
// placeholder tomalla-3). A heap object hung off the game registry at
// g_gameReg->m_78 (created in the registry Init at 0x83450, torn down in the
// registry close at 0x855e0). It owns the engine's pre-built RGB565 color /
// translucency lookup tables: an identity "grey" remap (CShadeTableCache::GreyTable),
// one additive glow table (AddTable), and eight subtractive color tables
// (SubTable, one per fixed effect color). Each Push() applies a chosen table to
// an image-set's frames (CImageSet::SetAllTypes / SetAllFormats).
//
// sizeof 0x3c (operator new(0x3c)). Field names are placeholders (m_<hexoffset>);
// only the OFFSETS + code bytes are load-bearing. The 10 shade-table slots live
// at +0x14..+0x3b as a CShadeTable*[10] array (Push indexes it by the clamped
// anchor key; Reset zeroes it). The CShadeTableCache builders + the global table
// registrar (0x14dcf0) + CImageSet accessors are external/reloc-masked.
#ifndef GRUNTZ_GRUNTZ_LIGHTFXMGR_H
#define GRUNTZ_GRUNTZ_LIGHTFXMGR_H

#include <rva.h>

#include <Ints.h>

class CShadeTableCache;
struct CShadeTable;
struct CGameRegistry; // canonical is `struct` (<Gruntz/GameRegistry.h>); keyword must match for Init's PAU mangling
class CDDrawSurfaceMgr; // reg->m_world (+0x30) - the loaded world/resource holder
class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and
                                // keeps this header pointer-only/include-light.

class CLightFxMgr {
public:
    // 0x9dad0  Init - bind to the game registry, fetch the shade-table cache, and
    // build the identity + additive + 8 subtractive color tables, registering the
    // grey table globally (key 9). __thiscall, 2 args (reg, owner), ret 0x8.
    i32 Init(CGameRegistry* reg, void* owner);
    // 0x9dc80  Reset - zero the bound pointers (+0x4..+0x10) and the 10 table
    // slots. __thiscall, no args, ret 0.
    void Reset();
    // 0x9dcb0  Push - apply the shade table chosen by `anchor` (clamped to [0,10))
    // to every frame of `imgSet`: re-type the frames (slot) then write the table's
    // resolved format word. __thiscall, 3 args, ret 0xc.
    i32 Push(CImageSet* imgSet, i32 anchor, i32 slot);

    void* m_owner;             // +0x00  owner (Init arg1)
    CGameRegistry* m_reg;      // +0x04  the game registry (Init arg0)
    CDDrawSurfaceMgr* m_world; // +0x08  reg->m_world (+0x30 loaded world/resource holder;
                               //        ex `void* m_spriteFactory` - misnamed)
    CShadeTableCache* m_cache; // +0x0c  reg->m_shadeCache (+0x50)
    CShadeTable* m_greyTable;  // +0x10  the identity "grey" table (registered key 9)
    CShadeTable* m_tables[10]; // +0x14  the 10 color tables (1 add + 9 sub)
};
SIZE(CLightFxMgr, 0x3c);

#endif // GRUNTZ_GRUNTZ_LIGHTFXMGR_H

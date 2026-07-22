#ifndef GRUNTZ_GRUNTZ_LIGHTFXMGR_H
#define GRUNTZ_GRUNTZ_LIGHTFXMGR_H

#include <rva.h>

#include <Ints.h>

class CShadeTableCache;
struct CShadeTable;
struct CGameRegistry; // canonical is `struct` (<Gruntz/GameRegistry.h>); keyword must match for Init's PAU mangling
class CDDrawSurfaceMgr; // reg->m_world (+0x30) - the loaded world/resource holder
class CDDrawWorker;             // CDDrawWorker IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);

class CLightFxMgr {
public:
    // 0x9dad0  Init - bind to the game registry, fetch the shade-table cache, and
    // build the identity + additive + 8 subtractive color tables, registering the
    // grey table globally (key 9). __thiscall, 2 args (reg, owner), ret 0x8.
    i32 Init(class CGruntzMgr* reg, class CGruntzMgr* owner); // both args ARE the one manager (the ex dual-view pair)
    // 0x9dc80  Reset - zero the bound pointers (+0x4..+0x10) and the 10 table
    // slots. __thiscall, no args, ret 0.
    void Reset();
    // 0x9dcb0  Push - apply the shade table chosen by `anchor` (clamped to [0,10))
    // to every frame of `imgSet`: re-type the frames (slot) then write the table's
    // resolved format word. __thiscall, 3 args, ret 0xc.
    i32 Push(CDDrawWorker* imgSet, i32 anchor, i32 slot);

    class CGruntzMgr* m_owner; // +0x00  owner (Init arg2 = the manager)
    class CGruntzMgr* m_reg;   // +0x04  the manager (Init arg0)
    CDDrawSurfaceMgr* m_world; // +0x08  reg->m_world (+0x30 loaded world/resource holder;
                               //        ex `void* m_spriteFactory` - misnamed)
    CShadeTableCache* m_cache; // +0x0c  reg->m_shadeCache (+0x50)
    CShadeTable* m_greyTable;  // +0x10  the identity "grey" table (registered key 9)
    CShadeTable* m_tables[10]; // +0x14  the 10 color tables (1 add + 9 sub)
};
SIZE(0x3c);

#endif // GRUNTZ_GRUNTZ_LIGHTFXMGR_H

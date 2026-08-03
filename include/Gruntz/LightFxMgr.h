#ifndef GRUNTZ_GRUNTZ_LIGHTFXMGR_H
#define GRUNTZ_GRUNTZ_LIGHTFXMGR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_FORWARD(ShadeMode);

struct CShadeTable;

class CShadeTableCache;
struct CShadeTable;
struct CGameRegistry;
class CDDrawSurfaceMgr;
class CDDrawWorker;

class CLightFxMgr {
public:
    i32 Init(class CGruntzMgr* reg, class CGruntzMgr* owner);

    void Reset();

    i32 Push(CDDrawWorker* imgSet, i32 anchor, ShadeMode slot);

    class CGruntzMgr* m_owner;
    class CGruntzMgr* m_reg;
    CDDrawSurfaceMgr* m_world;

    CShadeTableCache* m_cache;
    CShadeTable* m_greyTable;
    CShadeTable* m_tables[10];
};
SIZE(0x3c);

void SetShadeDescr(CShadeTable* v, ShadeMode mode);

#endif // GRUNTZ_GRUNTZ_LIGHTFXMGR_H

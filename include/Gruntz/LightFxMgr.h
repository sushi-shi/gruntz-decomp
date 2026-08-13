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
    CLightFxMgr();

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

inline CLightFxMgr::CLightFxMgr() {
    m_reg = 0;
    m_world = 0;
    m_cache = 0;
    m_greyTable = 0;
    for (i32 i = 0; i < 10; ++i) {
        m_tables[i] = 0;
    }
}

void SetShadeDescr(CShadeTable* v, ShadeMode mode);

#endif // GRUNTZ_GRUNTZ_LIGHTFXMGR_H

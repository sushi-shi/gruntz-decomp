#ifndef GRUNTZ_GRUNTZ_LIGHTFXMGR_H
#define GRUNTZ_GRUNTZ_LIGHTFXMGR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

#include <stddef.h>

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

    i32 Init(class CGruntzMgr* gameMgr, class CGruntzMgr* owner);

    void Reset();

    i32 ApplyShadeTable(CDDrawWorker* imageSet, i32 tableIndex, ShadeMode mode);

    class CGruntzMgr* m_owner;
    class CGruntzMgr* m_gameMgr;
    CDDrawSurfaceMgr* m_world;

    CShadeTableCache* m_cache;
    CShadeTable* m_greyTable;
    CShadeTable* m_tables[10];
};

inline CLightFxMgr::CLightFxMgr() {
    m_gameMgr = NULL;
    m_world = NULL;
    m_cache = NULL;
    m_greyTable = NULL;
    for (i32 i = 0; i < 10; ++i) {
        m_tables[i] = NULL;
    }
}

void SetShadeDescr(CShadeTable* v, ShadeMode mode);

#endif // GRUNTZ_GRUNTZ_LIGHTFXMGR_H

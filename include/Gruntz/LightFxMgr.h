#ifndef GRUNTZ_GRUNTZ_LIGHTFXMGR_H
#define GRUNTZ_GRUNTZ_LIGHTFXMGR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

#include <stddef.h>
#include <string.h>

GZ_ENUM_FORWARD(ShadeMode);

struct CShadeTable;

class CShadeTableCache;
struct CShadeTable;
struct CGameRegistry;
class CGameWorld;
class CImageSet;

class CLightFxMgr {
public:
    CLightFxMgr();
    ~CLightFxMgr();

    i32 Init(class CGruntzMgr* gameMgr, class CGruntzMgr* owner);

    void Reset();

    i32 ApplyShadeTable(CImageSet* imageSet, i32 tableIndex, ShadeMode mode);

    CShadeTable* GetShadeTable(i32 tableIndex) const {
        return m_tables[tableIndex];
    }

    class CGruntzMgr* m_owner;
    class CGruntzMgr* m_gameMgr;
    CGameWorld* m_world;

    CShadeTableCache* m_cache;
    CShadeTable* m_greyTable;
    CShadeTable* m_tables[10];
};

inline CLightFxMgr::CLightFxMgr() {
    m_gameMgr = NULL;
    m_world = NULL;
    m_cache = NULL;
    m_greyTable = NULL;
    memset(m_tables, 0, sizeof(m_tables));
}

inline CLightFxMgr::~CLightFxMgr() {
    Reset();
}

void SetShadeDescr(CShadeTable* v, ShadeMode mode);

#endif // GRUNTZ_GRUNTZ_LIGHTFXMGR_H

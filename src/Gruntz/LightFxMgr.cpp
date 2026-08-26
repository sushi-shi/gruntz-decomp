#include <rva.h>

#include <Gruntz/LightFxMgr.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Image/ImageSet.h>

#include <string.h>

struct CShadeTable;

RVA(0x0009d9f0, 0x14a)
i32 CLightFxMgr::Init(CGruntzMgr* gameMgr, CGruntzMgr* owner) {
    if (!gameMgr) {
        return 0;
    }
    m_gameMgr = gameMgr;
    m_owner = owner;
    m_world = gameMgr->m_world;
    m_cache = gameMgr->m_shadeCache;

    if (!m_cache) {
        return 0;
    }
    m_greyTable = m_cache->GreyTable();
    if (!m_greyTable) {
        return 0;
    }
    m_tables[0] = m_cache->AddTable(2.0f);
    if (!m_tables[0]) {
        return 0;
    }
    m_tables[1] = m_cache->SubTable(0xff);
    if (!m_tables[1]) {
        return 0;
    }
    m_tables[2] = m_cache->SubTable(0xff00);
    if (!m_tables[1]) {
        return 0;
    }
    m_tables[3] = m_cache->SubTable(0xff0000);
    if (!m_tables[3]) {
        return 0;
    }
    m_tables[4] = m_cache->SubTable(0xffff);
    if (!m_tables[4]) {
        return 0;
    }
    m_tables[5] = m_cache->SubTable(0x202020);
    if (!m_tables[5]) {
        return 0;
    }
    m_tables[6] = m_cache->SubTable(0xff8080);
    if (!m_tables[6]) {
        return 0;
    }
    m_tables[7] = m_cache->SubTable(0xc000c0);
    if (!m_tables[7]) {
        return 0;
    }
    m_tables[8] = m_cache->SubTable(0x60c0);
    if (!m_tables[8]) {
        return 0;
    }
    m_tables[9] = m_cache->SubTable(0xc0c0c0);
    if (!m_tables[9]) {
        return 0;
    }
    SetShadeDescr(m_greyTable, SHADE_GREY_TABLE);
    return 1;
}

RVA(0x0009dba0, 0x1d)
void CLightFxMgr::Reset() {
    m_gameMgr = NULL;
    m_world = NULL;
    m_cache = NULL;
    m_greyTable = NULL;
    memset(m_tables, 0, sizeof(m_tables));
}

RVA(0x0009dbd0, 0x41)
i32 CLightFxMgr::ApplyShadeTable(CDDrawWorker* imageSet, i32 tableIndex, ShadeMode mode) {
    if (!imageSet) {
        return 0;
    }
    if (tableIndex < 0 || tableIndex >= 10) {
        tableIndex = 0;
    }
    CShadeTable* table = m_tables[tableIndex];
    imageSet->SetAllTypes(mode);

    imageSet->SetAllFormats(table);
    return 1;
}

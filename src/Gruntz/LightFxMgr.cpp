#include <Gruntz/GruntzMgr.h> // the one manager type
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/GameRegistry.h> // the singleton Init binds (m_world/m_shadeCache)

#include <DDrawMgr/ShadeTableCache.h>
#include <Image/ImageSet.h>
#include <rva.h>

#include <string.h> // memset -> inline rep stosd in Reset

struct ShadeDescr;
void SetShadeDescr(ShadeDescr* v, int mode); // 0x14dcf0

RVA(0x0009dad0, 0x14a)
i32 CLightFxMgr::Init(CGruntzMgr* reg, CGruntzMgr* owner) {
    if (!reg) {
        return 0;
    }
    m_reg = reg;
    m_owner = owner;
    m_world = reg->m_world;
    m_cache = reg->m_shadeCache;
    // Re-read m_cache per builder call (don't cache it in a local) so the compiler
    // reloads [this+0xc] before each call rather than pinning the cache in a
    // callee-saved reg + an extra push. See reread-member-view-pointer.md.
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
    SetShadeDescr(reinterpret_cast<ShadeDescr*>(m_greyTable), 9);
    return 1;
}

RVA(0x0009dc80, 0x1d)
void CLightFxMgr::Reset() {
    m_reg = 0;
    m_world = 0;
    m_cache = 0;
    m_greyTable = 0;
    memset(m_tables, 0, sizeof(m_tables));
}

RVA(0x0009dcb0, 0x41)
i32 CLightFxMgr::Push(CImageSet* imgSet, i32 anchor, i32 slot) {
    if (!imgSet) {
        return 0;
    }
    if (anchor < 0 || anchor >= 10) {
        anchor = 0;
    }
    CShadeTable* table = m_tables[anchor];
    imgSet->SetAllTypes(slot);
    // The engine hands the resolved shade table straight to SetAllFormats as the
    // frames' format word (a pointer-as-value the frame stores verbatim at +0x1c).
    imgSet->SetAllFormats(reinterpret_cast<i32>(table));
    return 1;
}

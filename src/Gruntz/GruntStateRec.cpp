#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/Sprite.h>
#include <string.h>

// @early-stop
RVA(0x000ea990, 0xa72)
i32 CSBI_StatzTabGruntBar::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;
    i32 v;

    switch (mode) {
        case 4:

#define GS_SUBREC(field)                                                                           \
    g_serialCounter++;                                                                             \
    memset(buf, 0, sizeof(buf));                                                                   \
    v = 0;                                                                                         \
    if (field != 0) {                                                                              \
        reg->m_imageRegistry->AnyValueMatches(field, buf, &v);                                     \
    }                                                                                              \
    s->Write(buf, 0x80);                                                                           \
    s->Write(&v, 4)

            GS_SUBREC(m_statusGlyph);
            GS_SUBREC(m_statusGlyphLatched);
            s->Write(&m_statusValue, 4);

            GS_SUBREC(m_abilityGlyph);
            GS_SUBREC(m_abilityGlyphLatched);
            s->Write(&m_abilityValue, 4);
            GS_SUBREC(m_overrideGlyph);
            GS_SUBREC(m_overrideGlyphLatched);
            s->Write(&m_overrideValue, 4);
            GS_SUBREC(m_selectKey);
            GS_SUBREC(m_selectGlyph);
            s->Write(&m_selectValue, 4);
            GS_SUBREC(m_timerGlyph);
            s->Write(&m_timerValue, 4);
            s->Write(&m_unitRow, 4);
            s->Write(&m_unitCol, 4);
#undef GS_SUBREC

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_glyphMap != 0) {
                strcpy(buf, m_glyphMap->m_name);
            }
            s->Write(buf, 0x80);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_timerGlyphMap != 0) {
                strcpy(buf, m_timerGlyphMap->m_name);
            }
            s->Write(buf, 0x80);
            break;

        case 7:

#define GS_IDXREF(field)                                                                           \
    g_serialCounter++;                                                                             \
    s->Read(buf, 0x80);                                                                            \
    s->Read(&idx, 4);                                                                              \
    if (strlen(buf) != 0) {                                                                        \
        i32 i = idx;                                                                               \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_10map.Lookup(buf, out);                                            \
        CDDrawWorker* gm = static_cast<CDDrawWorker*>(out);                                        \
        CImage* r;                                                                                 \
        if (gm != 0 && i >= gm->m_minIndex && i <= gm->m_maxIndex) {                               \
            r = static_cast<CImage*>(gm->m_items.GetAt(i));                                        \
        } else {                                                                                   \
            r = 0;                                                                                 \
        }                                                                                          \
        field = r;                                                                                 \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }
#define GS_NAMEREF(field)                                                                          \
    g_serialCounter++;                                                                             \
    s->Read(buf, 0x80);                                                                            \
    if (strlen(buf) != 0) {                                                                        \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_10map.Lookup(buf, out);                                            \
        field = static_cast<CDDrawWorker*>(out);                                                   \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

            GS_IDXREF(m_statusGlyph);
            GS_IDXREF(m_statusGlyphLatched);
            s->Read(&m_statusValue, 4);
            GS_IDXREF(m_abilityGlyphLatched);
            s->Read(&m_abilityValue, 4);
            GS_IDXREF(m_overrideGlyph);
            GS_IDXREF(m_overrideGlyphLatched);
            s->Read(&m_overrideValue, 4);
            GS_IDXREF(m_selectKey);
            GS_IDXREF(m_selectGlyph);
            s->Read(&m_selectValue, 4);
            GS_IDXREF(m_timerGlyph);
            s->Read(&m_timerValue, 4);
            s->Read(&m_unitRow, 4);
            s->Read(&m_unitCol, 4);
            GS_NAMEREF(m_glyphMap);
            GS_NAMEREF(m_timerGlyphMap);
#undef GS_IDXREF
#undef GS_NAMEREF
            break;
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, pObj) != 0 ? 1 : 0;
}

#include <rva.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Io/FileMem.h>

#include <string.h>

RVA(0x000ea990, 0xa72)
i32 CSBI_StatzTabGruntBar::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];

    switch (mode) {
        case SERIAL_SAVE: {
            i32 v;

#define GS_SUBREC(field)                                                                           \
    g_serialCounter++;                                                                             \
    memset(buf, 0, sizeof(buf));                                                                   \
    v = 0;                                                                                         \
    if (field != 0) {                                                                              \
        reg->m_imageRegistry->AnyValueMatches(field, buf, &v);                                     \
    }                                                                                              \
    s->Write(buf, SERIAL_NAME_LEN);                                                                \
    s->Write(&v, 4)

            GS_SUBREC(m_statusGlyph);
            GS_SUBREC(m_statusGlyphLatched);
            s->Write(&m_statusValue, sizeof(m_statusValue));

            GS_SUBREC(m_abilityGlyph);
            GS_SUBREC(m_abilityGlyphLatched);
            s->Write(&m_abilityValue, sizeof(m_abilityValue));
            GS_SUBREC(m_overrideGlyph);
            GS_SUBREC(m_overrideGlyphLatched);
            s->Write(&m_overrideValue, sizeof(m_overrideValue));
            GS_SUBREC(m_selectKey);
            GS_SUBREC(m_selectGlyph);
            s->Write(&m_selectValue, sizeof(m_selectValue));
            GS_SUBREC(m_timerGlyph);
            s->Write(&m_timerValue, sizeof(m_timerValue));
            s->Write(&m_playerIndex, sizeof(m_playerIndex));
            s->Write(&m_unitIndex, sizeof(m_unitIndex));
#undef GS_SUBREC

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_glyphMap != NULL) {
                strcpy(buf, m_glyphMap->m_name);
            }
            s->Write(buf, SERIAL_NAME_LEN);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_timerGlyphMap != NULL) {
                strcpy(buf, m_timerGlyphMap->m_name);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;
            i32 idx;

#define GS_IDXREF(field)                                                                           \
    g_serialCounter++;                                                                             \
    s->Read(buf, SERIAL_NAME_LEN);                                                                 \
    s->Read(&idx, 4);                                                                              \
    if (strlen(buf) != 0) {                                                                        \
        i32 i = idx;                                                                               \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_workersByName.Lookup(buf, out);                                    \
        CDDrawWorker* gm = static_cast<CDDrawWorker*>(out);                                        \
        CImage* r = gm != 0 ? gm->GetAt(i) : 0;                                                    \
        field = r;                                                                                 \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }
#define GS_NAMEREF(field)                                                                          \
    g_serialCounter++;                                                                             \
    s->Read(buf, SERIAL_NAME_LEN);                                                                 \
    if (strlen(buf) != 0) {                                                                        \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_workersByName.Lookup(buf, out);                                    \
        field = static_cast<CDDrawWorker*>(out);                                                   \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

            GS_IDXREF(m_statusGlyph);
            GS_IDXREF(m_statusGlyphLatched);
            s->Read(&m_statusValue, sizeof(m_statusValue));
            GS_IDXREF(m_abilityGlyph);
            GS_IDXREF(m_abilityGlyphLatched);
            s->Read(&m_abilityValue, sizeof(m_abilityValue));
            GS_IDXREF(m_overrideGlyph);
            GS_IDXREF(m_overrideGlyphLatched);
            s->Read(&m_overrideValue, sizeof(m_overrideValue));
            GS_IDXREF(m_selectKey);
            GS_IDXREF(m_selectGlyph);
            s->Read(&m_selectValue, sizeof(m_selectValue));
            GS_IDXREF(m_timerGlyph);
            s->Read(&m_timerValue, sizeof(m_timerValue));
            s->Read(&m_playerIndex, sizeof(m_playerIndex));
            s->Read(&m_unitIndex, sizeof(m_unitIndex));
            GS_NAMEREF(m_glyphMap);
            GS_NAMEREF(m_timerGlyphMap);
#undef GS_IDXREF
#undef GS_NAMEREF
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, payload) != 0 ? 1 : 0;
}

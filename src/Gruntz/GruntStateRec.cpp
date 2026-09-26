#include <rva.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialWorkerRefMacros.h>
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

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_statusGlyph);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_statusGlyphLatched);
            s->Write(&m_statusValue, sizeof(m_statusValue));

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_abilityGlyph);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_abilityGlyphLatched);
            s->Write(&m_abilityValue, sizeof(m_abilityValue));
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_overrideGlyph);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_overrideGlyphLatched);
            s->Write(&m_overrideValue, sizeof(m_overrideValue));
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_selectKey);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_selectGlyph);
            s->Write(&m_selectValue, sizeof(m_selectValue));
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_timerGlyph);
            s->Write(&m_timerValue, sizeof(m_timerValue));
            s->Write(&m_playerIndex, sizeof(m_playerIndex));
            s->Write(&m_unitIndex, sizeof(m_unitIndex));

            SERIAL_WRITE_WORKER(s, buf, m_glyphMap);

            SERIAL_WRITE_WORKER(s, buf, m_timerGlyphMap);
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;
            i32 idx;

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
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, payload) != 0 ? 1 : 0;
}

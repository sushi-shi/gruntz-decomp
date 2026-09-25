#ifndef GRUNTZ_ANIADVANCECURSORINLINE_H
#define GRUNTZ_ANIADVANCECURSORINLINE_H

#include <DDrawMgr/AniRecord.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElementInline.h>

inline i32 CAniAdvanceCursor::IsComplete() const {
    return m_finished != false && m_frameTicksLeft == 0;
}

inline void CAniAdvanceCursor::AdvanceToNextRecord() {
    CAniElement* animation = m_animation;
    m_index = m_index + 1;
    CAniRecordView* record = animation->RecordAt(m_index);
    m_element = record;
    if (record == NULL) {
        m_index = 0;
        m_element = static_cast<CAniRecordView*>(animation->AtChecked(0));
    }
    if (m_element != NULL) {
        m_curDraw = m_pendingDraw;
        m_pendingDraw = m_element->m_drawValue;
    }
}

#endif // GRUNTZ_ANIADVANCECURSORINLINE_H

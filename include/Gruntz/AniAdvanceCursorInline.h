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
    m_recordIndex = m_recordIndex + 1;
    CAniRecordView* record = animation->RecordAt(m_recordIndex);
    m_currentRecord = record;
    if (record == NULL) {
        m_recordIndex = 0;
        m_currentRecord = static_cast<CAniRecordView*>(animation->AtChecked(0));
    }
    if (m_currentRecord != NULL) {
        m_curDraw = m_pendingDraw;
        m_pendingDraw = m_currentRecord->m_drawValue;
    }
}

#endif // GRUNTZ_ANIADVANCECURSORINLINE_H

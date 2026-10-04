#ifndef GRUNTZ_ANIADVANCECURSORINLINE_H
#define GRUNTZ_ANIADVANCECURSORINLINE_H

#include <DDrawMgr/AniRecord.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElementInline.h>

inline i32 CAniAdvanceCursor::IsComplete() const {
    return m_finished != false && m_frameTicksLeft == 0;
}

inline void CAniAdvanceCursor::AdvanceToNextRecord() {
    CAnimationSequence* animation = m_animation;
    m_recordIndex = m_recordIndex + 1;
    CAniFrameRecord* record = animation->RecordAt(m_recordIndex);
    m_currentRecord = record;
    if (record == NULL) {
        m_recordIndex = 0;
        m_currentRecord = static_cast<CAniFrameRecord*>(animation->AtChecked(0));
    }
    if (m_currentRecord != NULL) {
        m_currentEventCode = m_pendingEventCode;
        m_pendingEventCode = m_currentRecord->m_eventCode;
    }
}

#endif // GRUNTZ_ANIADVANCECURSORINLINE_H

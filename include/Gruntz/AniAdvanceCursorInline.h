#ifndef GRUNTZ_ANIADVANCECURSORINLINE_H
#define GRUNTZ_ANIADVANCECURSORINLINE_H

#include <DDrawMgr/AniRecord.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElementInline.h>

inline i32 IsAniCursorComplete(const CAniAdvanceCursor* cursor) {
    return cursor->m_finished != false && cursor->m_frameTicksLeft == 0;
}

static inline void AdvanceToNextRecord(CAniAdvanceCursor* cursor) {
    CAniElement* animation = cursor->m_animation;
    cursor->m_index = cursor->m_index + 1;
    CAniRecordView* record =
        static_cast<CAniRecordView*>(GetAniElementAt(animation, cursor->m_index));
    cursor->m_element = record;
    if (record == NULL) {
        cursor->m_index = 0;
        cursor->m_element = static_cast<CAniRecordView*>(animation->AtChecked(0));
    }
    if (cursor->m_element != NULL) {
        cursor->m_curDraw = cursor->m_pendingDraw;
        cursor->m_pendingDraw = cursor->m_element->m_drawValue;
    }
}

#endif // GRUNTZ_ANIADVANCECURSORINLINE_H

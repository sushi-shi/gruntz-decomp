#ifndef GRUNTZ_ANIELEMENTINLINE_H
#define GRUNTZ_ANIELEMENTINLINE_H

#include <DDrawMgr/AniRecord.h>
#include <Gruntz/AniElement.h>

#include <stddef.h>

inline CObject* GetAniElementAt(const CAniElement* animation, i32 i) {
    if (i >= 0 && i < animation->m_records.GetSize()) {
        return animation->m_records.GetAt(i);
    }
    return NULL;
}

static inline CAniRecordView* RecordAt(CAniElement* anim, i32 index) {
    return static_cast<CAniRecordView*>(GetAniElementAt(anim, index));
}

#endif // GRUNTZ_ANIELEMENTINLINE_H

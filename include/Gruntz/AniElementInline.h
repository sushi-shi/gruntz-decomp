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
    CAniRecordView* rec;
    if (index >= 0 && index < anim->m_records.GetSize()) {
        rec = static_cast<CAniRecordView*>(anim->m_records.GetAt(index));
    } else {
        rec = NULL;
    }
    return rec;
}

#endif // GRUNTZ_ANIELEMENTINLINE_H

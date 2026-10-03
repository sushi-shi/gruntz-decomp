#ifndef GRUNTZ_ANIELEMENTINLINE_H
#define GRUNTZ_ANIELEMENTINLINE_H

#include <DDrawMgr/AniRecord.h>
#include <Gruntz/AniElement.h>

#include <stddef.h>

inline CObject* CAniElement::GetAt(i32 i) const {
    if (i >= 0 && i < static_cast<i32>(m_records.size())) {
        return m_records[i];
    }
    return NULL;
}

inline CAniRecordView* CAniElement::RecordAt(i32 index) const {
    return static_cast<CAniRecordView*>(GetAt(index));
}

#endif

#ifndef GRUNTZ_ANIELEMENTINLINE_H
#define GRUNTZ_ANIELEMENTINLINE_H

#include <DDrawMgr/AniRecord.h>
#include <Gruntz/AniElement.h>

#include <stddef.h>

inline CObject* CAniElement::GetAt(i32 i) const {
    if (i >= 0 && i < m_records.GetSize()) {
        return m_records.GetAt(i);
    }
    return NULL;
}

inline CAniFrameRecord* CAniElement::RecordAt(i32 index) const {
    return static_cast<CAniFrameRecord*>(GetAt(index));
}

#endif // GRUNTZ_ANIELEMENTINLINE_H

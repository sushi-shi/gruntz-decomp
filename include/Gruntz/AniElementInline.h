#ifndef GRUNTZ_ANIELEMENTINLINE_H
#define GRUNTZ_ANIELEMENTINLINE_H

#include <Gruntz/AniElement.h>

inline CObject* GetAniElementAt(const CAniElement* animation, i32 i) {
    if (i >= 0 && i < animation->m_records.GetSize()) {
        return animation->m_records.GetAt(i);
    }
    return 0;
}

#endif // GRUNTZ_ANIELEMENTINLINE_H

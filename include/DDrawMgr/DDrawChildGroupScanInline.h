#ifndef GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H
#define GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <Wwd/WwdGameObjectFamily.h>

inline CGameObject* CDDrawChildGroup::Drain() {
    if (m_scanCursor == m_list.end()) {
        return NULL;
    }
    CGameObject* data = NextChild(m_scanCursor);
    if (data->GetClassId() == CLASSID_SERIALREF) {
        return data;
    }
    return Drain();
}

inline CGameObject* CDDrawChildGroup::FirstSerialChild() {
    m_scanCursor = m_list.begin();
    return Drain();
}

#endif

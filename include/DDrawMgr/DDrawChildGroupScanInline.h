#ifndef GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H
#define GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <Wwd/WwdGameObjectFamily.h>

RVA(0x00031250, 0x33)
inline CGameObject* CDDrawChildGroup::Drain() {
    if (m_scanCursor == NULL) {
        return NULL;
    }
    CGameObject* data = NextChild(m_scanCursor);
    if (data->GetClassId() == CLASSID_SERIALREF) {
        return data;
    }
    return Drain();
}

inline CGameObject* CDDrawChildGroup::FirstSerialChild() {
    m_scanCursor = m_list.GetHeadPosition();
    return Drain();
}

#endif // GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

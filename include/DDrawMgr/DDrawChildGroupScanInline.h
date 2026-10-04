#ifndef GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H
#define GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <Wwd/WwdGameObjectFamily.h>

RVA(0x00031250, 0x33)
inline CGameObject* CDDrawChildGroup::NextSerialChild() {
    if (m_serialScanCursor == NULL) {
        return NULL;
    }
    CGameObject* object = NextChild(m_serialScanCursor);
    if (object->GetClassId() == CLASSID_SERIALREF) {
        return object;
    }
    return NextSerialChild();
}

inline CGameObject* CDDrawChildGroup::FirstSerialChild() {
    m_serialScanCursor = m_list.GetHeadPosition();
    return NextSerialChild();
}

#endif // GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

#ifndef GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H
#define GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <Wwd/WwdGameObjectFamily.h>

RVA(0x00031250, 0x33)
inline CGameObject* CDDrawChildGroup::NextSpriteChild() {
    if (m_spriteScanCursor == NULL) {
        return NULL;
    }
    CGameObject* object = NextChild(m_spriteScanCursor);
    if (object->GetClassId() == CLASSID_WWD_SPRITE_OBJECT) {
        return object;
    }
    return NextSpriteChild();
}

inline CGameObject* CDDrawChildGroup::FirstSpriteChild() {
    m_spriteScanCursor = m_list.GetHeadPosition();
    return NextSpriteChild();
}

#endif // GRUNTZ_DDRAWCHILDGROUPSCANINLINE_H

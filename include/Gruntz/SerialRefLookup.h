#ifndef GRUNTZ_SERIALREFLOOKUP_H
#define GRUNTZ_SERIALREFLOOKUP_H

#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

inline CWwdSpriteObject* LookupSpriteObjectById(CMapPtrToPtr& byId, i32 objectId) {
    CGameObject* found = NULL;
    if (MapLookupById(byId, objectId, found) == false) {
        return NULL;
    }
    if (found == NULL) {
        return NULL;
    }
    return found->GetClassId() == CLASSID_WWD_SPRITE_OBJECT ? static_cast<CWwdSpriteObject*>(found)
                                                            : NULL;
}

#endif // GRUNTZ_SERIALREFLOOKUP_H

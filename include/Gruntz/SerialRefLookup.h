#ifndef GRUNTZ_SERIALREFLOOKUP_H
#define GRUNTZ_SERIALREFLOOKUP_H

#include <map>
#include <string>

#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

inline CWwdSpriteObject* LookupSerialRef(std::map<i32, CGameObject*>& byId, i32 id) {
    CGameObject* found = NULL;
    if (MapLookupById(byId, id, found) == false) {
        return NULL;
    }
    if (found == NULL) {
        return NULL;
    }
    return found->GetClassId() == CLASSID_SERIALREF ? static_cast<CWwdSpriteObject*>(found) : NULL;
}

#endif

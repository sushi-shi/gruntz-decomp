#ifndef GRUNTZ_WWD_WWDOBJMGRINLINE_H
#define GRUNTZ_WWD_WWDOBJMGRINLINE_H

#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdObjMgr.h>

inline void* WwdKey(CGameObject* o) {
    // API-forced: CMapPtrToPtr keys an integer id through its void* key.
    return reinterpret_cast<void*>(o->m_objectId);
}

static inline i32 WorldSpaceDifference(i32 leftFlags, i32 rightFlags) {
    return (leftFlags ^ rightFlags) & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE);
}

static inline i32 ObjectTypeBits(u32 objectType, i32 mask) {
    return static_cast<i32>(objectType) & mask;
}

static inline void DrawObjectDebugRect(
    CWwdGameObject* obj,
    const RECT& objectRect,
    CDDrawWorkerHost* view,
    CDDrawSurfacePair* drawHost
) {
    i32 ox = obj->m_screenX;
    RECT rc;
    rc.left = objectRect.left + ox;
    i32 oy = obj->m_screenY;
    rc.top = objectRect.top + oy;
    rc.right = objectRect.right + ox;
    rc.bottom = objectRect.bottom + oy;
    view->WorldToViewport(&rc.left, &rc.top);
    view->WorldToViewport(&rc.right, &rc.bottom);
    drawHost->DrawBox(&rc, 0xff);
}

static inline CWwdGameObject* LookupObjectById(CMapPtrToPtr& byId, i32 id) {
    CWwdGameObject* found = NULL;
    if (MapLookupById(byId, id, found) == false) {
        found = NULL;
    }
    return found;
}

inline CWwdGameObject* LookupActiveObject(CMapPtrToPtr& map, void* key) {
    CWwdGameObject* found = NULL;
    if (MapLookup(map, key, found) == false) {
        found = NULL;
    }
    return found;
}

#endif // GRUNTZ_WWD_WWDOBJMGRINLINE_H

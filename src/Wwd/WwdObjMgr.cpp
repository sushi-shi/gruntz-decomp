#include <StdAfx.h>

#include <Ints.h>

#include <Wwd/WwdObjMgr.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDSurfaceCaps.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ObList.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdFactoryObject.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdObjMgrInline.h>
#include <Wwd/WwdObjMgrMacros.h>

#include <new>

i32 g_wwdObjIdCounter = 1;

b32 g_soundEnabled = true;

i32 g_soundVolumePercent = 100;

void CDDrawChildGroup::Unload() {
    this->DestroyChildren();
}

void CDDrawChildGroup::DestroyChildren() {
    CGameLevel* p = OwnerMgr()->m_level;
    if (p != NULL) {

        CDDrawWorkerHost* q = static_cast<CDDrawWorkerHost*>(p->m_mainPlane);
        if (q != NULL) {
            q->Prune();
        }
    }
    std::list<CGameObject*>::iterator n = m_list.begin();
    while (n != m_list.end()) {
        CGameObject* cur_obj = NextChild(n);
        CGameObject* obj = cur_obj;
        if (obj != NULL) {
            delete obj;
        }
    }
    m_list.clear();
    m_walkCursor = m_list.end();
    m_scanCursor = m_list.end();
    m_activeGameObjectsById.clear();
    m_registeredGameObjectsById.clear();
}

CWwdDotObject* CDDrawChildGroup::CreateDotObject(
    int id,
    int x,
    int y,
    int sortKey,
    CLogicRecord* logicTemplate,
    int dotColor,
    int objectFlags
) {
    CWwdDotObject* result = new CWwdDotObject(OwnerMgr(), id, objectFlags);
    if (result->SetupDot(x, y, sortKey, logicTemplate, dotColor) == 0) {
        if (result != NULL) {
            delete result;
        }
        return NULL;
    }
    InsertSorted(result, 1);
    if (HAS(static_cast<WwdGameObjectFlags>(objectFlags),
            WWD_GAME_OBJECT_FLAG_DISPATCH_ON_CREATE)) {

        result->m_logicRecord->m_dispatch(result);
    }
    return result;
}

CWwdDotObject* CDDrawChildGroup::CreateNamedDotObject(
    int id,
    int x,
    int y,
    int sortKey,
    const std::string& name,
    int dotColor,
    int objectFlags
) {
    return CreateDotObject(
        id,
        x,
        y,
        sortKey,
        OwnerMgr()->m_logicRegistry->FindTemplate(name),
        dotColor,
        objectFlags
    );
}

CWwdDeferredObject* CDDrawChildGroup::CreateDeferredObject(
    int id,
    int sortKey,
    CLogicRecord* logicTemplate,
    int objectFlags
) {
    CWwdDeferredObject* result = new CWwdDeferredObject(OwnerMgr(), id, objectFlags);
    if (result->SetupDeferred(sortKey, logicTemplate) == 0) {
        if (result != NULL) {
            delete result;
        }
        return NULL;
    }
    InsertSorted(result, 1);
    if (HAS(static_cast<WwdGameObjectFlags>(objectFlags),
            WWD_GAME_OBJECT_FLAG_DISPATCH_ON_CREATE)) {
        result->m_logicRecord->m_dispatch(result);
    }
    return result;
}

CWwdDeferredObject* CDDrawChildGroup::CreateNamedDeferredObject(
    int id,
    int sortKey,
    const std::string& name,
    int objectFlags
) {
    return CreateDeferredObject(
        id,
        sortKey,
        OwnerMgr()->m_logicRegistry->FindTemplate(name),
        objectFlags
    );
}

CWwdSpriteObject* CDDrawChildGroup::CreateSpriteObject(
    i32 id,
    i32 x,
    i32 y,
    i32 sortKey,
    CLogicRecord* logicTemplate,
    i32 objectFlags
) {
    CWwdSpriteObject* result =
        new CWwdSpriteObject(OwnerMgr(), id, objectFlags, CGameObject::INLINE_BASE);
    if (result->Setup(x, y, sortKey, logicTemplate) == 0) {
        if (result != NULL) {
            delete result;
        }
        return NULL;
    }
    InsertSorted(result, 1);
    if (HAS(static_cast<WwdGameObjectFlags>(objectFlags),
            WWD_GAME_OBJECT_FLAG_DISPATCH_ON_CREATE)) {
        result->m_logicRecord->m_dispatch(result);
    }
    return result;
}

CWwdSpriteObject* CDDrawChildGroup::CreateSprite(
    i32 id,
    i32 x,
    i32 y,
    i32 sortKey,
    const std::string& name,
    i32 objectFlags
) {
    CLogicRecord* logicTemplate =
        OwnerMgr()->m_logicRegistry->FindTemplate(name);
    if (!logicTemplate) {
        return NULL;
    }

    return CreateSpriteObject(id, x, y, sortKey, logicTemplate, objectFlags);
}

i32 CDDrawChildGroup::AddObject(CGameObject* obj) {
    if (obj == NULL) {
        return 0;
    }
    InsertSorted(obj, 1);
    return 1;
}

i32 CDDrawChildGroup::AttachSprite(
    CWwdGameObject* obj,
    i32 x,
    i32 y,
    i32 sortKey,
    const std::string& name,
    i32 objectFlags
) {
    if (!obj) {
        return 0;
    }
    CLogicRecord* logicTemplate =
        OwnerMgr()->m_logicRegistry->FindTemplate(name);
    if (!logicTemplate) {
        return 0;
    }
    obj->m_flags = objectFlags;
    if (!obj->Setup(x, y, sortKey, logicTemplate)) {
        return 0;
    }

    this->InsertSorted(obj, 1);
    if (HAS(static_cast<WwdGameObjectFlags>(objectFlags),
            WWD_GAME_OBJECT_FLAG_DISPATCH_ON_CREATE)) {

        obj->m_logicRecord->m_dispatch(static_cast<CGameObject*>(obj));
    }
    return 1;
}

CWwdGameObject* CDDrawChildGroup::CreateContainerObject(
    int id,
    int x,
    int y,
    int sortKey,
    CLogicRecord* logicTemplate,
    int objectFlags
) {
    CWwdGameObject* result = new CWwdGameObject(OwnerMgr(), id, objectFlags);
    if (result->Setup(x, y, sortKey, logicTemplate) == 0) {
        if (result != NULL) {
            delete result;
        }
        return NULL;
    }
    InsertSorted(result, 1);
    if (HAS(static_cast<WwdGameObjectFlags>(objectFlags),
            WWD_GAME_OBJECT_FLAG_DISPATCH_ON_CREATE)) {
        result->m_logicRecord->m_dispatch(result);
    }
    return result;
}

CWwdGameObject* CDDrawChildGroup::CreateNamedContainerObject(
    int id,
    int x,
    int y,
    int sortKey,
    const std::string& name,
    int objectFlags
) {
    CLogicRecord* logicTemplate =
        OwnerMgr()->m_logicRegistry->FindTemplate(name);
    if (logicTemplate == NULL) {
        return NULL;
    }
    return CreateContainerObject(id, x, y, sortKey, logicTemplate, objectFlags);
}

void CDDrawChildGroup::TickKillCues(i32 advance) {

    static std::vector<CGameObject*> s_killQueue;

    static std::vector<CGameObject*> s_sortQueue;
    s_killQueue.clear();
    s_sortQueue.clear();

    if (advance != 0) {
        u32 now = timeGetTime();
        u32 delta = now - g_soundCueTimeMs;
        g_engineFrameDelta = delta;
        g_soundCueTimeMs = now;
    }

    std::list<CGameObject*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        CLogicRecord* record = obj->m_logicRecord;
        if (record->Consume(static_cast<i32>(g_engineFrameDelta)) == 0) {
            i32* refc = &record->m_frameDelay;
            if (*refc != 0) {
                --*refc;
            } else {
                record->m_dispatch(static_cast<CGameObject*>(obj));
            }
        }
        WwdGameObjectFlags objectFlags = static_cast<WwdGameObjectFlags>(obj->m_flags);
        if (HAS(objectFlags, WWD_GAME_OBJECT_FLAG_PENDING_DELETE)) {
            s_killQueue.push_back((obj));
        } else if (HAS(objectFlags, WWD_GAME_OBJECT_FLAG_SORT_PENDING)) {
            s_sortQueue.push_back((obj));
        }
    }

    i32 i;
    for (i = 0; i < static_cast<i32>(s_killQueue.size()); i++) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(s_killQueue[i]);
        if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags),
                WWD_GAME_OBJECT_FLAG_DISPATCH_OBJECT_REMOVED)) {
            CLogicRecord* record = obj->m_logicRecord;
            record->SetLogicEvent(ACT_OBJECT_REMOVED);
            record->m_dispatch(static_cast<CGameObject*>(obj));
        }
        if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags), WWD_GAME_OBJECT_FLAG_UNREGISTERED)) {
            if (obj != NULL) {
                delete obj;
            }
        } else {
            m_list.erase(obj->m_posCache);
            m_registeredGameObjectsById.erase(WwdKey(obj));
            m_activeGameObjectsById.erase(WwdKey(obj));
            if (obj != NULL) {
                delete obj;
            }
        }
    }

    for (i = 0; i < static_cast<i32>(s_sortQueue.size()); i++) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(s_sortQueue[i]);
        obj->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
        m_list.erase(obj->m_posCache);
        InsertSorted(obj, 0);
    }
}

void CDDrawChildGroup::RenderChildren(CDDrawSurfacePair* target) {
    std::list<CGameObject*>::iterator n = m_list.begin();
    if (n != m_list.end()) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->Render(target);
        } while (n != m_list.end());
    }
}

void CDDrawChildGroup::BltDirtyChildren(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {
    std::list<CGameObject*>::iterator n = m_list.begin();
    if (n != m_list.end()) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirty(dst, src);
        } while (n != m_list.end());
    }
}

void CDDrawChildGroup::BltDirtyChildrenEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    std::list<CGameObject*>::iterator n = m_list.begin();
    if (n != m_list.end()) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirtyEx(dst, src, restoreSrc);
        } while (n != m_list.end());
    }
    BltDirtyChildren(src, restoreSrc);
}

void CDDrawChildGroup::BltDirtyChildRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    std::list<CGameObject*>::iterator n = m_list.begin();
    if (n != m_list.end()) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirtyRegions(dst, src, restoreSrc);
        } while (n != m_list.end());
    }
    BltDirtyChildren(src, restoreSrc);
}

void CDDrawChildGroup::InvalidateChildShadows() {
    std::list<CGameObject*>::iterator n = m_list.begin();
    if (n != m_list.end()) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->m_shadow.m_armed = -1;
        } while (n != m_list.end());
    }
}

void CDDrawChildGroup::RemoveAndDelete(CWwdGameObject* obj) {
    if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags), WWD_GAME_OBJECT_FLAG_UNREGISTERED)) {
        delete obj;
        return;
    }
    m_list.erase(obj->m_posCache);
    m_registeredGameObjectsById.erase(WwdKey(obj));
    m_activeGameObjectsById.erase(WwdKey(obj));
    delete obj;
}

void CDDrawChildGroup::ReinsertUnflagged(CWwdGameObject* obj) {
    obj->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
    m_list.erase(obj->m_posCache);
    InsertSorted(obj, 0);
}

void CDDrawChildGroup::InsertSorted(CGameObject* obj, i32 addToMaps) {
    if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags), WWD_GAME_OBJECT_FLAG_UNREGISTERED)) {
        obj->m_posCache = m_list.end();
        return;
    }
    if (addToMaps != 0) {
        m_activeGameObjectsById[WwdKey(obj)] = obj;
        REGISTER_CHILD_OBJECT_ID(obj);
    }
    std::list<CGameObject*>::iterator pos = m_list.begin();
    i32 key = obj->m_sortKey;
    while (pos != m_list.end()) {
        std::list<CGameObject*>::iterator cur = pos;
        CWwdGameObject* data = static_cast<CWwdGameObject*>(NextChild(pos));
        if (data->m_sortKey > key
            && !HAS(
                static_cast<WwdGameObjectFlags>(data->m_flags),
                WWD_GAME_OBJECT_FLAG_SORT_PENDING
            )) {
            obj->m_posCache = (m_list.insert(cur, (obj)));
            return;
        }
    }
    obj->m_posCache = m_list.insert(m_list.end(), (obj));
}

void CDDrawChildGroup::ClearChildren() {
    DestroyChildren();
}

void CDDrawChildGroup::CollideBroadcast() {
    i32 mask1;
    i32 mask2;
    std::list<CGameObject*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CGameObject* oi = NextChild(pos);
        if (!(oi->m_flags & IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION))) {
            std::list<CGameObject*>::iterator ip = pos;
            while (ip != m_list.end()) {
                CGameObject* oj = NextChild(ip);
                i32 fj = oj->m_flags;
                if (fj & IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION)) {
                    continue;
                }
                i32 fi = oi->m_flags;
                if (WorldSpaceDifference(oj->m_flags, oi->m_flags)) {
                    continue;
                }

                if (!(fi & IDX(WWD_GAME_OBJECT_FLAG_IGNORE_HITS))
                    && !(fj & IDX(WWD_GAME_OBJECT_FLAG_DISABLE_ATTACKS))) {
                    mask1 = ObjectTypeBits(oj->m_objectType, oi->m_hitTypeFlags);
                    mask2 = oj->AttackBits(oi);
                    if (mask1 || mask2) {
                        i32 overlap;
                        if (oj->m_switchRect.left == COORD_UNSET) {
                            overlap = 0;
                        } else if (oi->m_area.left == COORD_UNSET) {
                            overlap = 0;
                        } else {
                            RECT ra, rb;
                            PLACE_OBJECT_RECT(ra, oi, m_area);
                            PLACE_OBJECT_RECT(rb, oj, m_switchRect);
                            overlap = RectsOverlap(&ra, &rb);
                        }
                        if (overlap) {
                            if (mask2) {
                                CLogicRecord* attackLogic = oj->m_attackLogic;
                                if (attackLogic != NULL) {
                                    oj->m_attackTarget = oi;

                                    attackLogic->m_dispatch(oj);
                                }
                            }
                            if (mask1) {
                                if (oi->m_flags
                                    & IDX(WWD_GAME_OBJECT_FLAG_DAMAGE_HEALTH_DIRECTLY)) {
                                    if ((oi->m_health = oi->m_health - oj->m_damage) <= 0) {

                                        oi->m_logicRecord->SetLogicEvent(ACT_HEALTH_DEPLETED);
                                    }
                                } else {
                                    CLogicRecord* hitLogic = oi->m_hitLogic;
                                    if (hitLogic != NULL) {
                                        oi->m_hitSource = oj;
                                        hitLogic->m_dispatch(oi);
                                    }
                                }
                            }
                        }
                    }
                }

                if (oj->m_flags & IDX(WWD_GAME_OBJECT_FLAG_IGNORE_HITS)) {
                    continue;
                }
                if (oi->m_flags & IDX(WWD_GAME_OBJECT_FLAG_DISABLE_ATTACKS)) {
                    continue;
                }
                mask1 = ObjectTypeBits(oi->m_objectType, oj->m_hitTypeFlags);
                mask2 = oi->AttackBits(oj);
                if ((mask1 || mask2) && BoxesOverlap(oj, oi)) {
                    if (mask2) {
                        CLogicRecord* attackLogic = oi->m_attackLogic;
                        if (attackLogic != NULL) {
                            oi->m_attackTarget = oj;
                            attackLogic->m_dispatch(oi);
                        }
                    }
                    if (mask1) {
                        oj->Notify(oi);
                    }
                }
            }
        }
    }
}

#include <Wwd/WwdRectOverlapInline.h>

i32 CDDrawChildGroup::BoxesOverlap(CGameObject* areaObj, CGameObject* switchObj) {
    if (switchObj->m_switchRect.left == COORD_UNSET) {
        return 0;
    }
    if (areaObj->m_area.left == COORD_UNSET) {
        return 0;
    }

    RECT ra, rb;
    PLACE_OBJECT_RECT(ra, areaObj, m_area);
    PLACE_OBJECT_RECT(rb, switchObj, m_switchRect);
    return CDDrawRectsOverlap(&ra, &rb);
}
#undef PLACE_OBJECT_RECT

static char s_dbgRle[] = "RLE";

static char s_dbgVid[] = "VID";

static char s_dbgSys[] = "SYS";

void CDDrawChildGroup::DrawObjectDebugGeometry() {
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_HIT_RECT)) {
        std::list<CGameObject*>::iterator pos = m_list.begin();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->GetDrawTarget()->GetBackPair();
        if (pos != m_list.end()) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_area.left != COORD_UNSET) {
                    DrawObjectDebugRect(obj, obj->m_area, view, drawHost);
                }
            } while (pos != m_list.end());
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_ATTACK_RECT)) {
        std::list<CGameObject*>::iterator pos = m_list.begin();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->GetDrawTarget()->GetBackPair();
        if (pos != m_list.end()) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_switchRect.left != COORD_UNSET) {
                    DrawObjectDebugRect(obj, obj->m_switchRect, view, drawHost);
                }
            } while (pos != m_list.end());
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_MOVE_RECT)) {
        std::list<CGameObject*>::iterator pos = m_list.begin();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->GetDrawTarget()->GetBackPair();
        if (pos != m_list.end()) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_extent.left != COORD_UNSET) {
                    DrawObjectDebugRect(obj, obj->m_extent, view, drawHost);
                }
            } while (pos != m_list.end());
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_ORIGIN)) {
        std::list<CGameObject*>::iterator pos = m_list.begin();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->GetDrawTarget()->GetBackPair();
        if (pos != m_list.end()) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                i32 x = obj->m_screenX;
                if (x != COORD_UNSET) {

                    WwdPlaneFlags fl = static_cast<WwdPlaneFlags>(view->m_flags);
                    i32 y = obj->m_screenY;
                    if (HAS(fl, WWD_PLANE_FLAG_WRAP_X)) {
                        i32 w = view->m_planePixelWidth;
                        if (x < 0) {
                            x = x + w;
                        } else if (x >= w) {
                            x = x - w;
                        }
                        i32 farEdge = view->m_planeViewRect.right;
                        if (farEdge >= w && x < view->m_planeViewRect.left && x <= farEdge - w) {
                            x = x + w;
                        }
                    }
                    if (HAS(fl, WWD_PLANE_FLAG_WRAP_Y)) {
                        i32 h = view->m_planePixelHeight;
                        if (y < 0) {
                            y = y + h;
                        } else if (y >= h) {
                            y = y - h;
                        }
                        i32 farEdge = view->m_planeViewRect.bottom;
                        if (farEdge >= h && y < view->m_planeViewRect.top && y <= farEdge - h) {
                            y = y + h;
                        }
                    }
                    drawHost->DrawCross(
                        view->m_viewportRect.left - view->m_planeViewRect.left + x,
                        view->m_viewportRect.top - view->m_planeViewRect.top + y
                    );
                }
            } while (pos != m_list.end());
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_SURFACE_MEMORY)) {
        std::list<CGameObject*>::iterator pos = m_list.begin();
        CDDrawSurfacePair* drawHost = OwnerMgr()->GetDrawTarget()->GetBackPair();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        if (pos != m_list.end()) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_screenX == COORD_UNSET) {
                    continue;
                }
                if (obj->GetClassId() != CLASSID_SERIALREF) {
                    continue;
                }
                CImage* fr = obj->m_frameImage;
                if (fr == NULL) {
                    continue;
                }
                i32 x = obj->m_screenX;
                i32 y = obj->m_screenY;
                RECT box;
                SetRect(&box, x - 0x20, y + 8, x + 0x20, y + 0x20);
                RECT rc = box;
                view->WorldToViewport(&rc.left, &rc.top);
                view->WorldToViewport(&rc.right, &rc.bottom);
                if (fr->m_owned != NULL) {
                    drawHost->DrawLabel(&rc, s_dbgRle);
                } else if (fr->m_surface != NULL
                           && SurfaceCaps(fr->m_surface, DDSCAPS_VIDEOMEMORY) != 0) {
                    drawHost->DrawLabel(&rc, s_dbgVid);
                } else if (fr->m_surface != NULL
                           && SurfaceCaps(fr->m_surface, DDSCAPS_SYSTEMMEMORY) != 0) {
                    drawHost->DrawLabel(&rc, s_dbgSys);
                } else {
                    drawHost->DrawLabel(&rc, "???");
                }
            } while (pos != m_list.end());
        }
    }
}

void CDDrawChildGroup::DrawObjectCounts() {
    i32 w, h;
    if (!(m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_SORT_KEY))) {
        return;
    }
    std::list<CGameObject*>::iterator pos = m_list.begin();
    CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
    CDDrawSurfacePair* drawHost = OwnerMgr()->GetDrawTarget()->GetBackPair();
    if (pos == m_list.end()) {
        return;
    }
    do {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        i32 oy = obj->m_screenY;
        i32 ox = obj->m_screenX;
        RECT box;
        SetRect(&box, ox - 0x20, oy - 8, ox + 0x20, oy + 8);
        RECT rc;
        i32 wl = box.left;
        i32 wt = box.top;
        rc.right = box.right;
        rc.bottom = box.bottom;
        WwdPlaneFlags fl = static_cast<WwdPlaneFlags>(view->m_flags);
        if (HAS(fl, WWD_PLANE_FLAG_WRAP_X)) {
            w = view->m_planePixelWidth;
            if (box.left < 0) {
                wl = box.left + w;
            } else if (box.left >= w) {
                wl = box.left - w;
            }
            i32 farEdge = view->m_planeViewRect.right;
            if (farEdge >= w && wl < view->m_planeViewRect.left && wl <= farEdge - w) {
                wl += w;
            }
        }
        if (HAS(fl, WWD_PLANE_FLAG_WRAP_Y)) {
            h = view->m_planePixelHeight;
            if (box.top < 0) {
                wt = box.top + h;
            } else if (box.top >= h) {
                wt = box.top - h;
            }
            i32 farEdge = view->m_planeViewRect.bottom;
            if (farEdge >= h && wt < view->m_planeViewRect.top && wt <= farEdge - h) {
                wt += h;
            }
        }
        rc.left = wl - view->m_planeViewRect.left + view->m_viewportRect.left;
        rc.top = wt - view->m_planeViewRect.top + view->m_viewportRect.top;

        view->WorldToViewport(&rc.right, &rc.bottom);
        drawHost->DrawCount(&rc, obj->m_sortKey);
    } while (pos != m_list.end());
}

i32 CDDrawChildGroup::CheckSortOrder() {
    std::list<CGameObject*>::iterator node = m_list.begin();
    CWwdGameObject* anchor = static_cast<CWwdGameObject*>(NextChild(node));
    if (anchor != NULL) {
        while (node != m_list.end() && anchor != NULL
               && HAS(
                   static_cast<WwdGameObjectFlags>(anchor->m_flags),
                   WWD_GAME_OBJECT_FLAG_SORT_PENDING
               )) {
            anchor = static_cast<CWwdGameObject*>(NextChild(node));
        }
        if (anchor != NULL) {
            i32 key = anchor->m_sortKey;
            while (node != m_list.end()) {
                CGameObject* cur_obj = NextChild(node);
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
                if (!HAS(
                        static_cast<WwdGameObjectFlags>(obj->m_flags),
                        WWD_GAME_OBJECT_FLAG_SORT_PENDING
                    )) {
                    i32 curKey = obj->m_sortKey;
                    if (key > curKey) {
                        anchor->GetClassId();
                        obj->GetClassId();
                    } else {
                        key = curKey;
                        anchor = obj;
                    }
                }
            }
        }
    }
    return 1;
}

CWwdGameObject* CDDrawChildGroup::FindById(i32 id) {
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_id == id) {
            return obj;
        }
    }
    return NULL;
}

CWwdGameObject* CDDrawChildGroup::FindSerialRefById(i32 id) {
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id) {
            return obj;
        }
    }
    return NULL;
}

CWwdGameObject* CDDrawChildGroup::FindByLogicRecord(i32 id, CLogicRecord* logicRecord) {
    std::list<CGameObject*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id) {

            CLogicRecord* record = obj->m_logicRecord;
            if (record->m_dispatch == logicRecord->m_dispatch) {
                return obj;
            }
        }
    }
    return NULL;
}

CGameObject* CDDrawChildGroup::Find(i32 id, const std::string& key) {
    CLogicRecord* logicTemplate =
        OwnerMgr()->m_logicRegistry->FindTemplate(key);
    std::list<CGameObject*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CGameObject* obj = NextChild(pos);
        LoadableClassId tag = obj->GetClassId();
        if (tag == CLASSID_WWD_SPRITE_OBJECT && obj->m_id == id
            && obj->m_logicRecord->m_dispatch == logicTemplate->m_dispatch) {
            return obj;
        }
    }
    return NULL;
}

CWwdGameObject* CDDrawChildGroup::FindByIdAndCollisionCategory(i32 id, u32 collisionCategory) {
    std::list<CGameObject*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));

        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id
            && obj->m_objectType == collisionCategory) {
            return obj;
        }
    }
    return NULL;
}

CWwdGameObject* CDDrawChildGroup::FindByObjectId(i32 objectId) {
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_objectId == objectId) {
            return obj;
        }
    }
    return NULL;
}

CWwdGameObject* CDDrawChildGroup::FindSerialRefByObjectId(i32 objectId) {
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_objectId == objectId) {
            return obj;
        }
    }
    return NULL;
}

i32 CDDrawChildGroup::IsKindUnique(i32 kind) {
    CWwdGameObject* found = NULL;
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_id == kind) {
            if (found != NULL) {
                return 0;
            }
            found = obj;
        }
    }
    return 1;
}

i32 CDDrawChildGroup::CountByKind(i32 kind) {
    i32 count = 0;
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_id == kind) {
            ++count;
        }
    }
    return count;
}

void CDDrawChildGroup::PruneList() {
    std::list<CGameObject*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        std::list<CGameObject*>::iterator cur = pos;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        if (obj != NULL && !(obj->m_flags & IDX(WWD_GAME_OBJECT_FLAG_PRESERVE_ON_PRUNE))) {
            m_list.erase(cur);
            m_activeGameObjectsById.erase(WwdKey(obj));
            m_registeredGameObjectsById.erase(WwdKey(obj));
            delete obj;
        }
    }
}

i32 CDDrawChildGroup::SumWeighted() {
    i32 i = 0;
    i32 sum = 0;
    std::list<CGameObject*>::iterator node = m_list.begin();
    while (node != m_list.end()) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        sum += i * (obj->m_screenX + obj->m_sortKey + obj->m_screenY + obj->m_id);
        ++i;
    }
    return sum;
}

void CDDrawChildGroup::RemoveAll(std::list<CGameObject*>::iterator pos, CGameObject* obj) {
    REMOVE_ACTIVE_OBJECT_AT(pos, obj);
    m_registeredGameObjectsById.erase(WwdKey(obj));
}

void CDDrawChildGroup::RemoveByPosition(std::list<CGameObject*>::iterator pos, CGameObject* obj) {
    REMOVE_ACTIVE_OBJECT_AT(pos, obj);
}

void CDDrawChildGroup::RegisterObjectId(CWwdGameObject* obj) {
    REGISTER_CHILD_OBJECT_ID(obj);
}

i32 CDDrawChildGroup::CountActive() {
    i32 n = 0;
    std::map<i32, CGameObject*>::iterator pos = m_registeredGameObjectsById.begin();
    if (pos != m_registeredGameObjectsById.end()) {
        do {
            i32 key = 0;
            CWwdGameObject* val = NULL;
            (key = pos->first, val = static_cast<CWwdGameObject*>(pos->second), ++pos);
            if (val != NULL
                && !HAS(
                    static_cast<WwdGameObjectFlags>(val->m_flags),
                    WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
                )) {
                ++n;
            }
        } while (pos != m_registeredGameObjectsById.end());
    }
    return n;
}

i32 CDDrawChildGroup::DispatchSerializationToObjects(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId
) {
    if (ar == NULL) {
        return 0;
    }
    std::map<i32, CGameObject*>::iterator pos = m_registeredGameObjectsById.begin();
    if (pos != m_registeredGameObjectsById.end()) {
        do {
            i32 key = 0;
            CWwdGameObject* val = NULL;
            (key = pos->first, val = static_cast<CWwdGameObject*>(pos->second), ++pos);
            if (val != NULL
                && !HAS(
                    static_cast<WwdGameObjectFlags>(val->m_flags),
                    WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
                )) {
                val->SerializeDispatch(ar, mode, typeId, val);
            }
        } while (pos != m_registeredGameObjectsById.end());
    }
    return 1;
}

i32 CDDrawChildGroup::WriteObjectSnapshots(CFileMemBase* ar, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    std::map<i32, CGameObject*>::iterator pos = m_registeredGameObjectsById.begin();
    if (pos != m_registeredGameObjectsById.end()) {
        do {
            i32 key = 0;
            CWwdGameObject* val = NULL;
            (key = pos->first, val = static_cast<CWwdGameObject*>(pos->second), ++pos);
            if (val != NULL
                && !HAS(
                    static_cast<WwdGameObjectFlags>(val->m_flags),
                    WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
                )) {

                val->WriteSnapshot(ar, typeId);
            }
        } while (pos != m_registeredGameObjectsById.end());
    }
    return 1;
}

i32 CDDrawChildGroup::LoadObjects(class CFileMemBase* reader, u32 count, LogicTypeId unused) {
    i32 savedCounter = 0;
    if (reader == NULL) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        WwdSnapshot desc;
        CUserLogic* child;
        reader->Read(&desc, sizeof(desc));

        CGameObject* createdObj = NULL;
        if (LookupObjectById(m_registeredGameObjectsById, desc.m_objectId) != NULL) {
            return 0;
        }

        savedCounter = g_wwdObjIdCounter;
        g_wwdObjIdCounter = desc.m_objectId;

        switch (desc.m_classId) {
            case CLASSID_WWD_DEFERRED_OBJECT: {
                i32 sortKey = desc.m_sortKey;
                i32 id = desc.m_id;
                createdObj = CreateDeferredObject(
                    id,
                    sortKey,
                    OwnerMgr()->m_logicRegistry->FindTemplate(desc.m_logicTypeName),
                    0
                );
                break;
            }
            case CLASSID_WWD_SPRITE_OBJECT: {
                i32 sortKey = desc.m_sortKey;
                i32 y = desc.m_screenY;
                i32 x = desc.m_screenX;
                i32 id = desc.m_id;
                CLogicRecord* logicTemplate = OwnerMgr()->m_logicRegistry->FindTemplate(desc.m_logicTypeName);
                if (logicTemplate == NULL) {
                    createdObj = NULL;
                } else {
                    createdObj = CreateSpriteObject(id, x, y, sortKey, logicTemplate, 0);
                }
                break;
            }
            case CLASSID_WWD_CONTAINER_OBJECT: {
                i32 sortKey = desc.m_sortKey;
                i32 y = desc.m_screenY;
                i32 x = desc.m_screenX;
                i32 id = desc.m_id;
                CLogicRecord* logicTemplate = OwnerMgr()->m_logicRegistry->FindTemplate(desc.m_logicTypeName);
                if (logicTemplate == NULL) {
                    createdObj = NULL;
                } else {
                    createdObj = CreateContainerObject(id, x, y, sortKey, logicTemplate, 0);
                }
                break;
            }
            case CLASSID_CALLBACKOBJ: {

                CWwdGameObject* rec = NULL;
                if (OwnerMgr()->DispatchSerializationCallback(
                        reader,
                        SERIAL_CREATE_BY_SERIAL_ID,
                        static_cast<LogicTypeId>(desc.m_serialTypeId),
                        static_cast<void*>(&rec)
                    )
                    == 0) {
                    return 0;
                }
                if (rec == NULL) {
                    return 0;
                }
                rec->m_id = desc.m_id;

                if (AttachSprite(
                        rec,
                        desc.m_screenX,
                        desc.m_screenY,
                        desc.m_sortKey,
                        desc.m_logicTypeName,
                        0
                    )
                    == 0) {
                    return 0;
                }
                createdObj = rec;
                break;
            }
            default:
                break;
        }

        g_wwdObjIdCounter = savedCounter;
        if (createdObj == NULL) {
            return 0;
        }
        if (createdObj->m_logicRecord == NULL) {
            return 0;
        }
        if (desc.m_logicTypeId != LOGIC_UNSET) {

            child = NULL;
            if (OwnerMgr()->DispatchSerializationCallback(
                    reader,
                    SERIAL_CREATE,
                    desc.m_logicTypeId,
                    static_cast<void*>(&child)
                )
                == 0) {
                return 0;
            }
            if (child == NULL) {
                return 0;
            }

            createdObj->m_logicRecord->m_userLogic = child;
        }
    }
    return 1;
}

i32 CDDrawChildGroup::SerializeObjects(CFileMemBase* ar, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    std::map<i32, CGameObject*>::iterator pos = m_registeredGameObjectsById.begin();
    while (pos != m_registeredGameObjectsById.end()) {
        i32 key = 0;
        CWwdGameObject* val = NULL;
        (key = pos->first, val = static_cast<CWwdGameObject*>(pos->second), ++pos);
        if (val != NULL
            && !HAS(
                static_cast<WwdGameObjectFlags>(val->m_flags),
                WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
            )) {
            i32 objectId = val->m_objectId;
            ar->Write(&objectId, sizeof(objectId));
            if (val->SerializeDispatch(ar, SERIAL_SAVE, typeId, val) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

i32 CDDrawChildGroup::DeserializeObjects(CFileMemBase* ar, u32 count, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        i32 objectId = 0;
        ar->Read(&objectId, sizeof(objectId));
        if (objectId == 0) {
            return 0;
        }
        CWwdGameObject* obj = LookupObjectById(m_registeredGameObjectsById, objectId);
        if (obj == NULL) {
            return 0;
        }
        if (obj->m_logicRecord == NULL) {
            return 0;
        }
        if ((typeId & 1) != LOGIC_UNSET) {
            TRACE("%s\n", (std::string(obj->m_name)).c_str());
        }
        if (obj->SerializeDispatch(ar, SERIAL_LOAD, typeId, obj) == 0) {
            return 0;
        }
    }
    return 1;
}

i32 CDDrawChildGroup::PruneOrphans() {
    i32 n = 0;
    std::map<i32, CGameObject*>::iterator pos = m_registeredGameObjectsById.begin();
    while (pos != m_registeredGameObjectsById.end()) {
        i32 key = 0;
        CWwdGameObject* val = NULL;
        (key = pos->first, val = static_cast<CWwdGameObject*>(pos->second), ++pos);
        if (val != NULL) {

            if (LookupActiveObject(m_activeGameObjectsById, WwdKey(val)) == NULL) {
                m_registeredGameObjectsById.erase(WwdKey(val));
                if (val != NULL) {
                    delete val;
                }
                ++n;
            }
        }
    }
    return n;
}

WwdDirtyRect::WwdDirtyRect() {
    m_rect.left = COORD_UNSET;
    m_armed = -1;
}

WwdGridNode::WwdGridNode() {
    m_bucket = NULL;
    m_reserved08 = 0;
}

WwdRegion::WwdRegion() : WwdGridNode(WwdGridNode::NO_SEED) {
    SeedFields();
}

CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

CLogicRecord::CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 logicFlags)
    : CWapObj(owner, id, logicFlags, CWapObj::NO_SEED) {
    ResetLogicFields();
}

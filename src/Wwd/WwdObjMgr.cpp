#include <rva.h>

#include <Wwd/WwdObjMgr.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Enums.h>
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

#include <new>

DATA(0x0021ba74)
i32 g_wwdObjIdCounter = 1;
DATA(0x0021ba80)
b32 g_soundEnabled = true;
DATA(0x0021ba84)
i32 g_soundVolumePercent = 100;

inline CLogicRecord* LookupLogicTemplate(CMapStringToOb& map, LPCTSTR name) {
    CObject* ob = NULL;
    map.Lookup(name, ob);
    return static_cast<CLogicRecord*>(ob);
}

inline void* WwdKey(CGameObject* o) {
    AddrWord<char> k;
    k.m_word = o->m_objectId;
    return k.m_addr;
}

RVA(0x001594c0, 0x5)
void CDDrawChildGroup::Unload() {
    this->DestroyChildren();
}

RVA(0x001594d0, 0x54)
void CDDrawChildGroup::DestroyChildren() {
    CGameLevel* p = OwnerMgr()->m_level;
    if (p != NULL) {

        CDDrawWorkerHost* q = static_cast<CDDrawWorkerHost*>(p->m_mainPlane);
        if (q != NULL) {
            q->Prune();
        }
    }
    POSITION n = m_list.GetHeadPosition();
    while (n != NULL) {
        CGameObject* cur_obj = NextChild(n);
        CGameObject* obj = cur_obj;
        if (obj != NULL) {
            delete obj;
        }
    }
    m_list.RemoveAll();
    m_activeGameObjectsById.RemoveAll();
    m_registeredGameObjectsById.RemoveAll();
}

RVA(0x00159530, 0x185)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001596c0, 0x53)
CWwdDotObject* CDDrawChildGroup::CreateNamedDotObject(
    int id,
    int x,
    int y,
    int sortKey,
    const char* name,
    int dotColor,
    int objectFlags
) {
    return CreateDotObject(
        id,
        x,
        y,
        sortKey,
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, name),
        dotColor,
        objectFlags
    );
}

RVA(0x00159720, 0x170)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00159890, 0x44)
CWwdDeferredObject* CDDrawChildGroup::CreateNamedDeferredObject(
    int id,
    int sortKey,
    const char* name,
    int objectFlags
) {
    return CreateDeferredObject(
        id,
        sortKey,
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, name),
        objectFlags
    );
}

RVA(0x001598e0, 0x1ab)
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

RVA(0x00159a90, 0x57)
CWwdSpriteObject* CDDrawChildGroup::CreateSprite(
    i32 id,
    i32 x,
    i32 y,
    i32 sortKey,
    const char* name,
    i32 objectFlags
) {
    CLogicRecord* logicTemplate =
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, name);
    if (!logicTemplate) {
        return NULL;
    }

    return CreateSpriteObject(id, x, y, sortKey, logicTemplate, objectFlags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00159af0, 0x1b)
i32 CDDrawChildGroup::AddObject(CGameObject* obj) {
    if (obj == NULL) {
        return 0;
    }
    InsertSorted(obj, 1);
    return 1;
}

RVA(0x00159b10, 0x92)
i32 CDDrawChildGroup::AttachSprite(
    CWwdGameObject* obj,
    i32 x,
    i32 y,
    i32 sortKey,
    const char* name,
    i32 objectFlags
) {
    if (!obj) {
        return 0;
    }
    CLogicRecord* logicTemplate =
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, name);
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

RVA(0x00159bb0, 0x13d)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00159cf0, 0x57)
CWwdGameObject* CDDrawChildGroup::CreateNamedContainerObject(
    int id,
    int x,
    int y,
    int sortKey,
    const char* name,
    int objectFlags
) {
    CLogicRecord* logicTemplate =
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, name);
    if (logicTemplate == NULL) {
        return NULL;
    }
    return CreateContainerObject(id, x, y, sortKey, logicTemplate, objectFlags);
}

// @early-stop
RVA(0x00159d50, 0x200)
void CDDrawChildGroup::TickKillCues(i32 advance) {
    RVA_DYNINIT(0x00159f60, 0xa, killQueue)
    DATA(0x002c0300)
    static CObArray killQueue;
    RVA_DYNINIT(0x00159f50, 0xa, sortQueue)
    DATA(0x002c02e8)
    static CObArray sortQueue;
    killQueue.SetSize(0, -1);
    sortQueue.SetSize(0, -1);

    if (advance != 0) {
        u32 now = timeGetTime();
        u32 delta = now - g_soundCueTimeMs;
        g_engineFrameDelta = delta;
        g_soundCueTimeMs = now;
    }

    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
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
            killQueue.Add(static_cast<CObject*>(obj));
        } else if (HAS(objectFlags, WWD_GAME_OBJECT_FLAG_SORT_PENDING)) {
            sortQueue.Add(static_cast<CObject*>(obj));
        }
    }

    i32 i;
    for (i = 0; i < killQueue.GetSize(); i++) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(killQueue.GetData()[i]);
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
            m_list.RemoveAt(obj->m_posCache);
            m_registeredGameObjectsById.RemoveKey(WwdKey(obj));
            m_activeGameObjectsById.RemoveKey(WwdKey(obj));
            if (obj != NULL) {
                delete obj;
            }
        }
    }

    for (i = 0; i < sortQueue.GetSize(); i++) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(sortQueue.GetData()[i]);
        obj->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
        m_list.RemoveAt(obj->m_posCache);
        InsertSorted(obj, 0);
    }
}

RVA(0x00159f70, 0x23)
void CDDrawChildGroup::RenderChildren(CDDrawSurfacePair* target) {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->Render(target);
        } while (n != NULL);
    }
}

RVA(0x00159fa0, 0x2a)
void CDDrawChildGroup::BltDirtyChildren(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirty(dst, src);
        } while (n != NULL);
    }
}

RVA(0x00159fd0, 0x42)
void CDDrawChildGroup::BltDirtyChildrenEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirtyEx(dst, src, restoreSrc);
        } while (n != NULL);
    }
    BltDirtyChildren(src, restoreSrc);
}

RVA(0x0015a020, 0x42)
void CDDrawChildGroup::BltDirtyChildRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirtyRegions(dst, src, restoreSrc);
        } while (n != NULL);
    }
    BltDirtyChildren(src, restoreSrc);
}

RVA(0x0015a070, 0x1c)
void CDDrawChildGroup::InvalidateChildShadows() {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->m_shadow.m_armed = -1;
        } while (n != NULL);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a090, 0x5e)
void CDDrawChildGroup::RemoveAndDelete(CWwdGameObject* obj) {
    if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags), WWD_GAME_OBJECT_FLAG_UNREGISTERED)) {
        delete obj;
        return;
    }
    m_list.RemoveAt(obj->m_posCache);
    m_registeredGameObjectsById.RemoveKey(WwdKey(obj));
    m_activeGameObjectsById.RemoveKey(WwdKey(obj));
    delete obj;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a0f0, 0x2e)
void CDDrawChildGroup::ReinsertUnflagged(CWwdGameObject* obj) {
    obj->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
    m_list.RemoveAt(obj->m_posCache);
    InsertSorted(obj, 0);
}

#define REGISTER_CHILD_OBJECT_ID(obj) m_registeredGameObjectsById[WwdKey(obj)] = obj

RVA(0x0015a120, 0xaa)
void CDDrawChildGroup::InsertSorted(CGameObject* obj, i32 addToMaps) {
    if (HAS(static_cast<WwdGameObjectFlags>(obj->m_flags), WWD_GAME_OBJECT_FLAG_UNREGISTERED)) {
        obj->m_posCache = NULL;
        return;
    }
    if (addToMaps != 0) {
        m_activeGameObjectsById[WwdKey(obj)] = obj;
        REGISTER_CHILD_OBJECT_ID(obj);
    }
    POSITION pos = m_list.GetHeadPosition();
    i32 key = obj->m_sortKey;
    while (pos != NULL) {
        POSITION cur = pos;
        CWwdGameObject* data = static_cast<CWwdGameObject*>(NextChild(pos));
        if (data->m_sortKey > key
            && !HAS(
                static_cast<WwdGameObjectFlags>(data->m_flags),
                WWD_GAME_OBJECT_FLAG_SORT_PENDING
            )) {
            obj->m_posCache = (m_list.InsertBefore(cur, static_cast<CObject*>(obj)));
            return;
        }
    }
    obj->m_posCache = m_list.AddTail(static_cast<CObject*>(obj));
}

RVA(0x0015a1d0, 0x5)
void CDDrawChildGroup::ClearChildren() {
    DestroyChildren();
}

// @early-stop
RVA(0x0015a1e0, 0x22e)
void CDDrawChildGroup::CollideBroadcast() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* oi = NextChild(pos);
        if (!(oi->m_flags & IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION))) {
            POSITION ip = pos;
            while (ip != NULL) {
                CGameObject* oj = NextChild(ip);
                i32 fj = oj->m_flags;
                if (fj & IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION)) {
                    continue;
                }
                i32 fi = oi->m_flags;
                if ((fi ^ fj) & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
                    continue;
                }

                if (!(fi & IDX(WWD_GAME_OBJECT_FLAG_IGNORE_HITS))
                    && !(fj & IDX(WWD_GAME_OBJECT_FLAG_DISABLE_ATTACKS))) {
                    i32 mask1 = static_cast<i32>(oj->m_objectType) & oi->m_hitTypeFlags;
                    i32 mask2 = static_cast<i32>(oi->m_objectType) & oj->m_attackTypeMask;
                    if (mask1 || mask2) {
                        i32 overlap;
                        if (oj->m_switchRect.left == COORD_UNSET) {
                            overlap = 0;
                        } else if (oi->m_area.left == COORD_UNSET) {
                            overlap = 0;
                        } else {
                            CDDrawRect ra, rb;
                            i32 xi = oi->m_screenX;
                            i32 yi = oi->m_screenY;
                            ra.left = oi->m_area.left + xi;
                            ra.top = oi->m_area.top + yi;
                            ra.right = oi->m_area.right + xi;
                            ra.bottom = oi->m_area.bottom + yi;
                            i32 xj = oj->m_screenX;
                            i32 yj = oj->m_screenY;
                            rb.left = oj->m_switchRect.left + xj;
                            rb.top = oj->m_switchRect.top + yj;
                            rb.right = oj->m_switchRect.right + xj;
                            rb.bottom = oj->m_switchRect.bottom + yj;
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
                                    i32 v = oi->m_health - oj->m_damage;
                                    oi->m_health = v;
                                    if (v <= 0) {

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
                i32 mask1b = oj->m_hitTypeFlags & static_cast<i32>(oi->m_objectType);
                i32 mask2b = static_cast<i32>(oj->m_objectType) & oi->m_attackTypeMask;
                if ((mask1b || mask2b) && BoxesOverlap(oj, oi)) {
                    if (mask2b) {
                        CLogicRecord* attackLogic = oi->m_attackLogic;
                        if (attackLogic != NULL) {
                            oi->m_attackTarget = oj;
                            attackLogic->m_dispatch(oi);
                        }
                    }
                    if (mask1b) {
                        oj->Notify(oi);
                    }
                }
            }
        }
    }
}

// @early-stop
RVA(0x0015a410, 0xdc)
i32 CDDrawChildGroup::BoxesOverlap(CGameObject* areaObj, CGameObject* switchObj) {
    if (switchObj->m_switchRect.left == COORD_UNSET) {
        return 0;
    }
    if (areaObj->m_area.left == COORD_UNSET) {
        return 0;
    }

    CDDrawRect ra, rb;
    i32 xi = areaObj->m_screenX;
    i32 yi = areaObj->m_screenY;
    ra.left = areaObj->m_area.left + xi;
    ra.top = areaObj->m_area.top + yi;
    ra.right = areaObj->m_area.right + xi;
    ra.bottom = areaObj->m_area.bottom + yi;
    i32 xj = switchObj->m_screenX;
    i32 yj = switchObj->m_screenY;
    rb.left = switchObj->m_switchRect.left + xj;
    rb.top = switchObj->m_switchRect.top + yj;
    rb.right = switchObj->m_switchRect.right + xj;
    rb.bottom = switchObj->m_switchRect.bottom + yj;
    if (ra.left > rb.right) {
        return 0;
    }
    if (ra.right < rb.left) {
        return 0;
    }
    if (ra.top > rb.bottom) {
        return 0;
    }
    return ra.bottom >= rb.top;
}

DATA(0x0021ba90)
static char s_dbgRle[] = "RLE";
DATA(0x0021ba8c)
static char s_dbgVid[] = "VID";
DATA(0x0021ba88)
static char s_dbgSys[] = "SYS";

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a4f0, 0x432)
void CDDrawChildGroup::DrawObjectDebugGeometry() {
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_HIT_RECT)) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        if (pos != NULL) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_area.left != COORD_UNSET) {
                    i32 ox = obj->m_screenX;
                    RECT rc;
                    rc.left = obj->m_area.left + ox;
                    i32 oy = obj->m_screenY;
                    rc.top = obj->m_area.top + oy;
                    rc.right = obj->m_area.right + ox;
                    rc.bottom = obj->m_area.bottom + oy;
                    view->WorldToViewport(&rc.left, &rc.top);
                    view->WorldToViewport(&rc.right, &rc.bottom);
                    drawHost->DrawBox(&rc, 0xff);
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_ATTACK_RECT)) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        if (pos != NULL) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_switchRect.left != COORD_UNSET) {
                    i32 ox = obj->m_screenX;
                    RECT rc;
                    rc.left = obj->m_switchRect.left + ox;
                    i32 oy = obj->m_screenY;
                    rc.top = obj->m_switchRect.top + oy;
                    rc.right = obj->m_switchRect.right + ox;
                    rc.bottom = obj->m_switchRect.bottom + oy;
                    view->WorldToViewport(&rc.left, &rc.top);
                    view->WorldToViewport(&rc.right, &rc.bottom);
                    drawHost->DrawBox(&rc, 0xff);
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_MOVE_RECT)) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        if (pos != NULL) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                if (obj->m_extent.left != COORD_UNSET) {
                    i32 ox = obj->m_screenX;
                    RECT rc;
                    rc.left = obj->m_extent.left + ox;
                    i32 oy = obj->m_screenY;
                    rc.top = obj->m_extent.top + oy;
                    rc.right = obj->m_extent.right + ox;
                    rc.bottom = obj->m_extent.bottom + oy;
                    view->WorldToViewport(&rc.left, &rc.top);
                    view->WorldToViewport(&rc.right, &rc.bottom);
                    drawHost->DrawBox(&rc, 0xff);
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_ORIGIN)) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        if (pos != NULL) {
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
            } while (pos != NULL);
        }
    }
    if (m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_SURFACE_MEMORY)) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        if (pos != NULL) {
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
                } else {

                    DDSCAPS caps;
                    i32 vid;
                    if (fr->m_surface != NULL && fr->m_surface->m_ddSurface->GetCaps(&caps) == 0) {
                        vid = caps.dwCaps & DDSCAPS_VIDEOMEMORY;
                    } else {
                        vid = 0;
                    }
                    if (vid != 0) {
                        drawHost->DrawLabel(&rc, s_dbgVid);
                    } else {
                        DDSCAPS caps2;
                        i32 sys;
                        if (fr->m_surface != NULL
                            && fr->m_surface->m_ddSurface->GetCaps(&caps2) == 0) {
                            sys = caps2.dwCaps & DDSCAPS_SYSTEMMEMORY;
                        } else {
                            sys = 0;
                        }
                        if (sys != 0) {
                            drawHost->DrawLabel(&rc, s_dbgSys);
                        } else {
                            drawHost->DrawLabel(&rc, "???");
                        }
                    }
                }
            } while (pos != NULL);
        }
    }
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a930, 0x12c)
void CDDrawChildGroup::DrawObjectCounts() {
    if (!(m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_SORT_KEY))) {
        return;
    }
    POSITION pos = m_list.GetHeadPosition();
    CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
    CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
    if (pos == NULL) {
        return;
    }
    do {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        i32 ox = obj->m_screenX;
        i32 oy = obj->m_screenY;
        RECT box;
        SetRect(&box, ox - 0x20, oy - 8, ox + 0x20, oy + 8);
        RECT rc;
        rc.right = box.right;
        rc.bottom = box.bottom;
        i32 wl = box.left;
        i32 wt = box.top;
        WwdPlaneFlags fl = static_cast<WwdPlaneFlags>(view->m_flags);
        if (HAS(fl, WWD_PLANE_FLAG_WRAP_X)) {
            i32 w = view->m_planePixelWidth;
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
            i32 h = view->m_planePixelHeight;
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
    } while (pos != NULL);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015aa60, 0x70)
i32 CDDrawChildGroup::CheckSortOrder() {
    POSITION node = m_list.GetHeadPosition();
    CWwdGameObject* anchor = static_cast<CWwdGameObject*>(NextChild(node));
    if (anchor != NULL) {
        while (node != NULL && anchor != NULL
               && HAS(
                   static_cast<WwdGameObjectFlags>(anchor->m_flags),
                   WWD_GAME_OBJECT_FLAG_SORT_PENDING
               )) {
            anchor = static_cast<CWwdGameObject*>(NextChild(node));
        }
        if (anchor != NULL) {
            i32 key = anchor->m_sortKey;
            while (node != NULL) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015aad0, 0x20)
CWwdGameObject* CDDrawChildGroup::FindById(i32 id) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_id == id) {
            return obj;
        }
    }
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015aaf0, 0x42)
CWwdGameObject* CDDrawChildGroup::FindSerialRefById(i32 id) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id) {
            return obj;
        }
    }
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015ab40, 0x57)
CWwdGameObject* CDDrawChildGroup::FindByLogicRecord(i32 id, CLogicRecord* logicRecord) {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015aba0, 0x7d)
CGameObject* CDDrawChildGroup::Find(i32 id, const char* key) {
    CLogicRecord* logicTemplate =
        LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, key);
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = NextChild(pos);
        LoadableClassId tag = obj->GetClassId();
        if (tag == CLASSID_WWD_SPRITE_OBJECT && obj->m_id == id
            && obj->m_logicRecord->m_dispatch == logicTemplate->m_dispatch) {
            return obj;
        }
    }
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015ac20, 0x52)
CWwdGameObject* CDDrawChildGroup::FindByIdAndCollisionCategory(i32 id, u32 collisionCategory) {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));

        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id
            && obj->m_objectType == collisionCategory) {
            return obj;
        }
    }
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015ac80, 0x23)
CWwdGameObject* CDDrawChildGroup::FindByObjectId(i32 objectId) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_objectId == objectId) {
            return obj;
        }
    }
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015acb0, 0x45)
CWwdGameObject* CDDrawChildGroup::FindSerialRefByObjectId(i32 objectId) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_objectId == objectId) {
            return obj;
        }
    }
    return NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015ad00, 0x3c)
i32 CDDrawChildGroup::IsKindUnique(i32 kind) {
    CWwdGameObject* found = NULL;
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015ad40, 0x23)
i32 CDDrawChildGroup::CountByKind(i32 kind) {
    i32 count = 0;
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_id == kind) {
            ++count;
        }
    }
    return count;
}

RVA(0x0015ad70, 0x5d)
void CDDrawChildGroup::PruneList() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        if (obj != NULL && !(obj->m_flags & IDX(WWD_GAME_OBJECT_FLAG_PRESERVE_ON_PRUNE))) {
            m_list.RemoveAt(cur);
            m_activeGameObjectsById.RemoveKey(WwdKey(obj));
            m_registeredGameObjectsById.RemoveKey(WwdKey(obj));
            delete obj;
        }
    }
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015add0, 0x35)
i32 CDDrawChildGroup::SumWeighted() {
    i32 i = 0;
    i32 sum = 0;
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        sum += i * (obj->m_screenX + obj->m_sortKey + obj->m_screenY + obj->m_id);
        ++i;
    }
    return sum;
}

#define REMOVE_ACTIVE_OBJECT_AT(pos, obj)                                                          \
    m_list.RemoveAt(pos);                                                                          \
    m_activeGameObjectsById.RemoveKey(WwdKey(obj))

RVA(0x0015ae10, 0x38)
void CDDrawChildGroup::RemoveAll(POSITION pos, CGameObject* obj) {
    REMOVE_ACTIVE_OBJECT_AT(pos, obj);
    m_registeredGameObjectsById.RemoveKey(WwdKey(obj));
}

RVA(0x0015ae50, 0x27)
void CDDrawChildGroup::RemoveByPosition(POSITION pos, CGameObject* obj) {
    REMOVE_ACTIVE_OBJECT_AT(pos, obj);
}

RVA(0x0015ae80, 0x1a)
void CDDrawChildGroup::RegisterObjectId(CWwdGameObject* obj) {
    REGISTER_CHILD_OBJECT_ID(obj);
}

RVA(0x0015aea0, 0x5e)
i32 CDDrawChildGroup::CountActive() {
    i32 n = 0;
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = NULL;
            CWwdGameObject* val = NULL;
            MapGetNext(m_registeredGameObjectsById, pos, key, val);
            if (val != NULL
                && !HAS(
                    static_cast<WwdGameObjectFlags>(val->m_flags),
                    WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
                )) {
                ++n;
            }
        } while (pos != NULL);
    }
    return n;
}

RVA(0x0015af00, 0x81)
i32 CDDrawChildGroup::DispatchSerializationToObjects(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId
) {
    if (ar == NULL) {
        return 0;
    }
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = NULL;
            CWwdGameObject* val = NULL;
            MapGetNext(m_registeredGameObjectsById, pos, key, val);
            if (val != NULL
                && !HAS(
                    static_cast<WwdGameObjectFlags>(val->m_flags),
                    WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
                )) {
                val->SerializeDispatch(ar, mode, typeId, val);
            }
        } while (pos != NULL);
    }
    return 1;
}

RVA(0x0015af90, 0x76)
i32 CDDrawChildGroup::WriteObjectSnapshots(CFileMemBase* ar, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = NULL;
            CWwdGameObject* val = NULL;
            MapGetNext(m_registeredGameObjectsById, pos, key, val);
            if (val != NULL
                && !HAS(
                    static_cast<WwdGameObjectFlags>(val->m_flags),
                    WWD_GAME_OBJECT_FLAG_SKIP_ACTIVE_PASSES
                )) {

                val->WriteSnapshot(ar, typeId);
            }
        } while (pos != NULL);
    }
    return 1;
}

RVA(0x0015b010, 0x2ec)
i32 CDDrawChildGroup::LoadObjects(class CFileMemBase* reader, u32 count, LogicTypeId unused) {
    i32 savedCounter = 0;
    if (reader == NULL) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        WwdSnapshot desc;
        reader->Read(&desc, sizeof(desc));

        CGameObject* createdObj = NULL;
        CWwdGameObject* found = NULL;
        if (MapLookupById(m_registeredGameObjectsById, desc.m_objectId, found) && found != NULL) {
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
                    LookupLogicTemplate(
                        OwnerMgr()->m_logicRegistry->m_templatesByName,
                        desc.m_logicTypeName
                    ),
                    0
                );
                break;
            }
            case CLASSID_WWD_SPRITE_OBJECT: {
                i32 sortKey = desc.m_sortKey;
                i32 y = desc.m_screenY;
                i32 x = desc.m_screenX;
                i32 id = desc.m_id;
                CLogicRecord* logicTemplate = LookupLogicTemplate(
                    OwnerMgr()->m_logicRegistry->m_templatesByName,
                    desc.m_logicTypeName
                );
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
                CLogicRecord* logicTemplate = LookupLogicTemplate(
                    OwnerMgr()->m_logicRegistry->m_templatesByName,
                    desc.m_logicTypeName
                );
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

            CUserLogic* child = NULL;
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

RVA(0x0015b300, 0xc0)
i32 CDDrawChildGroup::SerializeObjects(CFileMemBase* ar, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    while (pos != NULL) {
        void* key = NULL;
        CWwdGameObject* val = NULL;
        MapGetNext(m_registeredGameObjectsById, pos, key, val);
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

static inline CWwdGameObject* LookupObjectById(CMapPtrToPtr& byId, i32 id) {
    CWwdGameObject* found = NULL;
    if (MapLookupById(byId, id, found) == false) {
        found = NULL;
    }
    return found;
}

RVA(0x0015b3c0, 0xec)
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
            TRACE("%s\n", static_cast<LPCTSTR>(CString(obj->m_name)));
        }
        if (obj->SerializeDispatch(ar, SERIAL_LOAD, typeId, obj) == 0) {
            return 0;
        }
    }
    return 1;
}

inline CWwdGameObject* LookupActiveObject(CMapPtrToPtr& map, void* key) {
    CWwdGameObject* found = NULL;
    if (MapLookup(map, key, found) == false) {
        found = NULL;
    }
    return found;
}

RVA(0x0015b4b0, 0x9b)
i32 CDDrawChildGroup::PruneOrphans() {
    i32 n = 0;
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    while (pos != NULL) {
        void* key = NULL;
        CWwdGameObject* val = NULL;
        MapGetNext(m_registeredGameObjectsById, pos, key, val);
        if (val != NULL) {

            if (LookupActiveObject(m_activeGameObjectsById, WwdKey(val)) == NULL) {
                m_registeredGameObjectsById.RemoveKey(WwdKey(val));
                if (val != NULL) {
                    delete val;
                }
                ++n;
            }
        }
    }
    return n;
}

RVA(0x0015b550, 0x11)
WwdDirtyRect::WwdDirtyRect() {
    m_rect.left = COORD_UNSET;
    m_armed = -1;
}

RVA(0x0015b580, 0xb)
WwdGridNode::WwdGridNode() {
    m_bucket = NULL;
    m_reserved08 = 0;
}

RVA(0x0015b590, 0xe)
WwdRegion::WwdRegion() : WwdGridNode(WwdGridNode::NO_SEED) {
    SeedFields();
}

RVA(0x0015b5a0, 0x3d)
CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

RVA(0x0015b5e0, 0x40)
CLogicRecord::CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 logicFlags)
    : CWapObj(owner, id, logicFlags, CWapObj::NO_SEED) {
    ResetLogicFields();
}

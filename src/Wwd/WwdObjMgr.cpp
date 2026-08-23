#include <rva.h>

#include <Wwd/WwdObjMgr.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDSurface.h>
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
#include <Wwd/AnimWorkerAct.h>
#include <Wwd/WwdFactoryObject.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <new>

DATA(0x0021ab14)
i32 g_wwdObjIdCounter = 1;
DATA(0x0021ab20)
i32 g_sndEnabled = 1;
DATA(0x0021ab24)
i32 g_sndCueTag = 100;

// CMapStringToOb::Lookup leaves `out` untouched on a miss, so the clear belongs
// with the lookup: as the inline body's first statement cl schedules it after the
// caller's argument setup, which is retail's order.
inline AnimWorkerObj* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* ob = NULL;
    map.Lookup(name, ob);
    return static_cast<AnimWorkerObj*>(ob);
}

inline void* WwdKey(CGameObject* o) {
    AddrWord<char> k;
    k.m_word = o->m_objectId;
    return k.m_addr;
}

RVA(0x001591e0, 0x5)
void CDDrawChildGroup::Unload() {
    this->DestroyChildren();
}

RVA(0x001591f0, 0x54)
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

RVA(0x00159250, 0x185)
CWwdGameObjectC* CDDrawChildGroup::CreateDotObject(
    int id,
    int x,
    int y,
    int sortKey,
    AnimWorkerObj* tmpl,
    int dotColor,
    int stateFlags
) {
    CWwdGameObjectC* result = new CWwdGameObjectC(OwnerMgr(), id, stateFlags);
    if (result->SetupFlagged(x, y, sortKey, tmpl, dotColor) == 0) {
        if (result != NULL) {
            delete result;
        }
        return 0;
    }
    InsertSorted(result, 1);
    if (stateFlags & 0x200000) {

        result->m_animWorker->m_notify(result);
    }
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001593e0, 0x53)
CWwdGameObjectC* CDDrawChildGroup::CreateNamedDotObject(
    int id,
    int x,
    int y,
    int sortKey,
    const char* name,
    int dotColor,
    int stateFlags
) {
    return CreateDotObject(
        id,
        x,
        y,
        sortKey,
        LookupWorker(OwnerMgr()->m_workerCache->m_workers, name),
        dotColor,
        stateFlags
    );
}

RVA(0x00159440, 0x170)
CWwdGameObjectF*
CDDrawChildGroup::CreateDeferredObject(int id, int sortKey, AnimWorkerObj* tmpl, int stateFlags) {
    CWwdGameObjectF* result = new CWwdGameObjectF(OwnerMgr(), id, stateFlags);
    if (result->SetupDeferred(sortKey, tmpl) == 0) {
        if (result != NULL) {
            delete result;
        }
        return 0;
    }
    InsertSorted(result, 1);
    if (stateFlags & 0x200000) {
        result->m_animWorker->m_notify(result);
    }
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001595b0, 0x44)
CWwdGameObjectF*
CDDrawChildGroup::CreateNamedDeferredObject(int id, int sortKey, const char* name, int stateFlags) {
    return CreateDeferredObject(
        id,
        sortKey,
        LookupWorker(OwnerMgr()->m_workerCache->m_workers, name),
        stateFlags
    );
}

RVA(0x00159600, 0x1ab)
CWwdGameObjectA* CDDrawChildGroup::CreateSpriteObject(
    i32 id,
    i32 x,
    i32 y,
    i32 sortKey,
    AnimWorkerObj* tmpl,
    i32 stateFlags
) {
    CWwdGameObjectA* result =
        new CWwdGameObjectA(OwnerMgr(), id, stateFlags, CGameObject::INLINE_BASE);
    if (result->Setup(x, y, sortKey, tmpl) == 0) {
        if (result != NULL) {
            delete result;
        }
        return 0;
    }
    InsertSorted(result, 1);
    if (stateFlags & 0x200000) {
        result->m_animWorker->m_notify(result);
    }
    return result;
}

RVA(0x001597b0, 0x57)
CWwdGameObjectA* CDDrawChildGroup::CreateSprite(
    i32 id,
    i32 x,
    i32 y,
    i32 sortKey,
    const char* name,
    i32 stateFlags
) {
    AnimWorkerObj* tmpl = LookupWorker(OwnerMgr()->m_workerCache->m_workers, name);
    if (!tmpl) {
        return 0;
    }

    return CreateSpriteObject(id, x, y, sortKey, tmpl, stateFlags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00159810, 0x1b)
i32 CDDrawChildGroup::AddObject(CGameObject* obj) {
    if (obj == NULL) {
        return 0;
    }
    InsertSorted(obj, 1);
    return 1;
}

RVA(0x00159830, 0x92)
i32 CDDrawChildGroup::AttachSprite(
    CWwdGameObject* obj,
    i32 x,
    i32 y,
    i32 sortKey,
    const char* name,
    i32 stateFlags
) {
    if (!obj) {
        return 0;
    }
    AnimWorkerObj* tmpl = LookupWorker(OwnerMgr()->m_workerCache->m_workers, name);
    if (!tmpl) {
        return 0;
    }
    obj->m_flags = stateFlags;
    if (!obj->Setup(x, y, sortKey, tmpl)) {
        return 0;
    }

    this->InsertSorted(obj, 1);
    if (stateFlags & 0x200000) {

        obj->m_animWorker->m_notify(static_cast<CGameObject*>(obj));
    }
    return 1;
}

RVA(0x001598d0, 0x13d)
CWwdGameObject* CDDrawChildGroup::CreateContainerObject(
    int id,
    int x,
    int y,
    int sortKey,
    AnimWorkerObj* tmpl,
    int stateFlags
) {
    CWwdGameObject* result = new CWwdGameObject(OwnerMgr(), id, stateFlags);
    if (result->Setup(x, y, sortKey, tmpl) == 0) {
        if (result != NULL) {
            delete result;
        }
        return 0;
    }
    InsertSorted(result, 1);
    if (stateFlags & 0x200000) {
        result->m_animWorker->m_notify(result);
    }
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00159a10, 0x57)
CWwdGameObject* CDDrawChildGroup::CreateNamedContainerObject(
    int id,
    int x,
    int y,
    int sortKey,
    const char* name,
    int stateFlags
) {
    AnimWorkerObj* val = LookupWorker(OwnerMgr()->m_workerCache->m_workers, name);
    if (val == NULL) {
        return 0;
    }
    return CreateContainerObject(id, x, y, sortKey, val, stateFlags);
}

// @early-stop
// Both static queues emit real atexit thunks as separate anonymous COMDATs;
// the delinked image names those same bytes as this function's EH span.
RVA(0x00159a70, 0x200)
void CDDrawChildGroup::TickKillCues(i32 advance) {
    RVA_DYNINIT(0x00159c80, 0xa, killQueue)
    DATA(0x002bf3a8)
    static CObArray killQueue;
    RVA_DYNINIT(0x00159c70, 0xa, sortQueue)
    DATA(0x002bf390)
    static CObArray sortQueue;
    killQueue.SetSize(0, -1);
    sortQueue.SetSize(0, -1);

    if (advance != 0) {
        u32 now = timeGetTime();
        u32 delta = now - g_killCueClock;
        g_engineFrameDelta = delta;
        g_killCueClock = now;
    }

    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        AnimWorkerObj* rec = obj->m_animWorker;
        if (rec->Consume(static_cast<i32>(g_engineFrameDelta)) == 0) {
            i32* refc = &rec->m_frameDelay;
            if (*refc != 0) {
                --*refc;
            } else {
                rec->m_notify(static_cast<CGameObject*>(obj));
            }
        }
        i32 flags = obj->m_flags;
        if (flags & 0x10000) {
            killQueue.Add(static_cast<CObject*>(obj));
        } else if (flags & 0x20000) {
            sortQueue.Add(static_cast<CObject*>(obj));
        }
    }

    i32 i;
    for (i = 0; i < killQueue.GetSize(); i++) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(killQueue.GetData()[i]);
        if (obj->m_flags & 0x80000) {
            AnimWorkerObj* rec = obj->m_animWorker;
            rec->SetWorkerAct(ACT_OBJECT_REMOVED);
            rec->m_notify(static_cast<CGameObject*>(obj));
        }
        if (obj->m_flags & 0x800) {
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
        obj->m_flags &= ~0x20000;
        m_list.RemoveAt(obj->m_posCache);
        InsertSorted(obj, 0);
    }
}

RVA(0x00159c90, 0x23)
void CDDrawChildGroup::RenderChildren(CDDrawSurfacePair* target) {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->Render(target);
        } while (n != NULL);
    }
}

RVA(0x00159cc0, 0x2a)
void CDDrawChildGroup::BltDirtyChildren(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {
    POSITION n = m_list.GetHeadPosition();
    if (n != NULL) {
        do {
            CGameObject* cur_obj = NextChild(n);
            cur_obj->BltDirty(dst, src);
        } while (n != NULL);
    }
}

RVA(0x00159cf0, 0x42)
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

RVA(0x00159d40, 0x42)
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

RVA(0x00159d90, 0x1c)
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
RVA(0x00159db0, 0x5e)
void CDDrawChildGroup::RemoveAndDelete(CWwdGameObject* obj) {
    if (obj->m_flags & 0x800) {
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
RVA(0x00159e10, 0x2e)
void CDDrawChildGroup::ReinsertUnflagged(CWwdGameObject* obj) {
    obj->m_flags &= 0xfffdffff;
    m_list.RemoveAt(obj->m_posCache);
    InsertSorted(obj, 0);
}

#define REGISTER_CHILD_OBJECT_ID(obj) m_registeredGameObjectsById[WwdKey(obj)] = obj

RVA(0x00159e40, 0xaa)
void CDDrawChildGroup::InsertSorted(CGameObject* obj, i32 addToMaps) {
    if (obj->m_flags & 0x800) {
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
        if (data->m_sortKey > key && !(data->m_flags & 0x20000)) {
            obj->m_posCache = (m_list.InsertBefore(cur, static_cast<CObject*>(obj)));
            return;
        }
    }
    obj->m_posCache = m_list.AddTail(static_cast<CObject*>(obj));
}

RVA(0x00159ef0, 0x5)
void CDDrawChildGroup::ClearChildren() {
    DestroyChildren();
}

// @early-stop
// Residue is the frame-slot ORDER: retail lays the scalars out this/ip/pos/mask1
// from esp+0, we get mask1/this/ip/pos. Hoisting the mask decls to function scope
// does not move it (measured).
RVA(0x00159f00, 0x22e)
void CDDrawChildGroup::CollideBroadcast() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* oi = NextChild(pos);
        if (!(oi->m_flags & 1)) {
            POSITION ip = pos;
            while (ip != NULL) {
                CGameObject* oj = NextChild(ip);
                i32 fj = oj->m_flags;
                if (fj & 1) {
                    continue;
                }
                i32 fi = oi->m_flags;
                if ((fi ^ fj) & 0x40000) {
                    continue;
                }

                if (!(fi & 4) && !(fj & 0x80)) {
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
                                AnimWorkerObj* nf = oj->m_attackWorker;
                                if (nf != NULL) {
                                    oj->m_attackTarget = oi;

                                    nf->m_notify(oj);
                                }
                            }
                            if (mask1) {
                                if (oi->m_flags & 8) {
                                    i32 v = oi->m_health - oj->m_damage;
                                    oi->m_health = v;
                                    if (v <= 0) {

                                        oi->m_animWorker->SetWorkerAct(ACT_HEALTH_DEPLETED);
                                    }
                                } else {
                                    AnimWorkerObj* nf = oi->m_hitWorker;
                                    if (nf != NULL) {
                                        oi->m_hitSource = oj;
                                        nf->m_notify(oi);
                                    }
                                }
                            }
                        }
                    }
                }

                if (oj->m_flags & 4) {
                    continue;
                }
                if (oi->m_flags & 0x80) {
                    continue;
                }
                i32 mask1b = oj->m_hitTypeFlags & static_cast<i32>(oi->m_objectType);
                i32 mask2b = static_cast<i32>(oj->m_objectType) & oi->m_attackTypeMask;
                if ((mask1b || mask2b) && BoxesOverlap(oj, oi)) {
                    if (mask2b) {
                        AnimWorkerObj* nf = oi->m_attackWorker;
                        if (nf != NULL) {
                            oi->m_attackTarget = oj;
                            nf->m_notify(oi);
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
RVA(0x0015a130, 0xdc)
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

DATA(0x0021ab30)
static char s_dbgRle[] = "RLE";
DATA(0x0021ab2c)
static char s_dbgVid[] = "VID";
DATA(0x0021ab28)
static char s_dbgSys[] = "SYS";

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a210, 0x432)
void CDDrawChildGroup::DrawObjectDebugGeometry() {
    if (m_flags & 0x10000) {
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
                    view->WrapCoord(&rc.left, &rc.top);
                    view->WrapCoord(&rc.right, &rc.bottom);
                    drawHost->DrawBox(&rc, 0xff);
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & 0x20000) {
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
                    view->WrapCoord(&rc.left, &rc.top);
                    view->WrapCoord(&rc.right, &rc.bottom);
                    drawHost->DrawBox(&rc, 0xff);
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & 0x40000) {
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
                    view->WrapCoord(&rc.left, &rc.top);
                    view->WrapCoord(&rc.right, &rc.bottom);
                    drawHost->DrawBox(&rc, 0xff);
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & 0x100000) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        if (pos != NULL) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
                i32 x = obj->m_screenX;
                if (x != COORD_UNSET) {

                    i32 fl = view->m_flags;
                    i32 y = obj->m_screenY;
                    if (fl & 4) {
                        i32 w = view->m_wrapW;
                        if (x < 0) {
                            x = x + w;
                        } else if (x >= w) {
                            x = x - w;
                        }
                        i32 farEdge = view->m_viewRect.right;
                        if (farEdge >= w && x < view->m_viewRect.left && x <= farEdge - w) {
                            x = x + w;
                        }
                    }
                    if (fl & 8) {
                        i32 h = view->m_wrapH;
                        if (y < 0) {
                            y = y + h;
                        } else if (y >= h) {
                            y = y - h;
                        }
                        i32 farEdge = view->m_viewRect.bottom;
                        if (farEdge >= h && y < view->m_viewRect.top && y <= farEdge - h) {
                            y = y + h;
                        }
                    }
                    drawHost->DrawCross(
                        view->m_bounds50.left - view->m_viewRect.left + x,
                        view->m_bounds50.top - view->m_viewRect.top + y
                    );
                }
            } while (pos != NULL);
        }
    }
    if (m_flags & 0x1000000) {
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
                CImage* fr = obj->m_layer;
                if (fr == NULL) {
                    continue;
                }
                i32 x = obj->m_screenX;
                i32 y = obj->m_screenY;
                RECT box;
                SetRect(&box, x - 0x20, y + 8, x + 0x20, y + 0x20);
                RECT rc = box;
                view->WrapCoord(&rc.left, &rc.top);
                view->WrapCoord(&rc.right, &rc.bottom);
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
RVA(0x0015a650, 0x12c)
void CDDrawChildGroup::DrawObjectCounts() {
    if (!(m_flags & 0x200000)) {
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
        i32 fl = view->m_flags;
        if (fl & 4) {
            i32 w = view->m_wrapW;
            if (box.left < 0) {
                wl = box.left + w;
            } else if (box.left >= w) {
                wl = box.left - w;
            }
            i32 farEdge = view->m_viewRect.right;
            if (farEdge >= w && wl < view->m_viewRect.left && wl <= farEdge - w) {
                wl += w;
            }
        }
        if (fl & 8) {
            i32 h = view->m_wrapH;
            if (box.top < 0) {
                wt = box.top + h;
            } else if (box.top >= h) {
                wt = box.top - h;
            }
            i32 farEdge = view->m_viewRect.bottom;
            if (farEdge >= h && wt < view->m_viewRect.top && wt <= farEdge - h) {
                wt += h;
            }
        }
        rc.left = wl - view->m_viewRect.left + view->m_bounds50.left;
        rc.top = wt - view->m_viewRect.top + view->m_bounds50.top;

        view->WrapCoord(&rc.right, &rc.bottom);
        drawHost->DrawCount(&rc, obj->m_sortKey);
    } while (pos != NULL);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a780, 0x70)
i32 CDDrawChildGroup::CheckSortOrder() {
    POSITION node = m_list.GetHeadPosition();
    CWwdGameObject* anchor = static_cast<CWwdGameObject*>(NextChild(node));
    if (anchor != NULL) {
        while (node != NULL && anchor != NULL && (anchor->m_flags & 0x20000) != 0) {
            anchor = static_cast<CWwdGameObject*>(NextChild(node));
        }
        if (anchor != NULL) {
            i32 key = anchor->m_sortKey;
            while (node != NULL) {
                CGameObject* cur_obj = NextChild(node);
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
                if ((obj->m_flags & 0x20000) == 0) {
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
RVA(0x0015a7f0, 0x20)
CWwdGameObject* CDDrawChildGroup::FindByType04(i32 type) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_id == type) {
            return obj;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a810, 0x42)
CWwdGameObject* CDDrawChildGroup::FindByTypeProbe(i32 type) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == type) {
            return obj;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a860, 0x57)
CWwdGameObject* CDDrawChildGroup::FindByWorker(i32 type, AnimWorkerObj* key) {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == type) {

            AnimWorkerObj* worker = obj->m_animWorker;
            if (worker->m_notify == key->m_notify) {
                return obj;
            }
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @early-stop
RVA(0x0015a8c0, 0x7d)
CGameObject* CDDrawChildGroup::Find(i32 id, const char* key) {
    AnimWorkerObj* fp = LookupWorker(OwnerMgr()->m_workerCache->m_workers, key);
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = NextChild(pos);
        LoadableClassId tag = obj->GetClassId();
        if (tag == CLASSID_WWDOBJA && obj->m_id == id
            && obj->m_animWorker->m_notify == fp->m_notify) {
            return obj;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a940, 0x52)
CWwdGameObject* CDDrawChildGroup::FindByIdAndCollisionCategory(i32 id, u32 collisionCategory) {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));

        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id
            && obj->m_objectType == collisionCategory) {
            return obj;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a9a0, 0x23)
CWwdGameObject* CDDrawChildGroup::FindByObjectId(i32 objectId) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->m_objectId == objectId) {
            return obj;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015a9d0, 0x45)
CWwdGameObject* CDDrawChildGroup::FindSerialRefByObjectId(i32 objectId) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_objectId == objectId) {
            return obj;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015aa20, 0x3c)
i32 CDDrawChildGroup::IsKindUnique(i32 kind) {
    CWwdGameObject* found = 0;
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
RVA(0x0015aa60, 0x23)
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

RVA(0x0015aa90, 0x5d)
void CDDrawChildGroup::PruneList() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(NextChild(pos));
        if (obj != NULL && !(obj->m_flags & 0x200)) {
            m_list.RemoveAt(cur);
            m_activeGameObjectsById.RemoveKey(WwdKey(obj));
            m_registeredGameObjectsById.RemoveKey(WwdKey(obj));
            delete obj;
        }
    }
}

// @early-stop
// cl canonicalises the four-term sum's operand order from the operands: source
// order, every parenthesization, per-term statement splits and the distributed
// i*a+i*b+... all emit the identical order. The TU-state probe does NOT flip it.
// docs/patterns/commutative-operand-order-is-canonical.md
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015aaf0, 0x35)
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

RVA(0x0015ab30, 0x38)
void CDDrawChildGroup::RemoveAll(POSITION pos, CGameObject* obj) {
    REMOVE_ACTIVE_OBJECT_AT(pos, obj);
    m_registeredGameObjectsById.RemoveKey(WwdKey(obj));
}

RVA(0x0015ab70, 0x27)
void CDDrawChildGroup::RemoveByPosition(POSITION pos, CGameObject* obj) {
    REMOVE_ACTIVE_OBJECT_AT(pos, obj);
}

RVA(0x0015aba0, 0x1a)
void CDDrawChildGroup::RegisterObjectId(CWwdGameObject* obj) {
    REGISTER_CHILD_OBJECT_ID(obj);
}

RVA(0x0015abc0, 0x5e)
i32 CDDrawChildGroup::CountActive() {
    i32 n = 0;
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = NULL;
            CWwdGameObject* val = NULL;
            MapGetNext(m_registeredGameObjectsById, pos, key, val);
            if (val != NULL && !(val->m_flags & WAPOBJ_FLAG_SKIP_ACTIVE_PASSES)) {
                ++n;
            }
        } while (pos != NULL);
    }
    return n;
}

RVA(0x0015ac20, 0x81)
i32 CDDrawChildGroup::ForEachDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = NULL;
            CWwdGameObject* val = NULL;
            MapGetNext(m_registeredGameObjectsById, pos, key, val);
            if (val != NULL && !(val->m_flags & WAPOBJ_FLAG_SKIP_ACTIVE_PASSES)) {
                val->Play(ar, mode, typeId, val);
            }
        } while (pos != NULL);
    }
    return 1;
}

RVA(0x0015acb0, 0x76)
i32 CDDrawChildGroup::ForEachProbe(CFileMemBase* ar, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = NULL;
            CWwdGameObject* val = NULL;
            MapGetNext(m_registeredGameObjectsById, pos, key, val);
            if (val != NULL && !(val->m_flags & WAPOBJ_FLAG_SKIP_ACTIVE_PASSES)) {

                val->WriteSnapshot(ar, typeId);
            }
        } while (pos != NULL);
    }
    return 1;
}

RVA(0x0015ad30, 0x2ec)
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
            case CLASSID_WWDOBJF: {
                i32 sortKey = desc.m_sortKey;
                i32 id = desc.m_id;
                createdObj = CreateDeferredObject(
                    id,
                    sortKey,
                    LookupWorker(OwnerMgr()->m_workerCache->m_workers, desc.m_workerName),
                    0
                );
                break;
            }
            case CLASSID_WWDOBJA: {
                i32 sortKey = desc.m_sortKey;
                i32 y = desc.m_screenY;
                i32 x = desc.m_screenX;
                i32 id = desc.m_id;
                AnimWorkerObj* worker =
                    LookupWorker(OwnerMgr()->m_workerCache->m_workers, desc.m_workerName);
                if (worker == NULL) {
                    createdObj = NULL;
                } else {
                    createdObj = CreateSpriteObject(id, x, y, sortKey, worker, 0);
                }
                break;
            }
            case CLASSID_WWDOBJB: {
                i32 sortKey = desc.m_sortKey;
                i32 y = desc.m_screenY;
                i32 x = desc.m_screenX;
                i32 id = desc.m_id;
                AnimWorkerObj* worker =
                    LookupWorker(OwnerMgr()->m_workerCache->m_workers, desc.m_workerName);
                if (worker == NULL) {
                    createdObj = NULL;
                } else {
                    createdObj = CreateContainerObject(id, x, y, sortKey, worker, 0);
                }
                break;
            }
            case CLASSID_CALLBACKOBJ: {

                CWwdGameObject* rec = NULL;
                // m_serialTypeId is NOT a LogicTypeId: this phase keys off the
                // record's own serial type id.
                if (OwnerMgr()->InvokeCallback(
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
                        desc.m_workerName,
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
        if (createdObj->m_animWorker == NULL) {
            return 0;
        }
        if (desc.m_logicTypeId != LOGIC_UNSET) {

            CUserLogic* child = NULL;
            if (OwnerMgr()->InvokeCallback(
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

            createdObj->m_animWorker->m_logic = child;
        }
    }
    return 1;
}

RVA(0x0015b020, 0xc0)
i32 CDDrawChildGroup::ForEachSerialize(CFileMemBase* ar, LogicTypeId typeId) {
    if (ar == NULL) {
        return 0;
    }
    POSITION pos = m_registeredGameObjectsById.GetStartPosition();
    while (pos != NULL) {
        void* key = NULL;
        CWwdGameObject* val = NULL;
        MapGetNext(m_registeredGameObjectsById, pos, key, val);
        if (val != NULL && !(val->m_flags & WAPOBJ_FLAG_SKIP_ACTIVE_PASSES)) {
            i32 objectId = val->m_objectId;
            ar->Write(&objectId, sizeof(objectId));
            if (val->Play(ar, SERIAL_SAVE, typeId, val) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x0015b0e0, 0xec)
i32 CDDrawChildGroup::Deserialize(CFileMemBase* ar, u32 count, LogicTypeId flag) {
    if (ar == NULL) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        i32 objectId = 0;
        ar->Read(&objectId, sizeof(objectId));
        if (objectId == 0) {
            return 0;
        }
        CWwdGameObject* found = NULL;
        CWwdGameObject* obj = NULL;
        if (MapLookupById(m_registeredGameObjectsById, objectId, found)) {
            obj = found;
        }
        if (obj == NULL) {
            return 0;
        }
        if (obj->m_animWorker == NULL) {
            return 0;
        }
        // Release-dead TRACE (`1 ? (void)0 : ::AfxTrace`). Only "one CString
        // temporary lived here" is byte-proven; the arm itself is gone.
        if ((flag & 1) != 0) {
            TRACE("%s\n", static_cast<LPCTSTR>(CString(obj->m_name)));
        }
        if (obj->Play(ar, SERIAL_LOAD, flag, obj) == 0) {
            return 0;
        }
    }
    return 1;
}

inline CWwdGameObject* LookupActiveObject(CMapPtrToPtr& map, void* key) {
    CWwdGameObject* found = NULL;
    if (MapLookup(map, key, found) == 0) {
        found = NULL;
    }
    return found;
}

RVA(0x0015b1d0, 0x9b)
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

RVA(0x0015b270, 0x11)
WwdDirtyRect::WwdDirtyRect() {
    m_rect.left = COORD_UNSET;
    m_armed = -1;
}

// The header's `~WwdDirtyRect() {}` COMDAT, and where retail put it. cl inlines
// the empty body away at every call site (retail's ~CGameObject has no call to
// it) but must still give it an ADDRESS, because the /GX unwind funclet for the
// member at +0xb8 jumps there. Without the pin that funclet's target is an
// unnamed byte and the reference cannot be compared at all.

RVA(0x0015b2a0, 0xb)
WwdGridNode::WwdGridNode() {
    m_bucket = NULL;
    m_reserved08 = 0;
}

RVA(0x0015b2b0, 0xe)
WwdRegion::WwdRegion() : WwdGridNode(WwdGridNode::NO_SEED) {
    SeedFields();
}

// The three creators above call both three-argument ctors; these are the
// out-of-line homes.  CGameObject's 0x15b390 (WwdFactoryObject.cpp) expands
// CResolveNode's seed via the tagged inline sibling and AnimWorkerObj's via
// the opt-in <DDrawMgr/AnimWorkerObjCtorInline.h> view (see the docs/patterns/comdat-home-adjudicates-inline-spelling.md
// dossier: the creators' budget slices refute a single visible body for both).
RVA(0x0015b2c0, 0x3d)
CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CWapObj(owner, field04, field08, CWapObj::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

RVA(0x0015b300, 0x40)
AnimWorkerObj::AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CWapObj(owner, id, stateFlags, CWapObj::NO_SEED) {
    ResetWorkerFields();
}

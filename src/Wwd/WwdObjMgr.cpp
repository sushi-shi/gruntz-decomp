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
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ObList.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Io/FileMem.h>
#include <PlacementNew.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/AnimWorkerAct.h>
#include <Wwd/WwdFactoryObject.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>

DATA(0x0021ab14)
i32 g_wwdObjIdCounter = 1;

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
    m_map2c.RemoveAll();
    m_map48.RemoveAll();
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

// @early-stop
// docs/patterns/outparam-zeroinit-scheduling.md: retail sinks the `val = 0`
// store past BOTH Lookup arg pushes; eight source spellings re-measured here,
// all byte-identical.
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
    CObject* val = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(name, val);

    return CreateDotObject(
        id,
        x,
        y,
        sortKey,
        static_cast<AnimWorkerObj*>(val),
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

// @early-stop
// docs/patterns/outparam-zeroinit-scheduling.md: retail sinks the `val = 0`
// store past BOTH Lookup arg pushes; eight source spellings re-measured here,
// all byte-identical.
RVA(0x001595b0, 0x44)
CWwdGameObjectF*
CDDrawChildGroup::CreateNamedDeferredObject(int id, int sortKey, const char* name, int stateFlags) {
    CObject* val = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(name, val);
    return CreateDeferredObject(id, sortKey, static_cast<AnimWorkerObj*>(val), stateFlags);
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
    CObject* tmpl_ob = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(name, tmpl_ob);

    AnimWorkerObj* tmpl = static_cast<AnimWorkerObj*>(tmpl_ob);
    if (!tmpl) {
        return 0;
    }

    return CreateSpriteObject(id, x, y, sortKey, tmpl, stateFlags);
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
    CObject* tmpl_ob = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(name, tmpl_ob);

    AnimWorkerObj* tmpl = static_cast<AnimWorkerObj*>(tmpl_ob);
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

// @early-stop
// CAniAdvanceCursor's ctor is expanded here (INLINE_CURSOR), matching retail, but
// our cl then also expands the CLoadable base ctor inside it where retail keeps
// `call ??0CLoadable` - a depth-4 inline decision; #pragma inline_depth(2|3) is
// ignored by cl 5.0, so there is no source lever for it yet.
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

// @early-stop
// docs/patterns/outparam-zeroinit-scheduling.md: retail sinks the `val = 0`
// store past BOTH Lookup arg pushes; eight source spellings re-measured here,
// all byte-identical.
RVA(0x00159a10, 0x57)
CWwdGameObject* CDDrawChildGroup::CreateNamedContainerObject(
    int id,
    int x,
    int y,
    int sortKey,
    const char* name,
    int stateFlags
) {
    CObject* val = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(name, val);
    if (val == NULL) {
        return 0;
    }
    return CreateContainerObject(id, x, y, sortKey, static_cast<AnimWorkerObj*>(val), stateFlags);
}

RVA(0x00159a70, 0x200)
void CDDrawChildGroup::TickKillCues(i32 advance) {
    static CObArray killQueue;
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
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
            m_map48.RemoveKey(WwdKey(obj));
            m_map2c.RemoveKey(WwdKey(obj));
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
    CDDrawSurfacePair* dst,
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

RVA(0x00159db0, 0x5e)
void CDDrawChildGroup::RemoveAndDelete(CWwdGameObject* obj) {
    if (obj->m_flags & 0x800) {
        delete obj;
        return;
    }
    m_list.RemoveAt(obj->m_posCache);
    m_map48.RemoveKey(WwdKey(obj));
    m_map2c.RemoveKey(WwdKey(obj));
    delete obj;
}

RVA(0x00159e10, 0x2e)
void CDDrawChildGroup::ReinsertUnflagged(CWwdGameObject* obj) {
    obj->m_flags &= 0xfffdffff;
    m_list.RemoveAt(obj->m_posCache);
    InsertSorted(obj, 0);
}

RVA(0x00159e40, 0xaa)
void CDDrawChildGroup::InsertSorted(CGameObject* obj, i32 addToMaps) {
    if (obj->m_flags & 0x800) {
        obj->m_posCache = NULL;
        return;
    }
    if (addToMaps != 0) {
        m_map2c[WwdKey(obj)] = obj;
        m_map48[WwdKey(obj)] = obj;
    }
    POSITION pos = m_list.GetHeadPosition();
    i32 key = obj->m_sortKey;
    while (pos != NULL) {
        POSITION cur = pos;
        CWwdGameObject* data = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
        CGameObject* oi = static_cast<CGameObject*>(m_list.GetNext(pos));
        if (!(oi->m_flags & 1)) {
            POSITION ip = pos;
            while (ip != NULL) {
                CGameObject* oj = static_cast<CGameObject*>(m_list.GetNext(ip));
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
DATA(0x0020bdc4)
static char s_dbgNoCaps[] = "???";

DATA(0x002bf388)
u8 g_val_2bf388;
DATA(0x002bf394)
i32 g_val_2bf394;
DATA(0x002bf398)
i32 g_val_2bf398;
DATA(0x002bf3ac)
i32 g_val_2bf3ac;
DATA(0x002bf3b0)
i32 g_val_2bf3b0;

// @early-stop
RVA(0x0015a210, 0x432)
void CDDrawChildGroup::DrawObjectDebugGeometry() {
    if (m_flags & 0x10000) {
        POSITION pos = m_list.GetHeadPosition();
        CDDrawWorkerHost* view = OwnerMgr()->m_level->m_mainPlane;
        CDDrawSurfacePair* drawHost = OwnerMgr()->m_drawTarget->m_backPair;
        if (pos != NULL) {
            do {
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
                CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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
                RECT rc;
                rc.left = box.left;
                rc.top = box.top;
                rc.right = box.right;
                rc.bottom = box.bottom;
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
                            drawHost->DrawLabel(&rc, s_dbgNoCaps);
                        }
                    }
                }
            } while (pos != NULL);
        }
    }
}

// @early-stop
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
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
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

RVA(0x0015a860, 0x57)
CWwdGameObject* CDDrawChildGroup::FindByWorker(i32 type, void* key) {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == type) {

            AnimWorkerObj* worker = obj->m_animWorker;
            if (worker->m_notify == (static_cast<AnimWorkerObj*>(key))->m_notify) {
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
void* CDDrawChildGroup::Find(i32 id, const char* key) {
    CObject* found = 0;
    OwnerMgr()->m_workerCache->m_workers.Lookup(key, found);
    AnimWorkerObj* fp = static_cast<AnimWorkerObj*>(found);
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(m_list.GetNext(pos));
        LoadableClassId tag = obj->GetClassId();
        if (tag == CLASSID_WWDOBJA && obj->m_id == id
            && obj->m_animWorker->m_notify == fp->m_notify) {
            return obj;
        }
    }
    return 0;
}

RVA(0x0015a940, 0x52)
CWwdGameObject* CDDrawChildGroup::FindByIdAndCollisionCategory(i32 id, u32 collisionCategory) {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));

        if (obj->GetClassId() == CLASSID_SERIALREF && obj->m_id == id
            && obj->m_objectType == collisionCategory) {
            return obj;
        }
    }
    return 0;
}

RVA(0x0015a9a0, 0x23)
CWwdGameObject* CDDrawChildGroup::FindByKey(void* key) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (WwdKey(obj) == key) {
            return obj;
        }
    }
    return 0;
}

RVA(0x0015a9d0, 0x45)
CWwdGameObject* CDDrawChildGroup::FindByStatusKey(void* key) {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CGameObject* cur_obj = NextChild(node);
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(cur_obj);
        if (obj->GetClassId() == CLASSID_SERIALREF && WwdKey(obj) == key) {
            return obj;
        }
    }
    return 0;
}

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
        CWwdGameObject* obj = static_cast<CWwdGameObject*>(m_list.GetNext(pos));
        if (obj != NULL && !(obj->m_flags & 0x200)) {
            m_list.RemoveAt(cur);
            m_map2c.RemoveKey(WwdKey(obj));
            m_map48.RemoveKey(WwdKey(obj));
            delete obj;
        }
    }
}

// @early-stop
// cl canonicalises the four-term sum's operand order from the operands: source
// order, every parenthesization, per-term statement splits and the distributed
// i*a+i*b+... all emit the identical order. The TU-state probe does NOT flip it.
// docs/patterns/commutative-operand-order-is-canonical.md
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

RVA(0x0015ab30, 0x38)
void CDDrawChildGroup::RemoveAll(POSITION pos, CGameObject* obj) {
    m_list.RemoveAt(pos);
    m_map2c.RemoveKey(WwdKey(obj));
    m_map48.RemoveKey(WwdKey(obj));
}

RVA(0x0015ab70, 0x27)
void CDDrawChildGroup::RemoveByPosition(POSITION pos, CGameObject* obj) {
    m_list.RemoveAt(pos);
    m_map2c.RemoveKey(WwdKey(obj));
}

RVA(0x0015aba0, 0x1a)
void CDDrawChildGroup::AddToMap48(CWwdGameObject* obj) {
    m_map48[WwdKey(obj)] = obj;
}

RVA(0x0015abc0, 0x5e)
i32 CDDrawChildGroup::CountActive() {
    i32 n = 0;
    POSITION pos = m_map48.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            MapGetNext(m_map48, pos, key, val);
            if (val != NULL && !(val->m_flags & 0x4000000)) {
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
    POSITION pos = m_map48.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            MapGetNext(m_map48, pos, key, val);
            if (val != NULL && !(val->m_flags & 0x4000000)) {
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
    POSITION pos = m_map48.GetStartPosition();
    if (pos != NULL) {
        do {
            void* key = 0;
            CWwdGameObject* val = 0;
            MapGetNext(m_map48, pos, key, val);
            if (val != NULL && !(val->m_flags & 0x4000000)) {

                val->WriteSnapshot(ar, typeId);
            }
        } while (pos != NULL);
    }
    return 1;
}

// @early-stop
// Retail caches `reader` in ebx and uses ebp as the function's zero register; cl
// swaps them and therefore reloads `reader` from its parameter home every
// iteration. Size is exact. 13 body spellings measured; `createdObj` left
// uninitialised with an explicit zero in every non-creating arm was worth 71.0 -> 75.3.
RVA(0x0015ad30, 0x2ec)
i32 CDDrawChildGroup::LoadObjects(class CFileMemBase* reader, u32 count, LogicTypeId unused) {
    i32 savedCounter = 0;
    if (reader == NULL) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        WwdSnapshot desc;
        reader->Read(&desc, sizeof(desc));

        void* found;
        if (MapLookupById(m_map48, desc.m_objectId, found) && found != NULL) {
            return 0;
        }

        savedCounter = g_wwdObjIdCounter;
        g_wwdObjIdCounter = desc.m_objectId;

        CGameObject* createdObj;
        switch (desc.m_classId) {
            case CLASSID_WWDOBJA: {
                CObject* val;
                OwnerMgr()->m_workerCache->m_workers.Lookup(
                    static_cast<const char*>(desc.m_workerName),
                    val
                );
                if (val != NULL) {
                    createdObj = CreateSpriteObject(
                        desc.m_id,
                        desc.m_screenX,
                        desc.m_screenY,
                        desc.m_sortKey,
                        static_cast<AnimWorkerObj*>(val),
                        0
                    );
                } else {
                    createdObj = NULL;
                }
                break;
            }
            case CLASSID_WWDOBJF: {
                CObject* val;
                OwnerMgr()->m_workerCache->m_workers.Lookup(
                    static_cast<const char*>(desc.m_workerName),
                    val
                );
                createdObj = CreateDeferredObject(
                    desc.m_id,
                    desc.m_sortKey,
                    static_cast<AnimWorkerObj*>(val),
                    0
                );
                break;
            }
            case CLASSID_WWDOBJB: {
                CObject* val;
                OwnerMgr()->m_workerCache->m_workers.Lookup(
                    static_cast<const char*>(desc.m_workerName),
                    val
                );
                if (val != NULL) {
                    createdObj = CreateContainerObject(
                        desc.m_id,
                        desc.m_screenX,
                        desc.m_screenY,
                        desc.m_sortKey,
                        static_cast<AnimWorkerObj*>(val),
                        0
                    );
                } else {
                    createdObj = NULL;
                }
                break;
            }
            case CLASSID_CALLBACKOBJ: {

                void* out = 0;
                // m_serialTypeId is NOT a LogicTypeId: this phase keys off the
                // record's own serial type id, so the callback's type-id parameter
                // carries two domains depending on the phase. Recorded, not merged.
                if (OwnerMgr()->InvokeCallback(
                        reader,
                        SERIAL_CREATE_BY_SERIAL_ID,
                        static_cast<LogicTypeId>(desc.m_serialTypeId),
                        &out
                    )
                    == 0) {
                    return 0;
                }
                CWwdGameObject* rec = static_cast<CWwdGameObject*>(out);
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
                createdObj = NULL;
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

            void* childOut = 0;
            if (OwnerMgr()->InvokeCallback(reader, SERIAL_CREATE, desc.m_logicTypeId, &childOut)
                == 0) {
                return 0;
            }
            CUserLogic* child = static_cast<CUserLogic*>(childOut);
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
    POSITION pos = m_map48.GetStartPosition();
    while (pos != NULL) {
        void* key = 0;
        CWwdGameObject* val = 0;
        MapGetNext(m_map48, pos, key, val);
        if (val != NULL && !(val->m_flags & 0x4000000)) {
            void* k = WwdKey(val);
            ar->Write(&k, sizeof(k));
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
        void* key = 0;
        ar->Read(&key, sizeof(key));
        if (key == NULL) {
            return 0;
        }
        CWwdGameObject* found = NULL;
        CWwdGameObject* obj = NULL;
        if (MapLookup(m_map48, key, found)) {
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

// @early-stop
RVA(0x0015b1d0, 0x9b)
i32 CDDrawChildGroup::PruneOrphans() {
    i32 n = 0;
    POSITION pos = m_map48.GetStartPosition();
    while (pos != NULL) {
        void* key = 0;
        CWwdGameObject* val = 0;
        MapGetNext(m_map48, pos, key, val);
        if (val != NULL) {

            void* found = 0;
            if (m_map2c.Lookup(WwdKey(val), found) == 0 || found == NULL) {
                m_map48.RemoveKey(WwdKey(val));
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

RVA(0x0015b2a0, 0xb)
WwdGridNode::WwdGridNode() {
    m_bucket = NULL;
    m_reserved08 = 0;
}

// The out-of-line half of the visibility split: the three creators above carry
// CGameObject's ctor body expanded but still `call` these two.  The TU that expands
// them (WwdFactoryObject.cpp, inside 0x15b390) takes the *CtorInline.h views instead.
RVA(0x0015b2c0, 0x3d)
CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CLoadable(owner, field04, field08, CLoadable::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

RVA(0x0015b300, 0x40)
AnimWorkerObj::AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CLoadable(owner, id, stateFlags, CLoadable::NO_SEED) {
    ResetWorkerFields();
}

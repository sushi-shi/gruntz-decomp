#include <rva.h>

#include <Gruntz/FrontCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/BehindCandy.h>
#include <Gruntz/DoNothing.h>
#include <Gruntz/EyeCandy.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FrontCandy.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000ad110, 0xa, CActRegPool<CFrontCandyAni>::s_table)
RVA_DYNINIT(0x000ad130, 0x15, CActRegPool<CFrontCandyAni>::s_table)
RVA_DYNINIT(0x000ad160, 0xe, CActRegPool<CFrontCandyAni>::s_table)
RVA_DYNINIT(0x000ad180, 0x1f, CActRegPool<CFrontCandyAni>::s_table)
template<> DATA(0x002460b0)
CActReg CActRegPool<CFrontCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0000fa60, 0x47)
i32 CFrontCandy::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000fad0, 0x1e, ??_GCFrontCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fb00, 0x44, ??1CFrontCandy@@UAE@XZ)

RVA(0x0000fdf0, 0x47)
i32 CFrontCandyAni::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000fe60, 0x1e, ??_GCFrontCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fe90, 0x44, ??1CFrontCandyAni@@UAE@XZ)

RVA(0x0000ff20, 0x47)
i32 CEyeCandyAni::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000ff90, 0x1e, ??_GCEyeCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x0000ffc0, 0x44, ??1CEyeCandyAni@@UAE@XZ)

RVA(0x000abfa0, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_OVERLAY) {
        o->m_sortKey = SORTKEY_OVERLAY;
        o->m_flags |= 0x20000;
    }
    CImage* aux = m_object->m_layer;
    if (aux != NULL) {
        i32 bigW = aux->m_width;
        i32 bigH;
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigH = m_object->m_layer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

// @early-stop
// @early-stop
RVA(0x000ac1d0, 0x1a5)
CDoNothing::CDoNothing(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(1);
    CImage* aux = m_object->m_layer;
    if (aux != NULL) {
        i32 bigW = aux->m_width;
        i32 bigH;
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigH = m_object->m_layer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000ac3f0, 0x1b1)
CBehindCandy::CBehindCandy(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0) {
        o->m_sortKey = 0;
        o->m_flags |= 0x20000;
    }
    if (m_object->m_layer != NULL) {
        i32 bigW = m_object->m_layer->m_width;
        i32 bigH;
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigH = m_object->m_layer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

// cl5 propagates the branch equality into the guarded re-read of the member it
// just tested, so the load retail keeps is missing.
// docs/patterns/branch-equality-propagated-into-the-guarded-store.md
RVA(0x000ac620, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey == 0 && o->m_layer != NULL) {
        i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
        if (o->m_sortKey != v) {
            o->m_sortKey = v;
            o->m_flags |= 0x20000;
        }
    }
    CImage* aux = m_object->m_layer;
    if (aux != NULL) {
        i32 bigW = aux->m_width;
        i32 bigH;
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigH = m_object->m_layer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000ac870, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    if (m_wwdObject->m_animCursor.m_animation == NULL) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey == 0 && o->m_layer != NULL) {
        i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
        if (o->m_sortKey != v) {
            o->m_sortKey = v;
            o->m_flags |= 0x20000;
        }
    }
    CImage* aux = m_object->m_layer;
    if (aux != NULL) {
        i32 bigW = aux->m_width;
        i32 bigH;
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigH = m_object->m_layer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000acbb0, 0x102)
void CEyeCandyAni::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CEyeCandyAni>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CEyeCandyAni>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000acd10, 0x18d)
void CEyeCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CEyeCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CEyeCandyAni::AdvanceAnim);
}

RVA(0x000acf10, 0x17)
i32 CEyeCandyAni::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

// @early-stop
// Residue is one shared idiom (see
// docs/patterns/known-zero-reload-before-call.md): retail RE-READS
// m_wwdObject->m_animCursor.m_animation through the call's `this` even though the
// guard just proved it 0, while cl here copy-propagates the guard's zero register
// into the store. No source spelling reaches retail's form - 7 spellings tested
// (plain, wwdObject local, cursor-pointer local, mixed receivers, both statement
// orders); every assign-BEFORE-call form also costs an extra zero-constant use,
// which makes cl claim a 4th callee-saved register and shifts every frame offset.
RVA(0x000acf40, 0x16e)
CFrontCandyAni::CFrontCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    if (m_wwdObject->m_animCursor.m_animation == NULL) {
        m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
        m_value = m_wwdObject->m_animCursor.m_animation;
    }
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_OVERLAY) {
        o->m_sortKey = SORTKEY_OVERLAY;
        o->m_flags |= 0x20000;
    }
}

RVA(0x000ad1b0, 0x102)
void CFrontCandyAni::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x000ad310, 0x18d)
void CFrontCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CFrontCandyAni::AdvanceAnim);
}

RVA(0x000ad510, 0x17)
i32 CFrontCandyAni::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

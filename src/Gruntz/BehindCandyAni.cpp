#include <rva.h>

#include <Gruntz/BehindCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000ad7b0, 0xa, int)
RVA_DYNINIT(0x000ad7d0, 0x15, int)
RVA_DYNINIT(0x000ad800, 0xe, int)
RVA_DYNINIT(0x000ad820, 0x1f, int)
template<> DATA(0x00245f98)
CActReg CActRegPool<CBehindCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x000100c0, 0x1e, ??_GCBehindCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x000100f0, 0x44, ??1CBehindCandyAni@@UAE@XZ)

// @early-stop
// Residue is one shared idiom (see
// docs/patterns/known-zero-reload-before-call.md): retail RE-READS
// m_wwdObject->m_animCursor.m_animation through the call's `this` even though the
// guard just proved it 0, while cl here copy-propagates the guard's zero register
// into the store. No source spelling reaches retail's form - 7 spellings tested
// (plain, wwdObject local, cursor-pointer local, mixed receivers, both statement
// orders); every assign-BEFORE-call form also costs an extra zero-constant use,
// which makes cl claim a 4th callee-saved register and shifts every frame offset.
RVA(0x000ad540, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    if (m_wwdObject->m_animCursor.m_animation == NULL) {
        m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
        m_value = m_wwdObject->m_animCursor.m_animation;
    }
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0) {
        o->m_sortKey = 0;
        o->m_flags |= 0x20000;
    }
    CImage* aux = m_object->m_layer;
    if (aux != NULL) {
        i32 bigW = aux->m_width;
        i32 bigH;
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigH = aux->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000ad850, 0x102)
void CBehindCandyAni::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000ad9b0, 0x18d)
void CBehindCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBehindCandyAni::AdvanceAnim);
}

RVA(0x000adbb0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

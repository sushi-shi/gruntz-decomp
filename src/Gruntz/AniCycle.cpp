#include <rva.h>

#include <Gruntz/AniCycle.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

VTBL(CAniCycle, 0x001e86a4);
template<> DATA(0x00246088)
CActReg CActRegPool<CAniCycle>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x0000f470, 0x47)
i32 CAniCycle::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000f4e0, 0x1e, ??_GCAniCycle@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f510, 0x44, ??1CAniCycle@@UAE@XZ)

RVA(0x000aad20, 0x15c)
CAniCycle::CAniCycle(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 1;
    if (m_wwdObject->m_animCursor.m_animation == NULL) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
}

RVA(0x000aaf80, 0x102)
void CAniCycle::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CAniCycle>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CAniCycle>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000ab0e0, 0x18d)
void CAniCycle::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != NULL) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CAniCycle>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CAniCycle::AdvanceAnim);
}

RVA(0x000ab2e0, 0x17)
i32 CAniCycle::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

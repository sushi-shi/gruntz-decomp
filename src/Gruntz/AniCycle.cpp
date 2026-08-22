#include <rva.h>

#include <Gruntz/AniCycle.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000aaee0, 0xa, CActRegPool<CAniCycle>::s_table)
RVA_DYNINIT(0x000aaf00, 0x15, CActRegPool<CAniCycle>::s_table)
RVA_DYNINIT(0x000aaf30, 0xe, CActRegPool<CAniCycle>::s_table)
RVA_DYNINIT(0x000aaf50, 0x1f, CActRegPool<CAniCycle>::s_table)
template<> DATA(0x00246088)
CActReg CActRegPool<CAniCycle>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x0000f4e0, 0x1e, ??_GCAniCycle@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f510, 0x44, ??1CAniCycle@@UAE@XZ)

// @early-stop
// cl5 propagates the branch equality into the guarded re-read of the member it
// just tested, so the load retail keeps is missing.
// docs/patterns/branch-equality-propagated-into-the-guarded-store.md
RVA(0x000aad20, 0x15c)
CAniCycle::CAniCycle(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(1);
    if (m_wwdObject->m_animCursor.m_animation == NULL) {
        SwitchGeometry("GAME_CYCLE100", 0);
    }
    SET_ANIMATION_ACT("A");
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
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CAniCycle>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CAniCycle::AdvanceAnim);
}

RVA(0x000ab2e0, 0x17)
i32 CAniCycle::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

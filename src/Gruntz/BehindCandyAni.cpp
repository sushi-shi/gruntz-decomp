#include <rva.h>

#include <Gruntz/BehindCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000ad7a0, 0xa, CActRegPool<CBehindCandyAni>::s_table)
RVA_DYNINIT(0x000ad7c0, 0x15, CActRegPool<CBehindCandyAni>::s_table)
RVA_DYNINIT(0x000ad7f0, 0xe, CActRegPool<CBehindCandyAni>::s_table)
RVA_DYNINIT(0x000ad810, 0x1f, CActRegPool<CBehindCandyAni>::s_table)
template<> DATA(0x00246ef0)
CActReg CActRegPool<CBehindCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x000100d0, 0x1e, ??_GCBehindCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x00010100, 0x44, ??1CBehindCandyAni@@UAE@XZ)

// @early-stop
RVA(0x000ad530, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    INITIALIZE_DEFAULT_CYCLE_ANIMATION
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, 0)
    NORMALIZE_BIG_ANIMATION_WITH_AUX(aux)
}

RVA(0x000ad840, 0x102)
void CBehindCandyAni::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000ad9a0, 0x18d)
void CBehindCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CBehindCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBehindCandyAni::AdvanceAnim);
}

RVA(0x000adba0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

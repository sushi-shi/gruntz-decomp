#include <rva.h>

#include <Gruntz/SingleAnimation.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

VTBL(CSingleAnimation, 0x001e745c);
template<> DATA(0x00245f70)
CActReg CActRegPool<CSingleAnimation>::s_table(2000, 2010);

RVA_COMPGEN(0x00010510, 0x1e, ??_GCSingleAnimation@@UAEPAXI@Z)
RVA_COMPGEN(0x00010540, 0x44, ??1CSingleAnimation@@UAE@XZ)

// @early-stop
RVA(0x000ae7f0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
}

// @interleaver FireActivation - fixed-size generated body (258 B, byte-identical across
// 51 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000aea20, 0x102)
void CSingleAnimation::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)))))();
    }
}

// @interleaver RegisterActs - fixed-size generated body (397 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000aeb80, 0x18d)
void CSingleAnimation::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleAnimation::AdvanceAnim);
}

// @interleaver AdvanceAnim - 57 B lone body at 0xaed80, between RegisterActs
// (singleanimation) and _CreateRollingBall (logicworkerhandlersb): a first-use placement.
RVA(0x000aed80, 0x39)
i32 CSingleAnimation::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (m_wwdObject->m_animCursor.m_finished != 0
        && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

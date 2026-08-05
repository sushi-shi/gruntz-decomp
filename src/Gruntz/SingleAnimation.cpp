#include <rva.h>

#include <Gruntz/SingleAnimation.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

VTBL(CSingleAnimation, 0x001e745c);
template<> DATA(0x00245f70)
CActReg CActRegPool<CSingleAnimation>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x000104a0, 0x47)
i32 CSingleAnimation::SerializeMove(
    CFileMemBase* ar,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00010510, 0x1e, ??_GCSingleAnimation@@UAEPAXI@Z)
RVA_COMPGEN(0x00010540, 0x44, ??1CSingleAnimation@@UAE@XZ)

// @early-stop
// The vptr stamp and the body's first member re-read are transposed. Every body
// spelling is byte-identical (named local for the receiver, explicit read-modify-
// write, this->, a local for the flag word), as is the TU-state probe.
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
            if (list != NULL) {
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

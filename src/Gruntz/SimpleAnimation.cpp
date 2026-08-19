#include <rva.h>

#include <Gruntz/SimpleAnimation.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

RVA(0x0000f930, 0x47)
i32 CSimpleAnimation::SerializeMove(
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

RVA_COMPGEN(0x0000f9a0, 0x1e, ??_GCSimpleAnimation@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f9d0, 0x44, ??1CSimpleAnimation@@UAE@XZ)

RVA_DYNINIT(0x000abb70, 0xa, CActRegPool<CSimpleAnimation>::s_table)
RVA_DYNINIT(0x000abb90, 0x15, CActRegPool<CSimpleAnimation>::s_table)
RVA_DYNINIT(0x000abbc0, 0xe, CActRegPool<CSimpleAnimation>::s_table)
RVA_DYNINIT(0x000abbe0, 0x1f, CActRegPool<CSimpleAnimation>::s_table)
template<> DATA(0x00246038)
CActReg CActRegPool<CSimpleAnimation>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @early-stop
// Extent, calls, CFG, and ordered referents are exact. Retail materializes
// g_buteMgr before loading the second layer height; this TU schedules those
// independent loads in the opposite order, then chooses one different scratch.
// Thirty-two mixed declaration-kind TU states are byte-flat.
RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
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

RVA(0x000abc10, 0x102)
void CSimpleAnimation::FireActivation(i32 idx) {
    if (*CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx) != 0) {
        CActHandler fn = *CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx);
        (this->*fn)();
    }
}

RVA(0x000abd70, 0x18d)
void RegisterSimpleAnimLogic() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CSimpleAnimation::AdvanceAnim);
}

RVA(0x000abf70, 0x17)
i32 CSimpleAnimation::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

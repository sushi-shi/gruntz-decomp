#include <rva.h>

#include <Gruntz/SingleFrameMessage.h>

#include <Mfc.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WwdGameReg.h>
#include <Wap32/ZVec.h>

template<> DATA(0x00245ef0)
CActReg CActRegPool<CSingleFrameMessage>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0000f5a0, 0x47)
i32 CSingleFrameMessage::SerializeMove(
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

RVA_COMPGEN(0x0000f610, 0x1e, ??_GCSingleFrameMessage@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f640, 0x44, ??1CSingleFrameMessage@@UAE@XZ)

RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_object->ApplyLookupSprite("GAME_MESSAGEZ", m_wwdObject->m_id);
    RECT bounds;
    RECT r;
    CopyRect(&r, g_gameReg->GetRect(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}

// @interleaver FireActivation - fixed-size generated body (258 B, byte-identical across
// 51 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000ab5b0, 0x102)
void CSingleFrameMessage::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)))))();
    }
}

// @interleaver RegisterActs - fixed-size generated body (397 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000ab710, 0x18d)
void CSingleFrameMessage::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleFrameMessage::AdvanceAnim);
}

// @interleaver AdvanceAnim - 18 B lone body at 0xab910, between RegisterActs
// (singleframemessage) and ?0CSimpleAnimation (simpleanimation): a first-use placement.
RVA(0x000ab910, 0x12)
i32 CSingleFrameMessage::AdvanceAnim() {
    m_wwdObject->m_flags |= 0x10000;
    return 0;
}

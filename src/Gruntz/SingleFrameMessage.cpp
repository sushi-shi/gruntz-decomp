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

RVA_DYNINIT(0x000ab510, 0xa, CActRegPool<CSingleFrameMessage>::s_table)
RVA_DYNINIT(0x000ab530, 0x15, CActRegPool<CSingleFrameMessage>::s_table)
RVA_DYNINIT(0x000ab560, 0xe, CActRegPool<CSingleFrameMessage>::s_table)
RVA_DYNINIT(0x000ab580, 0x1f, CActRegPool<CSingleFrameMessage>::s_table)
template<> DATA(0x00245ef0)
CActReg CActRegPool<CSingleFrameMessage>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0000f5a0, 0x47)
i32 CSingleFrameMessage::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000f610, 0x1e, ??_GCSingleFrameMessage@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f640, 0x44, ??1CSingleFrameMessage@@UAE@XZ)

// @early-stop
RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    RECT r;
    SET_ANIMATION_ACT("A");
    m_object->SetImageFrameByName("GAME_MESSAGEZ", m_wwdObject->m_id);
    {
        RECT bounds;
        CopyRect(&r, g_gameReg->GetRect(&bounds));
    }
    CPoint center = CRect(r).CenterPoint();
    CWwdSpriteObject* object = m_object;
    object->SetScreenPos(Coord(center.x, center.y));
}

RVA(0x000ab5b0, 0x102)
void CSingleFrameMessage::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)))))();
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000ab710, 0x18d)
void CSingleFrameMessage::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleFrameMessage::AdvanceAnim);
}

RVA(0x000ab910, 0x12)
i32 CSingleFrameMessage::AdvanceAnim() {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return 0;
}

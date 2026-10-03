#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SingleFrameMessage.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WwdGameReg.h>
#include <ZTools/ZDArray.h>

template<>
CActReg CActRegPool<CSingleFrameMessage>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 CSingleFrameMessage::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    RECT r;
    SET_ANIMATION_ACT("A");
    m_object->SetImageFrameByName("GAME_MESSAGEZ", m_wwdObject->m_id);
    {
        RECT bounds;
        CopyRect(&r, g_gameReg->GetRect(&bounds));
    }
    POINT origin = {r.left, r.top};
    i32 centerY = (r.bottom - origin.y) / 2;
    i32 centerX = (r.right - origin.x) / 2;
    centerY += origin.y;
    centerX += origin.x;
    CWwdSpriteObject* object = m_object;
    SET_SCREEN_POS(object, centerX, centerY);
}

void CSingleFrameMessage::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CSingleFrameMessage::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CSingleFrameMessage>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleFrameMessage::AdvanceAnim);
}

i32 CSingleFrameMessage::AdvanceAnim() {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return 0;
}

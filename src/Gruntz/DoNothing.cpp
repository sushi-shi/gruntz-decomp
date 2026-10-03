#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/DoNothing.h>

#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/DoNothingNormal.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

i32 CDoNothing::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
){SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)}

i32 CDoNothingNormal::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
){SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)}

CDoNothing::CDoNothing(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/LevelTime.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

i32 CLevelTime::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

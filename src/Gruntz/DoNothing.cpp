#include <rva.h>

#include <Gruntz/DoNothing.h>

#include <Gruntz/DoNothingNormal.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

RVA(0x0000f6e0, 0x47)
i32 CDoNothing::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
){SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)}

RVA(0x0000f810, 0x47)
i32 CDoNothingNormal::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

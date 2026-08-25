#include <rva.h>

#include <Gruntz/DoNothing.h>

#include <Gruntz/DoNothingNormal.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

RVA(0x0000f6d0, 0x47)
i32 CDoNothing::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000f740, 0x1e, ??_GCDoNothing@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f770, 0x44, ??1CDoNothing@@UAE@XZ)

RVA(0x0000f800, 0x47)
i32 CDoNothingNormal::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000f870, 0x1e, ??_GCDoNothingNormal@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f8a0, 0x44, ??1CDoNothingNormal@@UAE@XZ)

CDoNothingNormal* RealizeCDoNothingNormal();
CDoNothingNormal* RealizeCDoNothingNormal() {
    return new CDoNothingNormal();
}

CDoNothing* RealizeCDoNothing() {
    return new CDoNothing();
}

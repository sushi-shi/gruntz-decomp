#include <rva.h>

#include <Gruntz/EyeCandy.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>

#include <stddef.h>

RVA(0x0000fcc0, 0x47)
i32 CEyeCandy::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000fd30, 0x1e, ??_GCEyeCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fd60, 0x44, ??1CEyeCandy@@UAE@XZ)

CEyeCandy* RealizeCEyeCandy() {
    return new CEyeCandy();
}

#include <rva.h>

#include <Gruntz/BehindCandy.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>

#include <stddef.h>

RVA(0x0000fb90, 0x47)
i32 CBehindCandy::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000fc00, 0x1e, ??_GCBehindCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fc30, 0x44, ??1CBehindCandy@@UAE@XZ)

CBehindCandy* RealizeCBehindCandy() {
    return new CBehindCandy();
}

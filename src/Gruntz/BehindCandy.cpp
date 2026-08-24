#include <rva.h>

#include <Gruntz/BehindCandy.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>

#include <stddef.h>

RVA(0x0000fb90, 0x47)
i32 CBehindCandy::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000fc00, 0x1e, ??_GCBehindCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fc30, 0x44, ??1CBehindCandy@@UAE@XZ)

// Realization device (see RealizeCDoNothingNormal): the CGameObject* ctor lives
// in FrontCandyAni.cpp, but retail kept this class's ??_G/??1 COMDATs inside
// this TU's contribution, so this TU still realizes the vtable (the header's
// inline default ctor stamps ??_7 here).
CBehindCandy* RealizeCBehindCandy() {
    return new CBehindCandy();
}

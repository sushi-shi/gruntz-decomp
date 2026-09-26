#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/FrontCandy.h>

#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>

#include <stddef.h>

RVA(0x0000fa60, 0x47)
i32 CFrontCandy::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000fad0, 0x1e, ??_GCFrontCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fb00, 0x44, ??1CFrontCandy@@UAE@XZ)

// @early-stop
RVA(0x000abfa0, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

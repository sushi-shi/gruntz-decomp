#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/FrontCandy.h>

#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>

#include <stddef.h>

i32 CFrontCandy::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/EyeCandy.h>

#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>

#include <stddef.h>

RVA(0x0000fcc0, 0x47)
i32 CEyeCandy::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
){SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)}

// @early-stop
RVA(0x000ac620, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    if (o->m_sortKey == 0 && o->m_frameImage != NULL) {
        i32 v = o->m_frameImage->m_anchorY + o->m_screenY + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(o, v)
    }
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

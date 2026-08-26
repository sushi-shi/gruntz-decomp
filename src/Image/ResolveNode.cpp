#include <rva.h>

#include <Gruntz/ResolveNode.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

RVA(0x00154cb0, 0x29)
CResolveNode::CResolveNode() : m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

RVA(0x00154ce0, 0x3)
LoadableClassId CWapObj::GetClassId() {
    return CLASSID_NONE;
}
RVA(0x00154cf0, 0x16)
i32 CResolveNode::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA_COMPGEN(0x00154d10, 0x1e, ??_GCResolveNode@@UAEPAXI@Z)
RVA_COMPGEN(0x00154d30, 0x23, ??1CResolveNode@@UAE@XZ)

#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ResolveNode.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

CResolveNode::CResolveNode() : m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

LoadableClassId CWapObj::GetClassId() {
    return CLASSID_NONE;
}

i32 CResolveNode::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

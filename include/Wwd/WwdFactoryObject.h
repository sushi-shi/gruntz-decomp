#ifndef GRUNTZ_WWD_WWDFACTORYOBJECT_H
#define GRUNTZ_WWD_WWDFACTORYOBJECT_H

#include <rva.h>

#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/ResolveNode.h>
#include <Ints.h>

struct CDDrawRect {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};

inline CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 id, i32 flags, EInlineSeed)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

#endif // GRUNTZ_WWD_WWDFACTORYOBJECT_H

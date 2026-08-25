#ifndef GRUNTZ_WWD_WWDFACTORYOBJECT_H
#define GRUNTZ_WWD_WWDFACTORYOBJECT_H

#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/ResolveNode.h>
#include <Ints.h>

struct CDDrawRect {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};

// The inline sibling of CResolveNode's out-of-line 0x15b2c0 (WwdObjMgr.cpp):
// only CGameObject's out-of-line 0x15b390 expands the seed, so the tag rides
// the inline entity (two-shapes-need-two-entities.md, ctor recipe).  A single
// visible 3-arg body is refuted: the creators' budget slice at its site
// affords cb 96+ (ASSERT x2 measured insufficient to make them decline), yet
// retail calls there.
inline CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 id, i32 flags, EInlineSeed)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

#endif // GRUNTZ_WWD_WWDFACTORYOBJECT_H

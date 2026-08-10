#ifndef GRUNTZ_GRUNTZ_RESOLVENODECTORINLINE_H
#define GRUNTZ_GRUNTZ_RESOLVENODECTORINLINE_H

#include <rva.h>

#include <Gruntz/ResolveNode.h>

// Opt-in inline visibility for CResolveNode's three-argument ctor (out of line at
// 0x15b2c0 in WwdObjMgr.cpp).  Retail expands it inside CGameObject's out-of-line
// ctor (0x15b390) and leaves it a `call` in CDDrawChildGroup's three creators, which
// carry the CGameObject body itself expanded - so the two shapes are a property of
// the TU, not of the call.
inline CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CWapObj(owner, field04, field08, CWapObj::NO_SEED), m_dirty(WwdDirtyRect::INLINE_SEED) {
    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}

#endif // GRUNTZ_GRUNTZ_RESOLVENODECTORINLINE_H

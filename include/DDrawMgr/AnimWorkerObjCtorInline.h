#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJCTORINLINE_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJCTORINLINE_H

#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>

// Opt-in inline visibility for AnimWorkerObj's three-argument ctor (out of line at
// 0x15b300 in WwdObjMgr.cpp).  Same split as CResolveNode's: expanded inside
// CGameObject's out-of-line ctor (0x15b390), a `call` in the three creators.
inline AnimWorkerObj::AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CLoadable(owner, id, stateFlags) {
    ResetWorkerFields();
}

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJCTORINLINE_H

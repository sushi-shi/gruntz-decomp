#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJCTORINLINE_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJCTORINLINE_H

#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>

// Opt-in inline visibility for AnimWorkerObj's three-argument ctor (out of
// line at 0x15b300 in WwdObjMgr.cpp) - a surviving per-TU visibility split.
// A single visible body is refuted by measurement (2026-08-15): the call and
// the expansion sit inside ONE spelling of CGameObject::AttachToOwner, and
// the creators' budget slice is coupled to CResolveNode's site - with
// CResolveNode out of line the slice affords cb 60+, so no plausible
// compiled-out content makes the creators decline this cb 56-58 body, and a
// tag sibling cannot ride the shared AttachToOwner text.  Ledger:
// docs/patterns/comdat-home-adjudicates-inline-spelling.md.
inline AnimWorkerObj::AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CWapObj(owner, id, stateFlags, CWapObj::NO_SEED) {
    ResetWorkerFields();
}

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJCTORINLINE_H

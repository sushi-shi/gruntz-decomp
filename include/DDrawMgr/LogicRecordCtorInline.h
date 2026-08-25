#ifndef GRUNTZ_DDRAWMGR_LOGICRECORDCTORINLINE_H
#define GRUNTZ_DDRAWMGR_LOGICRECORDCTORINLINE_H

#include <rva.h>

#include <DDrawMgr/LogicRecord.h>

// Opt-in inline visibility for CLogicRecord's three-argument ctor (out of
// line at 0x15b300 in WwdObjMgr.cpp, same text on both sides).  This is a
// WORKAROUND for caller-side modelling error, not a proven era structure - no
// dev writes a per-TU visibility header.  Retested 2026-08-22 by collapsing to
// ONE in-class body carrying the RVA pin, with the out-of-line definition and
// this header deleted:
//   * 0x15b300 lost every emitter - verify unique-names goes FATAL ("no body in
//     ANY claiming unit's object") and the row scores 100.00 -> 0.00, so unlike
//     CLogicRecordRegistry::FindTemplate no TU declines this one;
//   * the three creators retail CALLS it from expanded it instead:
//     CreateSpriteObject 100.00 -> 86.14, CreateDotObject 100.00 -> 84.92,
//     CreateDeferredObject 100.00 -> 83.04, plus ten of their /GX unwind
//     funclets (three to 0.00).
// The expansion site is `new CLogicRecord(owner, id, 0)` inside the single
// inline CGameObject::AttachToOwner body - depth 2 from the pinned CGameObject
// ctor (WwdFactoryObject, expands) and depth 3 from wwdobjmgr's creators
// (retail calls).  One text feeds both, so no per-site spelling (the tagged
// sibling that dissolved CResolveNode's .inl) can express the split.
// REMOVAL CONDITION: model wwdobjmgr's three creators' pre-optimization cost
// accurately enough that cl 5.0 declines the depth-3 site on its own budget and
// homes 0x15b300 in wwdobjmgr; then one visible body reproduces both shapes.
// Ledger: docs/patterns/comdat-home-adjudicates-inline-spelling.md.
inline CLogicRecord::CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CWapObj(owner, id, stateFlags, CWapObj::NO_SEED) {
    ResetLogicFields();
}

#endif // GRUNTZ_DDRAWMGR_LOGICRECORDCTORINLINE_H

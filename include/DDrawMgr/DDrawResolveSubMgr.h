#ifndef GRUNTZ_DDRAWMGR_CDDRAWRESOLVESUBMGR_H
#define GRUNTZ_DDRAWMGR_CDDRAWRESOLVESUBMGR_H

// DDrawResolveSubMgr.h - the +0x24 resolution/level child of CDDrawSurfaceMgr
// (tomalla CDDrawResolveSubMgr / "CGameLevel" in the Init decode). A real distinct
// class: 0x6d4 bytes, ctor 0x15ccd0 (`new(0x6d4)` in the owner's Init 0x155900),
// torn down through the family slot-1 scalar-deleting dtor like its siblings.
// SetCoords is the owner's SetDimensions relay (0x155f60 tail call site).
//
// PROVENANCE: replaces the former DDrawSurfaceMgr.cpp-local derived view.

#include <DDrawMgr/DDrawSubMgr.h>
#include <Ints.h>
#include <rva.h>

class CDDrawResolveSubMgr : public CDDrawSubMgr {
public:
    i32 SetCoords(i32 x, i32 y); // declared-only (body unreconstructed)
};
SIZE(CDDrawResolveSubMgr, 0x6d4); // new(0x6d4) at the owner Init store

#endif // GRUNTZ_DDRAWMGR_CDDRAWRESOLVESUBMGR_H

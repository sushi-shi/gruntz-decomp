// GameStateRecord.h - light, decl-only shims for CGrunt::SerializeMove's cross-unit
// serialize callees. Kept minimal (no Mfc.h, no CDDrawSubMgrLeaf) so it can be pulled
// into GruntSteps without colliding with the several other CDDrawSubMgrLeaf views
// (SerialObjRef.h's own CSerialObjRef carries one, which redefines the local view in
// BoundaryLowerMethodsViews.h). Every method below is DEFINED in another unit (bound
// via that unit's RVA()), so these are reloc-masked externals, not fake views.
#ifndef GRUNTZ_GAMESTATERECORD_H
#define GRUNTZ_GAMESTATERECORD_H

#include <Ints.h>

// (The CGameStateRecord shim is GONE: 0x555e0 is CGrunt::Load, declared in
// <Gruntz/Grunt.h> and defined in GameStateRecordLoad.cpp - the attribution
// TODO this header carried is executed.)

// The ONE CSerialObjRef (Chain @0x8c00). The ODR-duplicate one-method shell this
// header used to carry is folded (2026-07-16): SerialObjRef.h now uses the
// canonical CDDrawSubMgrLeaf, so the old "view collides" blocker is gone.
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

#endif // GRUNTZ_GAMESTATERECORD_H

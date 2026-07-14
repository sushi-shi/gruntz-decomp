// GameStateRecord.h - light, decl-only shims for CGrunt::SerializeMove's cross-unit
// serialize callees. Kept minimal (no Mfc.h, no CDDrawSubMgrLeaf) so it can be pulled
// into GruntSteps without colliding with the several other CDDrawSubMgrLeaf views
// (SerialObjRef.h's own CSerialObjRef carries one, which redefines the local view in
// BoundaryLowerMethodsViews.h). Every method below is DEFINED in another unit (bound
// via that unit's RVA()), so these are reloc-masked externals, not fake views.
#ifndef GRUNTZ_GAMESTATERECORD_H
#define GRUNTZ_GAMESTATERECORD_H

#include <Ints.h>

// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;
class CSerialObj;

// (The CGameStateRecord shim is GONE: 0x555e0 is CGrunt::Load, declared in
// <Gruntz/Grunt.h> and defined in GameStateRecordLoad.cpp - the attribution
// TODO this header carried is executed.)

// CSerialObjRef::Chain (0x8c00): the +0x150 serialized-object-reference's chain. The
// full class lives in <Gruntz/SerialObjRef.h>; only Chain is needed here and that
// header can't be pulled into GruntSteps (its CDDrawSubMgrLeaf view collides).
class CSerialObjRef {
public:
    i32 Chain(CSerialArchive* arc, i32 mode, i32 unused, CSerialObj* obj); // 0x8c00
};

#endif // GRUNTZ_GAMESTATERECORD_H

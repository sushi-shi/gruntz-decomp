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

// 0x555e0 is delinked/labeled ?Load@CGameStateRecord@@QAEHPAUCSerialArchive@@@Z
// (reconstructed in gamestaterecordload), but its ONLY caller is CGrunt::SerializeMove
// (mode 7) invoked on ecx = the CGrunt `this`, and its body indexes that same grunt
// layout by raw offset - i.e. it is really CGrunt's load facet. Declared here decl-only
// so SerializeMove's mode-7 call binds to 0x555e0 without a .cpp-local view. Attribution
// TODO: rename 0x555e0 to CGrunt::Load in gamestaterecordload.
class CGameStateRecord {
public:
    i32 Load(CSerialArchive* ar); // 0x555e0
};

// CSerialObjRef::Chain (0x8c00): the +0x150 serialized-object-reference's chain. The
// full class lives in <Gruntz/SerialObjRef.h>; only Chain is needed here and that
// header can't be pulled into GruntSteps (its CDDrawSubMgrLeaf view collides).
class CSerialObjRef {
public:
    i32 Chain(CSerialArchive* arc, i32 mode, i32 unused, CSerialObj* obj); // 0x8c00
};

#endif // GRUNTZ_GAMESTATERECORD_H

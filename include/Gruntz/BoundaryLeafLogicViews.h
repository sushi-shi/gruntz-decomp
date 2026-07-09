// BoundaryLeafLogicViews.h - the placeholder CUserLogic leaf classes reconstructed in
// BoundaryLeafLogic.cpp (tile-logic leaf destructors + Serialize overrides recovered
// from the engine_boundary backlog).
//
// RTTI cannot attribute these COMDAT-folded leaf methods, so the leaf class names are
// placeholders (L_<rva> / S_<rva>); the recovered FACT is that each is a CUserLogic
// leaf (the shared game-object hierarchy) - only the inheritance chain + offsets are
// load-bearing. Formerly declared inline per-TU; consolidating the class declarations
// into this shared header is pure code motion (matching-neutral). The bodies (each
// carrying its RVA()) stay in BoundaryLeafLogic.cpp.
#ifndef GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H
#define GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic / CUserBase base hierarchy

// Leaf destructors (the /GX leaf-dtor archetype; byte-identical to ~CSimpleAnimation,
// the only per-class difference being the reloc-masked EH funcinfo table).
class L_8860 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_8860() OVERRIDE;
};
SIZE_UNKNOWN(L_8860);
RELOC_VTBL(L_8860, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)
class L_fb00 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_fb00() OVERRIDE;
};
SIZE_UNKNOWN(L_fb00);
RELOC_VTBL(L_fb00, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)

// Serialize override (the two-chain archetype).
class S_fdf0 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
};
SIZE_UNKNOWN(S_fdf0);

class L_fe90 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_fe90() OVERRIDE;
};
SIZE_UNKNOWN(L_fe90);
RELOC_VTBL(L_fe90, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)
class L_101b0 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_101b0() OVERRIDE;
};
SIZE_UNKNOWN(L_101b0);
RELOC_VTBL(L_101b0, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)

class S_104a0 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
};
SIZE_UNKNOWN(S_104a0);
class S_105d0 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
};
SIZE_UNKNOWN(S_105d0);

class L_10fc0 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_10fc0() OVERRIDE;
};
SIZE_UNKNOWN(L_10fc0);
RELOC_VTBL(L_10fc0, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)
class L_11b80 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_11b80() OVERRIDE;
};
SIZE_UNKNOWN(L_11b80);
RELOC_VTBL(L_11b80, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)
class L_13400 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_13400() OVERRIDE;
};
SIZE_UNKNOWN(L_13400);
RELOC_VTBL(L_13400, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)

#endif // GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H

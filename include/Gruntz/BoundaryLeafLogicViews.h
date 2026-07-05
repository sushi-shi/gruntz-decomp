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
class L_8860 : public CTileLogic {
public:
    ~L_8860() OVERRIDE;
};
SIZE_UNKNOWN(L_8860);
class L_f510 : public CTileLogic {
public:
    ~L_f510() OVERRIDE;
};
SIZE_UNKNOWN(L_f510);
class L_f640 : public CTileLogic {
public:
    ~L_f640() OVERRIDE;
};
SIZE_UNKNOWN(L_f640);
class L_fb00 : public CTileLogic {
public:
    ~L_fb00() OVERRIDE;
};
SIZE_UNKNOWN(L_fb00);

// Serialize override (the two-chain archetype).
class S_fdf0 : public CTileLogic {
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
};
SIZE_UNKNOWN(S_fdf0);

class L_fe90 : public CTileLogic {
public:
    ~L_fe90() OVERRIDE;
};
SIZE_UNKNOWN(L_fe90);
class L_ffc0 : public CTileLogic {
public:
    ~L_ffc0() OVERRIDE;
};
SIZE_UNKNOWN(L_ffc0);
class L_101b0 : public CTileLogic {
public:
    ~L_101b0() OVERRIDE;
};
SIZE_UNKNOWN(L_101b0);

class S_104a0 : public CTileLogic {
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
};
SIZE_UNKNOWN(S_104a0);
class S_105d0 : public CTileLogic {
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
};
SIZE_UNKNOWN(S_105d0);

class L_10fc0 : public CTileLogic {
public:
    ~L_10fc0() OVERRIDE;
};
SIZE_UNKNOWN(L_10fc0);
class L_11b80 : public CTileLogic {
public:
    ~L_11b80() OVERRIDE;
};
SIZE_UNKNOWN(L_11b80);
class L_11c40 : public CTileLogic {
public:
    ~L_11c40() OVERRIDE;
};
SIZE_UNKNOWN(L_11c40);
class L_13040 : public CTileLogic {
public:
    ~L_13040() OVERRIDE;
};
SIZE_UNKNOWN(L_13040);
class L_13400 : public CTileLogic {
public:
    ~L_13400() OVERRIDE;
};
SIZE_UNKNOWN(L_13400);

#endif // GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H

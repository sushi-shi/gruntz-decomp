// DoNothingNormalDtor.h - matched-world view of CDoNothingNormal for its leaf
// destructor (C:\Proj\Gruntz).
//
// CDoNothingNormal : CUserLogic (RTTI .?AVCDoNothingNormal@@) - the "normal"
// variant of the inert do-nothing tile-logic object (sibling of CDoNothing).
// Owner recovered by caller-trace: the scalar-deleting-destructor @0x0000f870
// (CDoNothingNormal vftable slot 0) tail-calls this plain dtor @0x0000f8a0. The
// dtor stamps the CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4,
// tearing down the +0x18 link via the embedded ~EngStr @0x16d2a0 - byte-identical
// to ~CDoNothing @0x0000f770 / the established leaf-dtor archetype.
//
// NOTE: 0xa9e00 (the trace mis-named it the ctor; it is really the __cdecl logic-
// worker message pump) is reconstructed in src/Gruntz/DoNothingNormalLogic.cpp;
// this matched-world view exists ONLY to host the leaf dtor against the real
// CUserLogic teardown. Field names are placeholders; only OFFSETS + the
// inheritance chain are load-bearing.
#ifndef GRUNTZ_CDONOTHINGNORMALDTOR_H
#define GRUNTZ_CDONOTHINGNORMALDTOR_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CDoNothingNormal : CUserLogic)

SIZE_UNKNOWN(CDoNothingNormal);
class CDoNothingNormal : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~CDoNothingNormal() OVERRIDE; // 0x0000f8a0 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CDONOTHINGNORMALDTOR_H

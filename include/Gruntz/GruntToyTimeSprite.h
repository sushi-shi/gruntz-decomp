// GruntToyTimeSprite.h - the grunt toy-timer HUD sprite (C:\Proj\Gruntz).
//
// RTTI (.?AVCGruntToyTimeSprite@@, vtbl 0x1e79ec) gives the true base as
// CGruntHealthSprite (17 slots, slot 16 = origin CGruntHealthSprite overridden), so
// the class is modeled `: CGruntHealthSprite` - the ctor (0x0007fbd0) chains the
// out-of-line CGruntHealthSprite base ctor (0x7eb00) and the leaf dtor (0x012130)
// folds the SAME CUserLogic teardown in-place (the intermediate leaf/health vptr
// stamps dead-store-eliminate against the CUserLogic stamp - CGruntHealthSprite's dtor
// is inline, see GruntHealthSprite.h). Adds no data members over the 0x64-byte base.
// Only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTTOYTIMESPRITE_H
#define GRUNTZ_CGRUNTTOYTIMESPRITE_H

#include <rva.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/Grunt.h>       // CGrunt (the toy-time accessor's bound grunt)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)

class CGruntToyTimeSprite : public CGruntHealthSprite {
public:
    CGruntToyTimeSprite(CGameObject* obj); // 0x0007fbd0 (body in GruntToyTimeSprite.cpp)
    // GetTypeTag (0x120e0, slot 2): inline body + RVA in the header so cl emits the
    // COMDAT wherever the ctor's vtable is emitted (GruntToyTimeSprite.cpp).
    RVA(0x000120e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYTIMESPRITE;
    }
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    virtual i32 Vslot16(CGrunt* grunt) OVERRIDE; // slot 16 (stat-time getter)
};
VTBL(CGruntToyTimeSprite, 0x001e79ec);
SIZE(CGruntToyTimeSprite, 0x64); // recovered from operator-new sites (gruntz.analysis.news)

#endif // GRUNTZ_CGRUNTTOYTIMESPRITE_H

// CGruntToyTimeSprite.h - the grunt toy-timer HUD sprite (C:\Proj\Gruntz).
//
// RTTI (.?AVCGruntToyTimeSprite@@, vtbl 0x1e79ec) gives the true base as
// CGruntHealthSprite (17 slots, slot 16 = origin CGruntHealthSprite overridden).
// We DELIBERATELY model it as `: CUserLogic` DIRECTLY (documented held-base, the
// CLightningHazard precedent). Reason: the leaf dtor @0x012130 (0x44 B, 100%)
// INLINES the full CUserLogic teardown (stamp 0x5e705c, ~EngStr on +0x18, stamp
// 0x5e70b4) and DEAD-STORE-ELIMINATES the intermediate CGruntHealthSprite/leaf vptr
// stamps (dump 0x12130: `[esi]=0x5e705c` first, no CGruntHealthSprite vtable ref,
// no ~CGruntHealthSprite call). Deriving CGruntHealthSprite (out-of-line dtor
// @0x00011fb0 in another TU) would force cl to emit a CALL instead of the inlined
// teardown, regressing the byte-exact dtor; the intermediate base adds no
// destructible members, so direct-CUserLogic is byte-identical to the true chain.
// Only the /GX leaf dtor is reconstructed here; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTTOYTIMESPRITE_H
#define GRUNTZ_CGRUNTTOYTIMESPRITE_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CGruntToyTimeSprite : public CUserLogic {
public:
    ~CGruntToyTimeSprite() OVERRIDE; // 0x012130 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
SIZE(CGruntToyTimeSprite, 0x54);

#endif // GRUNTZ_CGRUNTTOYTIMESPRITE_H

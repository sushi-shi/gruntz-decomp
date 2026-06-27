#include <rva.h>
// CCoveredPowerupLogic.cpp - CCoveredPowerupLogic ctor (matched).
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CCoveredPowerupLogicBase {
    CCoveredPowerupLogicBase();
    // real polymorphic base: 1 declared-only virtual(s) so cl
    // emits the leaf ??_7 + implicit ctor vptr-stamp (RTTI auto-named).
    virtual void Vf0();
};

class CCoveredPowerupLogic : public CCoveredPowerupLogicBase {
public:
    CCoveredPowerupLogic();
};

// Leaf ??_7 vftable now emitted by cl + named on the target automatically
// (RTTI auto-namer); the manual struct stamp is gone.

// @confidence: high
// @source: rtti-vptr
RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {
    // base ctor call + vptr stamp are now both implicit (real
    // polymorphic class) - replaces the manual struct stamp.
}

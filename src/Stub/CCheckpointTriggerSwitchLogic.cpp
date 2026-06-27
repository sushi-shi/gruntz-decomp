#include <rva.h>
// CCheckpointTriggerSwitchLogic.cpp - CCheckpointTriggerSwitchLogic ctor.
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerSwitchLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CCheckpointTriggerSwitchLogicBase {
    CCheckpointTriggerSwitchLogicBase();
    // real polymorphic base: 4 declared-only virtual(s) so cl
    // emits the leaf ??_7 + implicit ctor vptr-stamp (RTTI auto-named).
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};

class CCheckpointTriggerSwitchLogic : public CCheckpointTriggerSwitchLogicBase {
public:
    CCheckpointTriggerSwitchLogic();
};

// Leaf ??_7 vftable now emitted by cl + named on the target automatically
// (RTTI auto-namer); the manual struct stamp is gone.

// @confidence: high
// @source: rtti-vptr
RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {
    // base ctor call + vptr stamp are now both implicit (real
    // polymorphic class) - replaces the manual struct stamp.
}

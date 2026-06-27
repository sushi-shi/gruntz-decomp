#include <rva.h>
// CTileMultiTriggerSwitchLogic.cpp - CTileMultiTriggerSwitchLogic ctor (matched).
//
// Derived tile-trigger "switch" logic class.  The ctor is the canonical 18-byte
// game-object archetype: chain to the CTileTriggerSwitchLogic base ctor (an
// engine fn we are NOT matching -> external no-body, reloc-masked rel32 call),
// then re-stamp the derived vftable into [this].  Field names are placeholders;
// only the offsets + the emitted code bytes are load-bearing.

// Base ctor: external, no body -> its `call` reloc-masks.  __thiscall (receiver
// in ecx); the base re-stamps its own vftable, the derived ctor overwrites it.
// (A per-file shell so this TU member of the All.cpp aggregate is self-contained;
// the reloc-masked call binds to the retail CTileTriggerSwitchLogic ctor by addr.)
struct CTileMultiTriggerSwitchLogicBase {
    CTileMultiTriggerSwitchLogicBase();
    // real polymorphic base: 4 declared-only virtual(s) so cl
    // emits the leaf ??_7 + implicit ctor vptr-stamp (RTTI auto-named).
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};

class CTileMultiTriggerSwitchLogic : public CTileMultiTriggerSwitchLogicBase {
public:
    CTileMultiTriggerSwitchLogic();
};

// Leaf ??_7 vftable now emitted by cl + named on the target automatically
// (RTTI auto-namer); the manual struct stamp is gone.

// @confidence: high
// @source: rtti-vptr
RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {
    // base ctor call + vptr stamp are now both implicit (real
    // polymorphic class) - replaces the manual struct stamp.
}

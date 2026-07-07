// TileTriggerDerivedCtors.cpp - the 18-byte derived tile-trigger logic ctors
// (C:\Proj\Gruntz), homed from src/Stub/. Each is the canonical logic-ctor
// archetype: chain to the (engine, NOT matched) base ctor - an external no-body
// call so its rel32 reloc-masks - then re-stamp the derived vftable into [this].
//
// Each derived class is now modeled over its REAL RTTI base: the 1-virtual
// CTileTriggerLogic (vtable 0x5eaea4) for the "logic" family and the 4-virtual
// CTileTriggerSwitchLogic (vtable 0x5eae8c) for the "switch" family (both fully
// modeled in their own headers). cl emits the leaf ??_7 (1 slot / 4 slots, matching
// retail) + the implicit ctor vptr-stamp, and the build's RTTI auto-namer names the
// leaf vtable. The base ctor is defined in a DIFFERENT TU (TileTriggerLogic.cpp /
// TileTriggerSwitchLogic.cpp), so its call stays external and the rel32 masks. The
// fabricated per-class `*Base` stand-ins are gone. Definitions are in retail-RVA
// order.
#include <Gruntz/TileTriggerLogic.h>       // real 1-virtual base (logic family)
#include <Gruntz/TileTriggerSwitchLogic.h> // real 4-virtual base (switch family)
#include <rva.h>

// --- CTileTriggerSwitchLogic family (base = 4 virtuals) --------------------
class CTileMultiTriggerSwitchLogic : public CTileTriggerSwitchLogic {
public:
    CTileMultiTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileMultiTriggerSwitchLogic);
RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

class CTileExclusiveTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual void Vf2() OVERRIDE; // slot 2
public:
    CTileExclusiveTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileExclusiveTriggerSwitchLogic);
RVA(0x00112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {}

// --- CTileTriggerLogic family (base = 1 virtual) ---------------------------
class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic();
};
SIZE_UNKNOWN(CGiantRockLogic);
RVA(0x00112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {}

class CCoveredPowerupLogic : public CTileTriggerLogic {
public:
    CCoveredPowerupLogic();
};
SIZE_UNKNOWN(CCoveredPowerupLogic);
RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {}

class CTileTimeTriggerLogic : public CTileTriggerLogic {
public:
    CTileTimeTriggerLogic();
};
SIZE_UNKNOWN(CTileTimeTriggerLogic);
RVA(0x00112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {}

class CTileSecretTriggerLogic : public CTileTriggerLogic {
    virtual i32 TileLogicVfunc0() OVERRIDE; // slot 0
public:
    CTileSecretTriggerLogic();
};
SIZE_UNKNOWN(CTileSecretTriggerLogic);
RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {}

// --- CTileTriggerSwitchLogic family (base = 4 virtuals), upper RVAs --------
class CTileSecretTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual void Vf2() OVERRIDE; // slot 2
public:
    CTileSecretTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileSecretTriggerSwitchLogic);
RVA(0x00112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {}

class CTileTimeTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual void Vf2() OVERRIDE; // slot 2
    virtual void Vf3() OVERRIDE; // slot 3
public:
    CTileTimeTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileTimeTriggerSwitchLogic);
RVA(0x001127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {}

class CCheckpointTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual void Vf1() OVERRIDE; // slot 1
    virtual void Vf2() OVERRIDE; // slot 2
    virtual void Vf3() OVERRIDE; // slot 3
public:
    CCheckpointTriggerSwitchLogic();
};
SIZE_UNKNOWN(CCheckpointTriggerSwitchLogic);
RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {}

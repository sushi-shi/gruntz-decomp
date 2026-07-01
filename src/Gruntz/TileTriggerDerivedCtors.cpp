// TileTriggerDerivedCtors.cpp - the 18-byte derived tile-trigger logic ctors
// (C:\Proj\Gruntz), homed from src/Stub/. Each is the canonical logic-ctor
// archetype: chain to the (engine, NOT matched) base ctor - an external no-body
// call so its rel32 reloc-masks - then re-stamp the derived vftable into [this].
//
// Each derived class is modeled as a REAL polymorphic class over a per-class base
// shell carrying the base's virtual count (1 for the CTileTriggerLogic family, 4
// for the CTileTriggerSwitchLogic family): cl then emits the leaf ??_7 + the
// implicit ctor vptr-stamp, and the build's RTTI auto-namer names the leaf vtable.
// The base ctor is declared-only here (defined in TileTriggerLogic.cpp /
// TileTriggerSwitchLogic.cpp, a different TU), so its call stays external and the
// reloc masks regardless of the placeholder base name. Definitions are in
// retail-RVA order.
#include <rva.h>

// --- CTileTriggerSwitchLogic family (base = 4 virtuals) --------------------
struct CTileMultiTriggerSwitchLogicBase {
    CTileMultiTriggerSwitchLogicBase();
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};
SIZE_UNKNOWN(CTileMultiTriggerSwitchLogicBase);
class CTileMultiTriggerSwitchLogic : public CTileMultiTriggerSwitchLogicBase {
public:
    CTileMultiTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileMultiTriggerSwitchLogic);
RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

struct CTileExclusiveTriggerSwitchLogicBase {
    CTileExclusiveTriggerSwitchLogicBase();
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};
SIZE_UNKNOWN(CTileExclusiveTriggerSwitchLogicBase);
class CTileExclusiveTriggerSwitchLogic : public CTileExclusiveTriggerSwitchLogicBase {
public:
    CTileExclusiveTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileExclusiveTriggerSwitchLogic);
RVA(0x00112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {}

// --- CTileTriggerLogic family (base = 1 virtual) ---------------------------
struct CGiantRockLogicBase {
    CGiantRockLogicBase();
    virtual void Vf0();
};
SIZE_UNKNOWN(CGiantRockLogicBase);
class CGiantRockLogic : public CGiantRockLogicBase {
public:
    CGiantRockLogic();
};
SIZE_UNKNOWN(CGiantRockLogic);
RVA(0x00112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {}

struct CCoveredPowerupLogicBase {
    CCoveredPowerupLogicBase();
    virtual void Vf0();
};
SIZE_UNKNOWN(CCoveredPowerupLogicBase);
class CCoveredPowerupLogic : public CCoveredPowerupLogicBase {
public:
    CCoveredPowerupLogic();
};
SIZE_UNKNOWN(CCoveredPowerupLogic);
RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {}

struct CTileTimeTriggerLogicBase {
    CTileTimeTriggerLogicBase();
    virtual void Vf0();
};
SIZE_UNKNOWN(CTileTimeTriggerLogicBase);
class CTileTimeTriggerLogic : public CTileTimeTriggerLogicBase {
public:
    CTileTimeTriggerLogic();
};
SIZE_UNKNOWN(CTileTimeTriggerLogic);
RVA(0x00112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {}

struct CTileSecretTriggerLogicBase {
    CTileSecretTriggerLogicBase();
    virtual void Vf0();
};
SIZE_UNKNOWN(CTileSecretTriggerLogicBase);
class CTileSecretTriggerLogic : public CTileSecretTriggerLogicBase {
public:
    CTileSecretTriggerLogic();
};
SIZE_UNKNOWN(CTileSecretTriggerLogic);
RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {}

// --- CTileTriggerSwitchLogic family (base = 4 virtuals), upper RVAs --------
struct CTileSecretTriggerSwitchLogicBase {
    CTileSecretTriggerSwitchLogicBase();
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};
SIZE_UNKNOWN(CTileSecretTriggerSwitchLogicBase);
class CTileSecretTriggerSwitchLogic : public CTileSecretTriggerSwitchLogicBase {
public:
    CTileSecretTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileSecretTriggerSwitchLogic);
RVA(0x00112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {}

struct CTileTimeTriggerSwitchLogicBase {
    CTileTimeTriggerSwitchLogicBase();
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};
SIZE_UNKNOWN(CTileTimeTriggerSwitchLogicBase);
class CTileTimeTriggerSwitchLogic : public CTileTimeTriggerSwitchLogicBase {
public:
    CTileTimeTriggerSwitchLogic();
};
SIZE_UNKNOWN(CTileTimeTriggerSwitchLogic);
RVA(0x001127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {}

struct CCheckpointTriggerSwitchLogicBase {
    CCheckpointTriggerSwitchLogicBase();
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
};
SIZE_UNKNOWN(CCheckpointTriggerSwitchLogicBase);
class CCheckpointTriggerSwitchLogic : public CCheckpointTriggerSwitchLogicBase {
public:
    CCheckpointTriggerSwitchLogic();
};
SIZE_UNKNOWN(CCheckpointTriggerSwitchLogic);
RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {}

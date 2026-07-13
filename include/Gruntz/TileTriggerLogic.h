// TileTriggerLogic.h - Gruntz tile-trigger logic class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the constructor. Offsets are the
// load-bearing fact the match proves.
#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <Ints.h>
#include <rva.h>

class CTileTriggerContainer; // owner, back-stamped into m_20 (fwd; def in TileTriggerContainer.h)

// ---------------------------------------------------------------------------
// CTileTriggerLogic
//   vftable (0x5eaea4, ONE slot). size 0x9c. ctor:
//     mov edx,this; vptr@0; rep stosl zeroes 24 dwords (96 bytes) starting at
//     +0x3c (m_block); then m_1c (+0x1c) = 0 (reusing the zero in eax, emitted
//     AFTER the rep stosl -> the m_block array is initialised before m_1c).
//
// The retail vtable at 0x5eaea4 holds exactly ONE slot (0x402072 -> the regular
// virtual at 0x110c10); the derived logic classes (CGiantRockLogic, ...) share
// that same slot value -> it is an INHERITED regular virtual, not a per-class
// destructor.  So CTileTriggerLogic is modeled with a single non-dtor virtual and
// NO virtual destructor (the earlier ??_G@0x116610 label was a misattribution: that
// function ends `ret 0x84`, a 33-arg engine fn, not a scalar-deleting dtor).
//
// The ctor zeroes only m_block+m_1c; the remaining fields (m_04..m_38) are filled
// by the AddLogic factory (0x116610) after construction. The inline dtor is the one
// the factory's failure path inlines: stamp ??_7 (auto vptr), zero m_1c, RezFree.
// ---------------------------------------------------------------------------
SIZE(CTileTriggerLogic, 0x9c);
VTBL(CTileTriggerLogic, 0x001eaea4); // vtable_names -> code (RTTI game class)
class CTileTriggerLogic {
public:
    CTileTriggerLogic();
    ~CTileTriggerLogic() {
        m_1c = 0;
    }
    virtual i32 TileLogicVfunc0(); // slot 0 (0x110c10 via ILT thunk 0x402072)

    i32 m_04;                    // +0x04
    i32 m_08;                    // +0x08
    i32 m_0c;                    // +0x0c
    i32 m_10;                    // +0x10
    i32 m_14;                    // +0x14
    i32 m_18;                    // +0x18
    i32 m_1c;                    // +0x1c  init flag (zeroed by ctor AFTER m_block)
    CTileTriggerContainer* m_20; // +0x20  owning container
    u32 m_24;                    // +0x24  game-clock snapshot (g_645588)
    i32 m_28;                    // +0x28
    i32 m_2c;                    // +0x2c
    i32 m_30;                    // +0x30
    i32 m_34;                    // +0x34
    i32 m_38;                    // +0x38
    i32 m_block[24];             // +0x3c..0x9b  (rep stosl, 24 dwords)
};

// The per-id logic leaves of the CTileTriggerLogic family (base = 1 virtual): each is
// 0x9c bytes and carries its own RTTI vtable. The AddLogic factory (0x116610) news the
// four non-rock leaves; the serialize Build factory (0x117800) constructs the same set
// via its CTrigLogic9c size-bucket. Their ctors are defined (RVA-bound) in
// TileTriggerSwitchLogic.cpp - the header carries only the declarations so both TUs
// share one class shape (dissolved from the old TileTriggerSwitchLogic.cpp-local defs).
class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic(); // 0x112210 (ILT 0x2c3e)
};
SIZE_UNKNOWN(CGiantRockLogic);
VTBL(CGiantRockLogic, 0x001eaee4); // vtable_names -> code (RTTI game class)

class CCoveredPowerupLogic : public CTileTriggerLogic {
public:
    CCoveredPowerupLogic(); // 0x112240 (ILT 0x2a4f)
};
// Size PROVEN from the allocation site (push 0x9c; call ??2 -> the ctor), and our
// reconstruction computes exactly that. Pinned so no future note can claim it unknown.
SIZE(CCoveredPowerupLogic, 0x9c);
VTBL(CCoveredPowerupLogic, 0x001eaef4); // vtable_names -> code (RTTI game class)

class CTileTimeTriggerLogic : public CTileTriggerLogic {
public:
    CTileTimeTriggerLogic(); // 0x112270 (ILT 0x18de)
};
// Size PROVEN from the allocation site (push 0x9c; call ??2 -> the ctor), and our
// reconstruction computes exactly that. Pinned so no future note can claim it unknown.
SIZE(CTileTimeTriggerLogic, 0x9c);
VTBL(CTileTimeTriggerLogic, 0x001eaf04); // vtable_names -> code (RTTI game class)

class CTileSecretTriggerLogic : public CTileTriggerLogic {
    virtual i32 TileLogicVfunc0() OVERRIDE; // slot 0
public:
    CTileSecretTriggerLogic(); // 0x112760 (ILT 0x310c)
};
// Size PROVEN from the allocation site (push 0x9c; call ??2 -> the ctor), and our
// reconstruction computes exactly that. Pinned so no future note can claim it unknown.
SIZE(CTileSecretTriggerLogic, 0x9c);
VTBL(CTileSecretTriggerLogic, 0x001eaf14); // vtable_names -> code (RTTI game class)

#endif // TILETRIGGERLOGIC_H

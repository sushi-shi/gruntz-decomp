// TileTriggerLogic.h - Gruntz tile-trigger logic class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the constructor. Offsets are the
// load-bearing fact the match proves.
#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @ +0x2c / Write @ +0x30)

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

    // Linear scan of the 24-dword m_block; 1 on a hit. RE-HOMED from
    // CTileTriggerSwitchLogic (0x8c), where it could not fit: retail does `add ecx,0x3c`
    // then 24 iterations -> this+0x3c..+0x9b, i.e. exactly m_block[0..23] of THIS class.
    // VerifyBlockLinks calls it (ILT 0x1fa5) on the child it then scans at child+0x3c.
    i32 FindIndexByKey(i32 key); // 0x110820

    // The 0x9c family's serialize dispatcher (type 4 = save, 7 = load), and the pair it
    // forwards `this` to. RE-HOMED from CTileTriggerSwitchLogic: CTileTriggerFactory::Build
    // calls ValidateByType (ILT 0x1abe) at 0x117aa7 on a freshly-`new`ed 0x9c
    // CTileTriggerLogic (`push 0x9c; call ??2; mov ecx,eax; call ??0CTileTriggerLogic`).
    i32 ValidateByType(void* archive, i32 type, i32 a3, i32 a4); // 0x113a90
    i32 Serialize(CSerialArchive* s);                            // 0x113ae0
    i32 Deserialize(CSerialArchive* s);                          // 0x113c10

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
    i32 m_dutyOn;                // +0x38  duty-cycle on/off latch (name kept from the folded
                                 //        CTileGridCommand view, which shares this layout)
    i32 m_block[24];             // +0x3c..0x9b  (rep stosl, 24 dwords)
};

// The per-id logic leaves of the CTileTriggerLogic family (base = 1 virtual): each is
// 0x9c bytes and carries its own RTTI vtable. The AddLogic factory (0x116610) news the
// four non-rock leaves; the serialize Build factory (0x117800) constructs the same set
// via its CTrigLogic9c size-bucket. Their ctors are defined (RVA-bound) in
// TileTriggerSwitchLogic.cpp - the header carries only the declarations so both TUs
// share one class shape (dissolved from the old TileTriggerSwitchLogic.cpp-local defs).
// CGiantRockLogic is the ONE leaf of this family that adds data. sizeof = 0xc8, PROVEN from
// the allocation site (CTileTriggerFactory::Build @0x117b49: `push 0xc8; call ??2; mov ecx,eax;
// call ??0CGiantRockLogic`, and again at 0x116d10). The 0x2c it adds over the 0x9c base is
// EXACTLY what SerializeMatrix streams, with zero slack:
//     base 0x9c + m_matrix[9] (0x24) + m_c0 + m_c4 = 0xc8
// Retail hands this object to ApplyByType right after that ctor (`mov ecx,esi; call 0x1d39`),
// which is why ApplyByType/SerializeMatrix/DeserializeMatrix - reaching this+0x9c..+0xc4 -
// belong HERE and not on the 0x8c CTileTriggerSwitchLogic they were filed under.
class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic(); // 0x112210 (ILT 0x2c3e)

    i32 ApplyByType(void* archive, i32 type, i32 a3, i32 a4); // 0x113d40 (ILT 0x1d39)
    i32 SerializeMatrix(CSerialArchive* s);                   // 0x113dd0 (type-4 save)
    i32 DeserializeMatrix(CSerialArchive* s);                 // 0x113e70 (type-7 load)

    i32 m_matrix[9]; // +0x9c..0xbf  3x3, streamed as a nested 3x3 loop
    i32 m_c0;        // +0xc0        streamed FIRST (before the matrix)
    i32 m_c4;        // +0xc4        streamed SECOND; the object ends at 0xc8
};
SIZE(CGiantRockLogic, 0xc8);
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

// TileTriggerLogic.h - Gruntz tile-trigger logic class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the constructor. Offsets are the
// load-bearing fact the match proves.
#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @ +0x2c / Write @ +0x30)

class CTileTriggerContainer; // owner, back-stamped into m_20 (fwd; def in TileTriggerContainer.h)

// The active-player slot index the switch/validation logic reads (an index into
// m_playerFlags[] and g_gameReg->m_options[]). Reloc-masked, no DATA home; declared
// here so both tile-logic TUs reference it from one place, not per-TU externs.
extern i32 g_tileKindMagic;

// The tile-trigger factory/serialize type-id space: the id CTileTriggerContainer::
// LoadElement (0x117800) switches on and stamps into the element (switch family
// m_04 / logic family m_typeTag), re-read by the serialize dispatchers
// (SerializeApplyA: 1..8; SerializeApplyB: 0x15..0x1a), AddLogic (0x116610:
// 0x15..0x1a) and the container finders (FindChild id filter, FindInLists12 tag).
// PROVEN arms only - each value's class is pinned by its `new` in the retail
// switch. Ids 1/2/5 all build the base CTileTriggerSwitchLogic (their gameplay
// distinction is unrecovered, so they keep numeric suffixes, not invented roles).
typedef enum TrigLogicId {
    TRIGID_SWITCH_1 = 1,              // CTileTriggerSwitchLogic
    TRIGID_SWITCH_2 = 2,              // CTileTriggerSwitchLogic
    TRIGID_MULTI_SWITCH_3 = 3,        // CTileMultiTriggerSwitchLogic
    TRIGID_EXCLUSIVE_SWITCH_4 = 4,    // CTileExclusiveTriggerSwitchLogic (Broadcast's filter)
    TRIGID_SWITCH_5 = 5,              // CTileTriggerSwitchLogic
    TRIGID_SECRET_SWITCH_6 = 6,       // CTileSecretTriggerSwitchLogic
    TRIGID_TIME_SWITCH_7 = 7,         // CTileTimeTriggerSwitchLogic
    TRIGID_CHECKPOINT_SWITCH_8 = 8,   // CCheckpointTriggerSwitchLogic (VerifyBlockLinks filter)
    TRIGID_TILE_TRIGGER_21 = 0x15,    // CTileTriggerLogic (the id-21 board-latch arm)
    TRIGID_GIANT_ROCK_22 = 0x16,      // CGiantRockLogic - THE rock discriminant
    TRIGID_TIME_TRIGGER_23 = 0x17,    // CTileTimeTriggerLogic (AddLogic routes to m_list2)
    TRIGID_TILE_TRIGGER_24 = 0x18,    // CTileTriggerLogic
    TRIGID_SECRET_TRIGGER_25 = 0x19,  // CTileSecretTriggerLogic
    TRIGID_COVERED_POWERUP_26 = 0x1a, // CCoveredPowerupLogic (SetCell's fallback probe tag)
} TrigLogicId;

// The two CGruntzMgr::ReportError classes the trigger-link validators raise
// (arg1 of ReportError(class, site)). PROVEN arms only: every 0x80dd site in the
// tree reports a lookup that came back empty (FindChild key miss, the rock-scan
// miss @TriggerMgr 0x403), every 0x80de site a resolved link that failed
// validation (no claiming child). The wider 0x80xx space belongs to CGruntzMgr -
// unproven arms are left unenumerated.
typedef enum TrigErrClass {
    TRIGERR_LOOKUP_MISS = 0x80dd, // a FindChild/registry lookup returned nothing
    TRIGERR_LINK_BROKEN = 0x80de, // a link validation failed (no child claims the switch)
} TrigErrClass;

// The per-site diagnostic tags (arg2) of the trigger-link validators - one per
// PROVEN report site.
typedef enum TrigErrSite {
    TRIGSITE_ROCK_SCAN_MISS = 0x403,  // TriggerMgr rock-break: no giant rock around (x,y)
    TRIGSITE_LINKSB_NO_OWNER = 0x44d, // VerifyBlockLinksB: no m_list1 child claims this switch
    TRIGSITE_LINKSB_KEY_MISS = 0x44e, // VerifyBlockLinksB: block key unresolved (id-3 filter)
    TRIGSITE_BCAST_KEY_MISS = 0x44f,  // Broadcast: block key unresolved (id-4 filter)
    TRIGSITE_BCAST_NO_CLAIM = 0x450,  // Broadcast: no m_list1 child claims the sibling
    TRIGSITE_LINKS_NO_OWNER = 0x452,  // VerifyBlockLinks: no m_list1 child claims this switch
    TRIGSITE_LINKS_KEY_MISS = 0x453,  // VerifyBlockLinks: block key unresolved (id-8 filter)
} TrigErrSite;

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
    // slot 0 (0x110c10 via ILT thunk 0x402072): the duty-edge tick virtual - the
    // pyramid/bridge tile-transition dispatcher, run on this trigger's own (m_08, m_0c)
    // coords. Defined in TileTriggerSwitchLogic.cpp (@early-stop megafunction; it was
    // the old CPlayLevelLoad::LoadPyramidBridge shell - fake receiver + a fabricated
    // spriteType arg; the switch key is really the locally-resolved cell id). `Tick` is
    // the name the folded CTileGridCommand view carried for this same slot-0 virtual.
    virtual i32 Tick();

    // --- folded in from the INVENTED "CTileGridCommand" (see the note below) -------------
    void RecordMove(); // 0x112880
    // Time-driven duty-cycle classifier: +1 while inside the on/off span, 0 on the rising
    // edge of a one-shot, -1 on the falling edge.
    i32 Classify(i32 arg); // 0x112970
    // NOTE: BumpCell (0x112b70) is NOT a member, though the folded view claimed it. The
    // retail vtable ??_7CCheckpointTriggerSwitchLogic@@6B@ (0x1eaf54) holds it in SLOT 2 (and
    // its decrement sibling 0x112bf0 in slot 3), so both are overrides on the 0x8c
    // CTileTriggerSwitchLogic hierarchy. They touch only +0x08/+0x0c/+0x14 - offsets both
    // families share, which is why only the vtable could tell them apart. See
    // CCheckpointTriggerSwitchLogic::SwitchDown/SwitchUp in TileTriggerSwitchLogic.cpp.

    // Edits the tile grid according to a verb arg (set/clear/notify), then reports the move
    // into the in-game text log.
    i32 ApplyMove(i32 verb); // 0x112590

    // Linear scan of the 24-dword m_block; 1 on a hit. RE-HOMED from
    // CTileTriggerSwitchLogic (0x8c), where it could not fit: retail does `add ecx,0x3c`
    // then 24 iterations -> this+0x3c..+0x9b, i.e. exactly m_block[0..23] of THIS class.
    // VerifyBlockLinks calls it (ILT 0x1fa5) on the child it then scans at child+0x3c.
    i32 FindIndexByKey(i32 key); // 0x110820

    // Slot-0 Tick's bridge/pyramid sound-cue helper: dispatch a 0x66-case jump table
    // over (type - 0xf), playing the matching bridge-transition cue for this trigger's
    // own (m_08, m_0c) tile. Body in BridgeMoveSprites.cpp (was the CPlayLevelLoad view).
    void LoadBridgeMove(i32 type); // 0x110860

    // The 0x9c family's serialize dispatcher (type 4 = save, 7 = load), and the pair it
    // forwards `this` to. RE-HOMED from CTileTriggerSwitchLogic: CTileTriggerFactory::Build
    // calls ValidateByType (ILT 0x1abe) at 0x117aa7 on a freshly-`new`ed 0x9c
    // CTileTriggerLogic (`push 0x9c; call ??2; mov ecx,eax; call ??0CTileTriggerLogic`).
    i32 ValidateByType(void* archive, i32 type, i32 a3, i32 a4); // 0x113a90
    i32 Serialize(CSerialArchive* s);                            // 0x113ae0
    i32 Deserialize(CSerialArchive* s);                          // 0x113c10

    // Field names below take the RICHER of the two spellings this class was reconstructed
    // under (the CTileGridCommand view named the tag/coords/duty spans; this one did not).
    i32 m_typeTag;               // +0x04  type tag (0x17/0x18 duty-cycle discriminant)
    i32 m_08;                    // +0x08  coord x
    i32 m_0c;                    // +0x0c  coord y
    i32 m_10;                    // +0x10
    i32 m_14;                    // +0x14  flag
    i32 m_18;                    // +0x18
    i32 m_1c;                    // +0x1c  init flag (zeroed by ctor AFTER m_block; the
                                 //        inlined `delete` in the container walkers zeroes
                                 //        exactly this - which is what proves those list
                                 //        elements are THIS class, not the 0x8c switch logic
                                 //        whose dtor zeroes +0x20)
    CTileTriggerContainer* m_20; // +0x20  owning container
    u32 m_24;                    // +0x24  game-clock snapshot (g_frameTime)
    u32 m_28;                    // +0x28  duty on-span (unsigned duration)
    u32 m_2c;                    // +0x2c  lead-in span (unsigned duration)
    u32 m_30;                    // +0x30  duty off-span (unsigned duration)
    i32 m_34;                    // +0x34
    i32 m_dutyOn;                // +0x38  duty-cycle on/off latch (1 = currently on)
    i32 m_block[24];             // +0x3c..0x9b  (rep stosl, 24 dwords; the "m_grid" of the
                                 //        folded view - same 24 dwords at the same offset)
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

    // The rock-break tile-effect loader (0x1122a0): writes the m_matrix 3x3 back
    // into the level plane, fires the effect + optional InGameText, plays the cue.
    // OWNER SETTLED 2026-07-13: `this` reaches +0x9c..+0xc4 (m_matrix/m_c0/m_c4) -
    // only THIS 0xc8 class holds them; the old CTileTriggerSwitchLogic filing was a
    // Ghidra rtti-vptr guess an 0x8c object cannot satisfy.
    void BuildRockBreakInGameText(); // 0x1122a0

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
    virtual i32 Tick() OVERRIDE; // slot 0
public:
    CTileSecretTriggerLogic(); // 0x112760 (ILT 0x310c)
};
// Size PROVEN from the allocation site (push 0x9c; call ??2 -> the ctor), and our
// reconstruction computes exactly that. Pinned so no future note can claim it unknown.
SIZE(CTileSecretTriggerLogic, 0x9c);
VTBL(CTileSecretTriggerLogic, 0x001eaf14); // vtable_names -> code (RTTI game class)

#endif // TILETRIGGERLOGIC_H

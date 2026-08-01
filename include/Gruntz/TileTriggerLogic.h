#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // CFileMemBase (Read @ +0x2c / Write @ +0x30)

class
    CTileTriggerContainer; // owner, back-stamped into m_owner (fwd; def in TileTriggerContainer.h)

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

// Tile collision kinds - the CTileImageSet/CImageSet1::GetCollisionAt (slot 8) result for
// the tile under an object. The space is dense (~0xb..0x74) and mostly unrecovered; ONLY
// the arms proven from the binary are enumerated here, never a filled-in range.
//
// The 0x5d..0x74 band IS recovered, from CTileTriggerLogic::Tick's jump table (0x110c10,
// tables at 0x511a50/0x511a98): each arm names its own sprite key literal, so the kind is
// read straight off the string, and every kind but the two crumble bridges owns a
// consecutive (DOWN, UP) pair whose UP member is the value the arm compares against
// before selecting "GAME_PYRAMIDUP" / "LEVEL_BRIDGEUP".
typedef enum TileCollisionKind {
    TILEKIND_CHECKPOINTPYRAMID_DOWN = 0x5d, // "GAME_CHECKPOINTPYRAMIDZ"
    TILEKIND_CHECKPOINTPYRAMID_UP = 0x5e,
    TILEKIND_WHITEPYRAMID_DOWN = 0x5f, // "GAME_WHITEPYRAMIDZ"
    TILEKIND_WHITEPYRAMID_UP = 0x60,
    TILEKIND_ORANGEPYRAMID_DOWN = 0x61, // "GAME_ORANGEPYRAMIDZ"
    TILEKIND_ORANGEPYRAMID_UP = 0x62,
    TILEKIND_BLACKPYRAMID_DOWN = 0x63, // "GAME_BLACKPYRAMIDZ"
    TILEKIND_BLACKPYRAMID_UP = 0x64,
    TILEKIND_GREENPYRAMID_DOWN = 0x65, // "GAME_GREENPYRAMIDZ"
    TILEKIND_GREENPYRAMID_UP = 0x66,
    // The two PYRAMID-band kinds (the whole 0x5d..0x6a band plays GAME_PYRAMIDMOVE in
    // CTileTriggerLogic::LoadBridgeMove @0x110860) that make an id-21 trigger the level's
    // single latched leaf: AddLogic (0x116610) and LoadElement (0x117800) both stamp
    // CTileTriggerContainer::m_latchedLeaf on exactly {0x67, 0x68}, and
    // CBattlezMapConfig::PathToNearestGoal (0x30b20) reads that latch in place of a
    // per-cell FindInLists12 when a board cell's marker is 0x67. Which state each denotes
    // IS now recovered: Tick's whole-grid red sweep plays "GAME_PYRAMIDUP" on 0x68 and
    // "GAME_PYRAMIDDOWN" otherwise.
    TILEKIND_PYRAMID_LATCH_A = 0x67,
    TILEKIND_PYRAMID_LATCH_B = 0x68,
    TILEKIND_REDPYRAMID_DOWN = 0x67,    // "GAME_REDPYRAMIDZ" (== TILEKIND_PYRAMID_LATCH_A)
    TILEKIND_REDPYRAMID_UP = 0x68,      //                    (== TILEKIND_PYRAMID_LATCH_B)
    TILEKIND_PURPLEPYRAMID_DOWN = 0x69, // "GAME_PURPLEPYRAMIDZ"
    TILEKIND_PURPLEPYRAMID_UP = 0x6a,
    TILEKIND_WATERBRIDGE_DOWN = 0x6b, // "LEVEL_WATERBRIDGE"
    TILEKIND_WATERBRIDGE_UP = 0x6c,
    TILEKIND_DEATHBRIDGE_DOWN = 0x6d, // "LEVEL_DEATHBRIDGE"
    TILEKIND_DEATHBRIDGE_UP = 0x6e,
    TILEKIND_CRUMBLEWATERBRIDGE = 0x6f,     // "LEVEL_CRUMBLEWATERBRIDGE" (single state)
    TILEKIND_CRUMBLEDEATHBRIDGE = 0x70,     // "LEVEL_CRUMBLEDEATHBRIDGE" (single state)
    TILEKIND_TOGGLEWATERBRIDGE_DOWN = 0x71, // "LEVEL_TOGGLEWATERBRIDGE"
    TILEKIND_TOGGLEWATERBRIDGE_UP = 0x72,
    TILEKIND_TOGGLEDEATHBRIDGE_DOWN = 0x73, // "LEVEL_TOGGLEDEATHBRIDGE"
    TILEKIND_TOGGLEDEATHBRIDGE_UP = 0x74,
} TileCollisionKind;

typedef enum TrigErrClass {
    TRIGERR_LOOKUP_MISS = 0x80dd, // a FindChild/registry lookup returned nothing
    TRIGERR_LINK_BROKEN = 0x80de, // a link validation failed (no child claims the switch)
} TrigErrClass;

typedef enum TrigErrSite {
    // CTriggerMgr::ApplySwitch (0x6d300): one site id per switch-kind arm, in the arm's
    // source order (they are also the /GX EH states 0..6 of the seven CString scopes).
    TRIGSITE_APPLY_SWITCH_40 = 0x3f7,  // kind 0x40: FindChild(key, 7) miss
    TRIGSITE_APPLY_SWITCH_34 = 0x3f8,  // kind 0x34: FindChild(key, 0) miss
    TRIGSITE_APPLY_SWITCH_36 = 0x3f9,  // kind 0x36: FindChild(key, 0) miss
    TRIGSITE_APPLY_TRIGGER_36 = 0x3fa, // kind 0x36: no m_list1 child claims the switch
    TRIGSITE_APPLY_SWITCH_38 = 0x3fb,  // kind 0x38: FindChild(key, 3) miss
    TRIGSITE_APPLY_TRIGGER_38 = 0x3fc, // kind 0x38: no m_list1 child claims the switch
    TRIGSITE_APPLY_SWITCH_42 = 0x3fd,  // kind 0x42: FindChild(key, 8) miss
    TRIGSITE_ROCK_SCAN_MISS = 0x403,   // TriggerMgr rock-break: no giant rock around (x,y)
    TRIGSITE_LINKSB_NO_OWNER = 0x44d,  // VerifyBlockLinksB: no m_list1 child claims this switch
    TRIGSITE_LINKSB_KEY_MISS = 0x44e,  // VerifyBlockLinksB: block key unresolved (id-3 filter)
    TRIGSITE_BCAST_KEY_MISS = 0x44f,   // Broadcast: block key unresolved (id-4 filter)
    TRIGSITE_BCAST_NO_CLAIM = 0x450,   // Broadcast: no m_list1 child claims the sibling
    TRIGSITE_LINKS_NO_OWNER = 0x452,   // VerifyBlockLinks: no m_list1 child claims this switch
    TRIGSITE_LINKS_KEY_MISS = 0x453,   // VerifyBlockLinks: block key unresolved (id-8 filter)
} TrigErrSite;

VTBL(CTileTriggerLogic, 0x001eaea4); // vtable_names -> code (RTTI game class)
class CTileTriggerLogic {
public:
    CTileTriggerLogic();
    ~CTileTriggerLogic() {
        m_initGate = 0;
    }
    // slot 0 (0x110c10 via ILT thunk 0x402072): the duty-edge tick virtual - the
    // pyramid/bridge tile-transition dispatcher, run on this trigger's own (m_tileX, m_tileY)
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
    // own (m_tileX, m_tileY) tile. Body in BridgeMoveSprites.cpp.
    void LoadBridgeMove(i32 type); // 0x110860

    // The 0x9c family's serialize dispatcher (type 4 = save, 7 = load), and the pair it
    // forwards `this` to. RE-HOMED from CTileTriggerSwitchLogic: CTileTriggerFactory::Build
    // calls ValidateByType (ILT 0x1abe) at 0x117aa7 on a freshly-`new`ed 0x9c
    // CTileTriggerLogic (`push 0x9c; call ??2; mov ecx,eax; call ??0CTileTriggerLogic`).
    i32 ValidateByType(void* archive, i32 mode, i32 typeId, i32 pObj); // 0x113a90
    i32 Serialize(CFileMemBase* s);                                    // 0x113ae0
    i32 Deserialize(CFileMemBase* s);                                  // 0x113c10

    // Field names below take the RICHER of the two spellings this class was reconstructed
    // under (the CTileGridCommand view named the tag/coords/duty spans; this one did not).
    i32 m_typeTag;                  // +0x04  type tag (0x17/0x18 duty-cycle discriminant)
    i32 m_tileX;                    // +0x08  coord x
    i32 m_tileY;                    // +0x0c  coord y
    i32 m_10;                       // +0x10
    i32 m_14;                       // +0x14  flag
    i32 m_18;                       // +0x18
    i32 m_initGate;                 // +0x1c  init flag (zeroed by ctor AFTER m_block; the
                                    //        inlined `delete` in the container walkers zeroes
                                    //        exactly this - which is what proves those list
                                    //        elements are THIS class, not the 0x8c switch logic
                                    //        whose dtor zeroes +0x20)
    CTileTriggerContainer* m_owner; // +0x20  owning container
    u32 m_startClock;               // +0x24  game-clock snapshot (g_frameTime)
    u32 m_dutyOnSpan;               // +0x28  duty on-span (unsigned duration)
    u32 m_leadInSpan;               // +0x2c  lead-in span (unsigned duration)
    u32 m_dutyOffSpan;              // +0x30  duty off-span (unsigned duration)
    i32 m_tileToken;                // +0x34
    i32 m_dutyOn;                   // +0x38  duty-cycle on/off latch (1 = currently on)
    i32 m_block[24];                // +0x3c..0x9b  (rep stosl, 24 dwords; the "m_grid" of the
                                    //        folded view - same 24 dwords at the same offset)
};
SIZE(0x9c);

class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic(); // 0x112210 (ILT 0x2c3e)

    // The rock-break tile-effect loader (0x1122a0): writes the m_matrix 3x3 back
    // into the level plane, fires the effect + optional InGameText, plays the cue.
    // OWNER SETTLED 2026-07-13: `this` reaches +0x9c..+0xc4 (m_matrix/m_c0/m_c4) -
    // only THIS 0xc8 class holds them; the old CTileTriggerSwitchLogic filing was a
    // Ghidra rtti-vptr guess an 0x8c object cannot satisfy.
    void BuildRockBreakInGameText(); // 0x1122a0

    i32 ApplyByType(void* archive, i32 mode, i32 typeId, i32 pObj); // 0x113d40 (ILT 0x1d39)
    i32 SerializeMatrix(CFileMemBase* s);                           // 0x113dd0 (type-4 save)
    i32 DeserializeMatrix(CFileMemBase* s);                         // 0x113e70 (type-7 load)

    i32 m_matrix[9];   // +0x9c..0xbf  3x3, streamed as a nested 3x3 loop
    i32 m_powerupType; // +0xc0        streamed FIRST (before the matrix)
    i32 m_textId;      // +0xc4        streamed SECOND; the object ends at 0xc8
};
SIZE(0xc8);
VTBL(CGiantRockLogic, 0x001eaee4); // vtable_names -> code (RTTI game class)

class CCoveredPowerupLogic : public CTileTriggerLogic {
public:
    CCoveredPowerupLogic(); // 0x112240 (ILT 0x2a4f)
};
SIZE(0x9c);
VTBL(CCoveredPowerupLogic, 0x001eaef4); // vtable_names -> code (RTTI game class)

class CTileTimeTriggerLogic : public CTileTriggerLogic {
public:
    CTileTimeTriggerLogic(); // 0x112270 (ILT 0x18de)
};
SIZE(0x9c);
VTBL(CTileTimeTriggerLogic, 0x001eaf04); // vtable_names -> code (RTTI game class)

class CTileSecretTriggerLogic : public CTileTriggerLogic {
    virtual i32 Tick() OVERRIDE; // slot 0
public:
    CTileSecretTriggerLogic(); // 0x112760 (ILT 0x310c)
};
SIZE(0x9c);
VTBL(CTileSecretTriggerLogic, 0x001eaf14); // vtable_names -> code (RTTI game class)

#endif // TILETRIGGERLOGIC_H

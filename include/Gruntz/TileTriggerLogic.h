#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <rva.h>

#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CTileTriggerContainer;

typedef enum TrigLogicId {
    TRIGID_SWITCH_1 = 1,
    TRIGID_SWITCH_2 = 2,
    TRIGID_MULTI_SWITCH_3 = 3,
    TRIGID_EXCLUSIVE_SWITCH_4 = 4,
    TRIGID_SWITCH_5 = 5,
    TRIGID_SECRET_SWITCH_6 = 6,
    TRIGID_TIME_SWITCH_7 = 7,
    TRIGID_CHECKPOINT_SWITCH_8 = 8,
    TRIGID_TILE_TRIGGER_21 = 0x15,
    TRIGID_GIANT_ROCK_22 = 0x16,
    TRIGID_TIME_TRIGGER_23 = 0x17,
    TRIGID_TILE_TRIGGER_24 = 0x18,
    TRIGID_SECRET_TRIGGER_25 = 0x19,
    TRIGID_COVERED_POWERUP_26 = 0x1a,
} TrigLogicId;

typedef enum TileCollisionKind {
    TILEKIND_ARROW_UP_A = 0x0b,
    TILEKIND_ARROW_DOWN_A = 0x0c,
    TILEKIND_ARROW_LEFT_A = 0x0d,
    TILEKIND_ARROW_RIGHT_A = 0x0e,
    TILEKIND_ARROW_UP_B = 0x0f,
    TILEKIND_ARROW_DOWN_B = 0x10,
    TILEKIND_ARROW_LEFT_B = 0x11,
    TILEKIND_ARROW_RIGHT_B = 0x12,
    TILEKIND_ARROW_CURRENT = 0x13,

    TILEKIND_GAUNTLET_ROCK_A = 0x1e,
    TILEKIND_GAUNTLET_ROCK_B = 0x1f,
    TILEKIND_GIANT_ROCK = 0x21,
    TILEKIND_COVERED_POWERUP = 0x22,
    TILEKIND_REVEALED_POWERUP = 0x23,

    TILEKIND_SWITCH_A = 0x33,
    TILEKIND_SWITCH_B = 0x35,
    TILEKIND_MULTI_SWITCH = 0x37,
    TILEKIND_SWITCH_C = 0x39,
    TILEKIND_EXCLUSIVE_SWITCH = 0x3b,
    TILEKIND_SECRET_SWITCH = 0x3d,
    TILEKIND_TIME_SWITCH = 0x3f,
    TILEKIND_CHECKPOINT = 0x41,

    TILEKIND_HIDDEN_POWERUP = 0x96,
    TILEKIND_GAUNTLET_BRICK_A = 0x97,
    TILEKIND_GAUNTLET_BRICK_B = 0x98,
    TILEKIND_GAUNTLET_BRICK_C = 0x99,

    TILEKIND_CHECKPOINTPYRAMID_DOWN = 0x5d,
    TILEKIND_CHECKPOINTPYRAMID_UP = 0x5e,
    TILEKIND_WHITEPYRAMID_DOWN = 0x5f,
    TILEKIND_WHITEPYRAMID_UP = 0x60,
    TILEKIND_ORANGEPYRAMID_DOWN = 0x61,
    TILEKIND_ORANGEPYRAMID_UP = 0x62,
    TILEKIND_BLACKPYRAMID_DOWN = 0x63,
    TILEKIND_BLACKPYRAMID_UP = 0x64,
    TILEKIND_GREENPYRAMID_DOWN = 0x65,
    TILEKIND_GREENPYRAMID_UP = 0x66,

    TILEKIND_PYRAMID_LATCH_A = 0x67,
    TILEKIND_PYRAMID_LATCH_B = 0x68,
    TILEKIND_REDPYRAMID_DOWN = 0x67,
    TILEKIND_REDPYRAMID_UP = 0x68,
    TILEKIND_PURPLEPYRAMID_DOWN = 0x69,
    TILEKIND_PURPLEPYRAMID_UP = 0x6a,
    TILEKIND_WATERBRIDGE_DOWN = 0x6b,
    TILEKIND_WATERBRIDGE_UP = 0x6c,
    TILEKIND_DEATHBRIDGE_DOWN = 0x6d,
    TILEKIND_DEATHBRIDGE_UP = 0x6e,
    TILEKIND_CRUMBLEWATERBRIDGE = 0x6f,
    TILEKIND_CRUMBLEDEATHBRIDGE = 0x70,
    TILEKIND_TOGGLEWATERBRIDGE_DOWN = 0x71,
    TILEKIND_TOGGLEWATERBRIDGE_UP = 0x72,
    TILEKIND_TOGGLEDEATHBRIDGE_DOWN = 0x73,
    TILEKIND_TOGGLEDEATHBRIDGE_UP = 0x74,
} TileCollisionKind;

typedef enum TrigErrClass {
    TRIGERR_LOOKUP_MISS = 0x80dd,
    TRIGERR_LINK_BROKEN = 0x80de,
} TrigErrClass;

typedef enum TrigErrSite {
    TRIGSITE_WIRE_TIME_SWITCH = 0x3eb,
    TRIGSITE_WIRE_TIME_TRIGGER = 0x3ec,
    TRIGSITE_WIRE_SECRET_SWITCH = 0x3ed,
    TRIGSITE_WIRE_SECRET_TRIGGER = 0x3ee,
    TRIGSITE_WIRE_SWITCH = 0x3ef,
    TRIGSITE_WIRE_TRIGGER = 0x3f0,
    TRIGSITE_WIRE_MULTI_SWITCH = 0x3f1,
    TRIGSITE_WIRE_MULTI_TRIGGER = 0x3f2,
    TRIGSITE_WIRE_EXCLUSIVE_SWITCH = 0x3f3,
    TRIGSITE_WIRE_EXCLUSIVE_TRIGGER = 0x3f4,
    TRIGSITE_WIRE_CHECKPOINT = 0x3f5,
    TRIGSITE_WIRE_CHECKPOINT_TRIGGER = 0x3f6,
    TRIGSITE_ARRIVAL_GIANT_ROCK = 0x3fe,

    TRIGSITE_APPLY_SWITCH_40 = 0x3f7,
    TRIGSITE_APPLY_SWITCH_34 = 0x3f8,
    TRIGSITE_APPLY_SWITCH_36 = 0x3f9,
    TRIGSITE_APPLY_TRIGGER_36 = 0x3fa,
    TRIGSITE_APPLY_SWITCH_38 = 0x3fb,
    TRIGSITE_APPLY_TRIGGER_38 = 0x3fc,
    TRIGSITE_APPLY_SWITCH_42 = 0x3fd,
    TRIGSITE_ROCK_SCAN_MISS = 0x403,
    TRIGSITE_LINKSB_NO_OWNER = 0x44d,
    TRIGSITE_LINKSB_KEY_MISS = 0x44e,
    TRIGSITE_BCAST_KEY_MISS = 0x44f,
    TRIGSITE_BCAST_NO_CLAIM = 0x450,
    TRIGSITE_LINKS_NO_OWNER = 0x452,
    TRIGSITE_LINKS_KEY_MISS = 0x453,
} TrigErrSite;

VTBL(CTileTriggerLogic, 0x001eaea4);
class CTileTriggerLogic {
public:
    CTileTriggerLogic();
    ~CTileTriggerLogic() {
        m_initGate = 0;
    }

    virtual i32 Tick();

    void RecordMove();

    i32 Classify(i32 arg);

    i32 ApplyMove(i32 verb);

    i32 FindIndexByKey(i32 key);

    void LoadBridgeMove(i32 type);

    i32 ValidateByType(void* archive, i32 mode, i32 typeId, i32 pObj);
    i32 Serialize(CFileMemBase* s);
    i32 Deserialize(CFileMemBase* s);

    i32 m_typeTag;
    i32 m_tileX;
    i32 m_tileY;
    i32 m_cellKey;
    i32 m_reserved14;
    i32 m_reserved18;
    i32 m_initGate;

    CTileTriggerContainer* m_owner;
    u32 m_startClock;
    u32 m_dutyOnSpan;
    u32 m_leadInSpan;
    u32 m_dutyOffSpan;
    i32 m_tileToken;
    i32 m_dutyOn;
    i32 m_linkKeys[24];
};
SIZE(0x9c);

class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic();

    void BuildRockBreakInGameText();

    i32 ApplyByType(void* archive, i32 mode, i32 typeId, i32 pObj);
    i32 SerializeMatrix(CFileMemBase* s);
    i32 DeserializeMatrix(CFileMemBase* s);

    i32 m_matrix[9];
    i32 m_powerupType;
    i32 m_textId;
};
SIZE(0xc8);
VTBL(CGiantRockLogic, 0x001eaee4);

class CCoveredPowerupLogic : public CTileTriggerLogic {
public:
    CCoveredPowerupLogic();
};
SIZE(0x9c);
VTBL(CCoveredPowerupLogic, 0x001eaef4);

class CTileTimeTriggerLogic : public CTileTriggerLogic {
public:
    CTileTimeTriggerLogic();
};
SIZE(0x9c);
VTBL(CTileTimeTriggerLogic, 0x001eaf04);

class CTileSecretTriggerLogic : public CTileTriggerLogic {
    virtual i32 Tick() OVERRIDE;

public:
    CTileSecretTriggerLogic();
};
SIZE(0x9c);
VTBL(CTileSecretTriggerLogic, 0x001eaf14);

#endif // TILETRIGGERLOGIC_H

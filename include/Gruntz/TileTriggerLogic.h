#ifndef TILETRIGGERLOGIC_H
#define TILETRIGGERLOGIC_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>

class CTileTriggerContainer;
struct tagRECT;

GZ_ENUM_BEGIN(TrigLogicId)
// Wildcard: the container lookups read `if (k2 == 0 || m_typeId == k2)`,
// so 0 means "any type" rather than a type of its own.
    TRIGID_ANY = 0,
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
    TRIGID_COVERED_POWERUP_26 = 0x1a
GZ_ENUM_END(TrigLogicId)

// The diagnostic ids CGruntzMgr::ReportError takes. Its parameter is NOT
// this domain - the WARP macro passes 0x46c-range ids through the same
// slot - so these leave the type system with IDX() at the call.
GZ_ENUM_BEGIN(TrigErrClass)
    TRIGERR_LOOKUP_MISS = 0x80dd,
    TRIGERR_LINK_BROKEN = 0x80de
GZ_ENUM_END(TrigErrClass)

GZ_ENUM_BEGIN(TrigErrSite)
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
    TRIGSITE_LINKS_KEY_MISS = 0x453
GZ_ENUM_END(TrigErrSite)

class CTileTriggerLogic {
public:
    CTileTriggerLogic();
    ~CTileTriggerLogic() {
        m_initGate = 0;
    }

    virtual i32 Tick();

    void RecordMove();

    i32 Classify(i32 unusedFrameDelta);

    i32 ApplyMove(TileCollisionKind verb);

    i32 FindIndexByKey(i32 key);

    void LoadBridgeMove(TileCollisionKind type);

    i32 Build(
        CTileTriggerContainer* owner,
        TrigLogicId typeTag,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        const tagRECT* rects,
        i32 tileToken,
        i32 dutyOnSpan,
        i32 leadInSpan,
        i32 dutyOffSpan
    );

    i32 Setup(
        CTileTriggerContainer* owner,
        TrigLogicId typeTag,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        i32 tileToken,
        i32 dutyOnSpan,
        i32 leadInSpan,
        i32 dutyOffSpan
    );

    i32 ValidateByType(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 Serialize(CFileMemBase* s);
    i32 Deserialize(CFileMemBase* s);

    TrigLogicId m_typeTag;
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

class CGiantRockLogic : public CTileTriggerLogic {
public:
    CGiantRockLogic();

    i32 BuildRockBreakInGameText();

    i32 ApplyByType(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 SerializeMatrix(CFileMemBase* s);
    i32 DeserializeMatrix(CFileMemBase* s);

    i32 m_matrix[9];
    PickupType m_powerupType;
    i32 m_textId;
};

class CCoveredPowerupLogic : public CTileTriggerLogic {
public:
    CCoveredPowerupLogic();
};

class CTileTimeTriggerLogic : public CTileTriggerLogic {
public:
    CTileTimeTriggerLogic();
};

class CTileSecretTriggerLogic : public CTileTriggerLogic {
    virtual i32 Tick() OVERRIDE;

public:
    CTileSecretTriggerLogic();
};

#endif // TILETRIGGERLOGIC_H

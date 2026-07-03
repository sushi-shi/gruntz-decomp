// EyeCandyTypeTags.cpp - per-class logic-type id accessors (GetTypeTag) for four
// tile-logic leaf classes whose owning class name is not yet pinned: their plain
// dtors are COMDAT-folded with siblings, so RTTI can't attribute them. The returned
// id IS the recovered fact; class names here are placeholders, to be re-homed into
// the real class TU once identified. Each is the 6-byte `mov eax,<id>; ret`
// archetype shared with CBehindCandy::GetTypeTag (0x00fb70). Owner hints (nearest
// reconstructed neighbor) noted per entry.
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

// 0xfa40 (id 0x3ef): eyecandy leaf between CSimpleAnimation and CBehindCandy.
class CTileLogicTag3ef : public CUserLogic {
public:
    i32 GetTypeTag();
};
RVA(0x0000fa40, 0x6)
i32 CTileLogicTag3ef::GetTypeTag() {
    return LOGIC_TAG_3EF; // 0x3ef
}

// 0x10f00 (id 0x429): tile-logic leaf after CFortressFlag.
class CTileLogicTag429 : public CUserLogic {
public:
    i32 GetTypeTag();
};
RVA(0x00010f00, 0x6)
i32 CTileLogicTag429::GetTypeTag() {
    return LOGIC_TAG_429; // 0x429
}

// 0x11bf0 (id 0x428): HUD/sprite leaf near CCursorSnapSprite.
class CTileLogicTag428 : public CUserLogic {
public:
    i32 GetTypeTag();
};
RVA(0x00011bf0, 0x6)
i32 CTileLogicTag428::GetTypeTag() {
    return LOGIC_TAG_428; // 0x428
}

// 0x12ff0 (id 0x41d): tile-logic leaf between CRollingBall and CKitchenSlime.
class CTileLogicTag41d : public CUserLogic {
public:
    i32 GetTypeTag();
};
RVA(0x00012ff0, 0x6)
i32 CTileLogicTag41d::GetTypeTag() {
    return LOGIC_TAG_41D; // 0x41d
}

SIZE_UNKNOWN(CTileLogicTag3ef);
SIZE_UNKNOWN(CTileLogicTag429);
SIZE_UNKNOWN(CTileLogicTag428);
SIZE_UNKNOWN(CTileLogicTag41d);

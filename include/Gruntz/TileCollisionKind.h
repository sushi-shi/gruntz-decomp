#ifndef GRUNTZ_TILECOLLISIONKIND_H
#define GRUNTZ_TILECOLLISIONKIND_H

#include <Enums.h>

// The WWD tile-attribute value - what CTileImageSet::GetCollisionAt(x, y)
// returns for a cell.
//
// ONE value space, previously spelled twice: `TileCollision` (0-4, the generic
// walk band, GameLevel.h) and `TileCollisionKind` (0x0b-0x99, the specific
// tiles, TileTriggerLogic.h). The proof they are one domain is that the SAME
// read feeds both: CGameLevel::AxisProbe returns GetCollisionAt(subX, subY) and
// is compared against TILEKIND_HARD (3), while CGameLevel::LookupTile returns
// GetCollisionAt(0, 0) and is compared against TILEKIND_TOGGLEWATERBRIDGE_UP
// (0x72) in CTileTriggerLogic::Tick. CTriggerMgr::ApplySwitchAt gates the same
// value with `(u32)(tag - 0xb) > 0x65` before dispatching the special tiles,
// which is the band boundary between the two halves.
GZ_ENUM_BEGIN(TileCollisionKind)
// The generic walk band the movement code tests.
    TILEKIND_PASSABLE = 0,
    TILEKIND_SOFT = 1,
    TILEKIND_SOFT2 = 2,
    TILEKIND_HARD = 3,
    TILEKIND_SPECIAL = 4,

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

    // Switch tiles come in DOWN/UP pairs, odd then even. CTriggerMgr::TileDown
    // dispatches the odd value to SwitchDown() and CTriggerMgr::TileUp the even
    // one to SwitchUp(), both resolving the switch through the SAME TrigLogicId
    // (0x3f/0x40 -> TRIGID_TIME_SWITCH_7, 0x37/0x38 -> TRIGID_MULTI_SWITCH_3,
    // 0x41/0x42 -> TRIGID_CHECKPOINT_SWITCH_8). Only the four UP values retail
    // actually dispatches are named; 0x3a/0x3c/0x3e are never referenced.
    TILEKIND_SWITCH_A = 0x33,
    TILEKIND_SWITCH_A_UP = 0x34,
    TILEKIND_SWITCH_B = 0x35,
    TILEKIND_SWITCH_B_UP = 0x36,
    TILEKIND_MULTI_SWITCH = 0x37,
    TILEKIND_MULTI_SWITCH_UP = 0x38,
    TILEKIND_SWITCH_C = 0x39,
    TILEKIND_EXCLUSIVE_SWITCH = 0x3b,
    TILEKIND_SECRET_SWITCH = 0x3d,
    TILEKIND_TIME_SWITCH = 0x3f,
    TILEKIND_TIME_SWITCH_UP = 0x40,
    TILEKIND_CHECKPOINT = 0x41,
    TILEKIND_CHECKPOINT_UP = 0x42,

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

    TILEKIND_HIDDEN_POWERUP = 0x96,
    TILEKIND_GAUNTLET_BRICK_A = 0x97,
    TILEKIND_GAUNTLET_BRICK_B = 0x98,
    TILEKIND_GAUNTLET_BRICK_C = 0x99
GZ_ENUM_END(TileCollisionKind)

#endif // GRUNTZ_TILECOLLISIONKIND_H

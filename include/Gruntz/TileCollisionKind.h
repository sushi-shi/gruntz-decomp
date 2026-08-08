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
// is compared against TILEKIND_CLIMB (3), while CGameLevel::LookupTile returns
// GetCollisionAt(0, 0) and is compared against TILEKIND_TOGGLEWATERBRIDGE_UP
// (0x72) in CTileTriggerLogic::Tick. CTriggerMgr::ApplySwitchAt gates the same
// value with `(u32)(tag - 0xb) > 0x65` before dispatching the special tiles,
// which is the band boundary between the two halves.
GZ_ENUM_BEGIN(TileCollisionKind)
// 0..4 is the WAP-editor walk band, and the names below are the editor's own
// (docs/reference/gooroosgruntz/editor/TileAttributez.html: Clear / Solid /
// Ground / Climb / Death), each re-proven against retail before adoption:
//
//   0 blocks nothing. Also what AxisProbe/LookupTile return for an absent
//     cell, which is why the enumerator keeps the behavioural spelling
//     PASSABLE - TILE_CLEAR already means the 0xffffffff tile HANDLE, and two
//     `CLEAR`s in `tile == TILE_CLEAR ? TILEKIND_CLEAR : ...` would read as
//     one thing.
//   1 blocks on every axis: StepAxisLo/StepAxisHi (horizontal),
//     ResolveCeilingCollision (up) and ResolveFloorCollision/FreeMove (down)
//     all test it. Solid.
//   2 is tested ONLY on the floor paths, and MoveStepXHi @0x167352 rewrites
//     it to 0 when the mover has flag 0x400 (`cmp eax,0x2; test ch,0x4; xor
//     eax,eax`). A one-way platform: stand on it, walk and jump through it.
//   3 blocks exactly like 2 EXCEPT when the mover is already MOVE_CLIMBING
//     (`m_moveMode != MOVE_CLIMBING && result == 3` in FreeMove and
//     ResolveFloorCollision; retail `cmp eax,0x3` twice in MoveGrounded
//     @0x15e130). Solid to walkers, transparent to climbers.
//   4 is not backed off - it is walked onto and kills: ResolveFloorCollision
//     sets m_flags 0x400000, the cell classifier gives it bit 0x2 where 1
//     gets 0x1, and CGrunt/CRollingBall handle it in the same switch arm as
//     TILEKIND_DEATHBRIDGE_UP / TILEKIND_TOGGLEDEATHBRIDGE_UP.
//
// Corpus check over all 63 shipped WWDs (57 330 attribute records): value 1
// occupies exactly indices 39..76 and value 4 indices 102..139, against the
// editor reference's "Solid #39->#76" and "Death #99->#140". Values 2 and 3
// are near-unused - 2 at indices 69/74/180/181, 3 at 162/167 - which is why
// the editor page says "there don't appear to be any of these".
    TILEKIND_PASSABLE = 0,
    TILEKIND_SOLID = 1,
    TILEKIND_GROUND = 2,
    TILEKIND_CLIMB = 3,
    TILEKIND_DEATH = 4,

    // Open water. CMapMgr::ComputeCellFlags gives it cell bit 0x100 on its own,
    // and 0x100 is the bit CTriggerMgr::PlaceObject requires before it will drop
    // a Toob grunt (typeKind 0x12) on a cell. CRollingBall::Update groups it with
    // TILEKIND_WATERBRIDGE_UP (0x6c) and TILEKIND_TOGGLEWATERBRIDGE_UP (0x72),
    // whose arm plays "LEVEL_ROLLINGBALLSINKWATER" + a "GAME_WATER" splash.
    TILEKIND_WATER = 0x0a,

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
    // Periodic contact damage (5 on easy single-player, otherwise 10), with
    // gravity bootz and invulnerable Gruntz bypassing the damage path.
    TILEKIND_SPIKES = 0x20,
    TILEKIND_GIANT_ROCK = 0x21,
    TILEKIND_COVERED_POWERUP = 0x22,
    TILEKIND_REVEALED_POWERUP = 0x23,
    // A non-water sink hazard: it sinks rolling balls through the water effect
    // path, but its cell classification bit is 0x800 rather than water's 0x100.
    TILEKIND_SINK_HAZARD = 0x24,

    // Switch tiles come in DOWN/UP pairs, odd then even. CTriggerMgr::TileDown
    // dispatches the odd value to SwitchDown() and CTriggerMgr::TileUp the even
    // one to SwitchUp(), both resolving the switch through the SAME TrigLogicId
    // (0x3f/0x40 -> TRIGID_TIME_SWITCH_7, 0x37/0x38 -> TRIGID_MULTI_SWITCH_3,
    // 0x41/0x42 -> TRIGID_CHECKPOINT_SWITCH_8). CMapMgr::ComputeCellFlags proves
    // 0x33-0x42 is ONE contiguous band: all sixteen arms store the same cell bit
    // 0x4, which is what fixes the three UP partners nothing else dispatches.
    TILEKIND_SWITCH_A = 0x33,
    TILEKIND_SWITCH_A_UP = 0x34,
    TILEKIND_SWITCH_B = 0x35,
    TILEKIND_SWITCH_B_UP = 0x36,
    TILEKIND_MULTI_SWITCH = 0x37,
    TILEKIND_MULTI_SWITCH_UP = 0x38,
    TILEKIND_SWITCH_C = 0x39,
    TILEKIND_SWITCH_C_UP = 0x3a,
    TILEKIND_EXCLUSIVE_SWITCH = 0x3b,
    TILEKIND_EXCLUSIVE_SWITCH_UP = 0x3c,
    TILEKIND_SECRET_SWITCH = 0x3d,
    TILEKIND_SECRET_SWITCH_UP = 0x3e,
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
    TILEKIND_TOGGLE_BRIDGE_FIRST = 0x71,
    TILEKIND_TOGGLE_BRIDGE_LAST = 0x74,

    TILEKIND_HIDDEN_POWERUP = 0x96,
    TILEKIND_GAUNTLET_BRICK_A = 0x97,
    TILEKIND_GAUNTLET_BRICK_B = 0x98,
    TILEKIND_GAUNTLET_BRICK_C = 0x99,
    // Battlez pathfinding rejects this value alongside its blocked-cell masks;
    // the cell classifier gives it 0x2001 rather than the 0x6021 of 0x97..0x99.
    TILEKIND_AI_PATH_BLOCKER = 0x9a
GZ_ENUM_END(TileCollisionKind)

#endif // GRUNTZ_TILECOLLISIONKIND_H

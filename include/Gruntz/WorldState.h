// WorldState.h - the world/level-select state CWorldState and its LevelMgr member.
// Both were viewed per-TU in WorldLevelPath.cpp (BuildWorldLevelPath, the full field
// set) and WorldLevelKey.cpp (BuildWorldLevelKey, a subset); folded here (wave 3) to
// the union. Field names are placeholders; only offsets + code bytes are load-bearing.
#ifndef SRC_GRUNTZ_WORLDSTATE_H
#define SRC_GRUNTZ_WORLDSTATE_H

#include <Ints.h>
#include <rva.h>

class CWorldObj;  // WorldLevelPath.cpp-local (CWorldState::m_4)
class CSymTab;    // <Bute/SymTab.h>
class CGameLevel; // <Gruntz/GameLevel.h>

// The world level-set manager (CWorldState::m_0c); its +0x24 is the CGameLevel.
SIZE_UNKNOWN(LevelMgr);
struct LevelMgr {
    char m_pad00[0x24];
    CGameLevel* m_24; // +0x24
};

SIZE_UNKNOWN(CWorldState);
class CWorldState {
public:
    i32 BuildWorldLevelPath(i32 unused); // 0x0dbc80 (WorldLevelPath.cpp)
    i32 BuildWorldLevelKey(i32 unused);  // (WorldLevelKey.cpp)

    char m_pad00[0x4];
    CWorldObj* m_4; // +0x04
    char m_pad08[0xc - 0x8];
    LevelMgr* m_0c; // +0x0c
    char m_pad10[0x1c - 0x10];
    i32 m_1c; // +0x1c  level number
    char m_pad20[0x28 - 0x20];
    CSymTab* m_28; // +0x28  level-record symbol table
    char m_pad2c[0x34 - 0x2c];
    CSymTab* m_34; // +0x34  battlez/multi symbol table
};

#endif // SRC_GRUNTZ_WORLDSTATE_H

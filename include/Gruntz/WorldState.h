#ifndef SRC_GRUNTZ_WORLDSTATE_H
#define SRC_GRUNTZ_WORLDSTATE_H

#include <Ints.h>
#include <rva.h>

class CGruntzMgr; // Gruntz/GruntzMgr.h (CWorldState::m_4 IS the game mgr)
class CSymTab;    // <Bute/SymTab.h>
class CGameLevel; // <Gruntz/GameLevel.h>

struct LevelMgr {
    char m_pad00[0x24];
    CGameLevel* m_24; // +0x24
};
SIZE_UNKNOWN();

class CWorldState {
public:
    i32 BuildWorldLevelPath(i32 unused); // 0x0dbc80 (WorldLevelPath.cpp)
    i32 BuildWorldLevelKey(i32 unused);  // (WorldLevelKey.cpp)

    char m_pad00[0x4];
    CGruntzMgr* m_4; // +0x04
    char m_pad08[0xc - 0x8];
    LevelMgr* m_0c; // +0x0c
    char m_pad10[0x1c - 0x10];
    i32 m_1c; // +0x1c  level number
    char m_pad20[0x28 - 0x20];
    CSymTab* m_28; // +0x28  level-record symbol table
    char m_pad2c[0x34 - 0x2c];
    CSymTab* m_34; // +0x34  battlez/multi symbol table
};
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_WORLDSTATE_H

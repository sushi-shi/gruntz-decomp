// TerrainTileLoader.h - CTerrainTileLoader, the per-tile terrain-action loader's owner.
//
// Promoted out of src/Gruntz/TerrainTileLoader.cpp (2026-07-13): a type defined in a .cpp
// is a fake per-TU view, and keeping it there is what let CGruntBehaviorLeaf invent
// `CDecayAnim::PlayStateAnim` for this class's Load instead of naming it.
#ifndef GRUNTZ_TERRAINTILELOADER_H
#define GRUNTZ_TERRAINTILELOADER_H

#include <Ints.h>
#include <rva.h>

class CTerrainTileLoader {
public:
    // __thiscall (retail passes `this` in ecx: `mov esi,ecx` prologue); returns 1 on every
    // handled path (retail materialises eax=1 before each `ret`).
    i32 Load(i32 actionIndex, i32 a1, i32 tileX, i32 tileY, i32 actionType, i32 a5); // 0x00075e90
    char m_pad[4];
};
SIZE_UNKNOWN(CTerrainTileLoader);

#endif // GRUNTZ_TERRAINTILELOADER_H

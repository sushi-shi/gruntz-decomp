#ifndef GRUNTZ_GRUNTZ_LEVELAREA_H
#define GRUNTZ_GRUNTZ_LEVELAREA_H

#include <Enums.h>

// The eight areas of the campaign, as carried by CState::m_levelType.
//
// It is the AREA number, not a level kind: CGameAssetNamespaces sets
// `m_levelType = t / 4 + 1` and immediately formats it as "AREA%i" to build the
// asset namespace, so the four levels of an area share one value. CPlay repeats
// the same `r / 4 + 1`.
//
// The names are RETAIL'S OWN, read out of the string table, which stores them
// descending (Gruntz in Space first, Rocky Roadz last) - the usual layout. The
// binary corroborates the numbering separately with MENU_AREAS_AREA1TITLE ..
// MENU_AREAS_AREA8TITLE and MENU_QUESTZ_AREA1 .. AREA8.
GZ_ENUM_BEGIN(LevelArea)
    AREA_ROCKY_ROADZ = 1,
    AREA_GRUNTZICLEZ = 2,
    AREA_TROUBLE_IN_THE_TROPICZ = 3,
    AREA_HIGH_ON_SWEETZ = 4,
    AREA_HIGH_ROLLERZ = 5,
    AREA_HONEY_I_SHRUNK_THE_GRUNTZ = 6,
    AREA_MINIATURE_MASTERZ = 7,
    AREA_GRUNTZ_IN_SPACE = 8,
    AREA_LAST = AREA_GRUNTZ_IN_SPACE,
    AREA_COUNT = 8,
    // The eight areas split into two tile sets at this boundary, and two
    // unrelated files agree on where: CGrunt's entrance-move switch takes the
    // first set with `area < AREA_TILESET_B_FIRST`, and CTriggerMgr picks a
    // different crumble-bridge token with `m_levelType > AREA_TILESET_A_LAST`.
    // Each half's tile ids are that set's own image indices, which is why the
    // ids themselves carry no engine names.
    AREA_TILESET_A_LAST = AREA_HIGH_ON_SWEETZ,
    AREA_TILESET_B_FIRST = AREA_HIGH_ROLLERZ
GZ_ENUM_END(LevelArea)

#endif // GRUNTZ_GRUNTZ_LEVELAREA_H

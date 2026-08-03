#ifndef GRUNTZ_GRUNTZ_QUESTLEVEL_H
#define GRUNTZ_GRUNTZ_QUESTLEVEL_H

#include <Enums.h>

// Level ordinals in the single-player Questz campaign, as carried by
// CSaveGame::m_curLevel / m_maxLevel and by CMD_LOAD_WORLD's lParam.
//
// The whole layout falls out of the menu builder, which enumerates every level
// once and gates each behind the progress needed to reach it:
//
//   * Eight areas of four stages each. Area N's four CMD_LOAD_WORLD items carry
//     levels 4*(N-1)+1 .. 4*N, and each stage's Disable guard is
//     `progress < level - 1` - so a stage unlocks exactly when the one before
//     it is cleared. That makes 32 campaign levels, 1..0x20.
//
//   * The Training page is four more, 0x25..0x28, ungated - reachable from the
//     start.
//
//   * Every one of the 38 guards also rejects `progress > 0x24`, which is what
//     puts the campaign's ceiling at 0x24 rather than at 0x20 or 0x28.
//
// The four ordinals between 0x20 and 0x25 are a real HOLE, not just menu
// omission. CGruntzMgr's level navigator says so outright: after wrapping, both
// GoToNextLevel and GoToPrevLevel will only load a level that satisfies
// `n <= QUESTLEVEL_CAMPAIGN_LAST || n >= QUESTLEVEL_TRAINING_FIRST`, so 0x21 to
// 0x24 are skipped by construction. That also separates two marks the menu
// alone could not: 0x20 is the last PLAYABLE campaign level (8 areas x 4), while
// 0x24 is the highest value the menu still treats as progress.
GZ_ENUM_BEGIN(QuestLevel)
    QUESTLEVEL_NONE = 0,
    QUESTLEVEL_FIRST = 1,
    // Last playable campaign level: QUESTLEVEL_PER_AREA * AREA_COUNT.
    QUESTLEVEL_CAMPAIGN_LAST = 0x20,
    // The highest ordinal the menu will still treat as in-campaign progress.
    // Every stage guard pairs its own lower bound with `> QUESTLEVEL_LAST`.
    QUESTLEVEL_LAST = 0x24,
    QUESTLEVEL_TRAINING_FIRST = 0x25,
    QUESTLEVEL_TRAINING_LAST = 0x28,
    // Stages per area, the step between one area's first level and the next's.
    QUESTLEVEL_PER_AREA = 4,
    // Not a level: CGruntzMgr::Post accepts it as the top of its range and then
    // rewrites it to QUESTLEVEL_FIRST, so it is the "start over" request.
    QUESTLEVEL_RESTART = 0x29,
    // The top of the range CGruntzMgr::Post will accept, which is that sentinel
    // rather than a level - so the bound gets its own name at the same value.
    QUESTLEVEL_POST_LAST = QUESTLEVEL_RESTART
GZ_ENUM_END(QuestLevel)

#endif // GRUNTZ_GRUNTZ_QUESTLEVEL_H

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
// The four ordinals between 0x20 and 0x25 are not enumerated by any menu page,
// so nothing here names them.
GZ_ENUM_BEGIN(QuestLevel)
    QUESTLEVEL_NONE = 0,
    QUESTLEVEL_FIRST = 1,
    // The highest ordinal the menu will still treat as in-campaign progress.
    // Every stage guard pairs its own lower bound with `> QUESTLEVEL_LAST`.
    QUESTLEVEL_LAST = 0x24,
    QUESTLEVEL_TRAINING_FIRST = 0x25,
    QUESTLEVEL_TRAINING_LAST = 0x28,
    // Stages per area, the step between one area's first level and the next's.
    QUESTLEVEL_PER_AREA = 4
GZ_ENUM_END(QuestLevel)

#endif // GRUNTZ_GRUNTZ_QUESTLEVEL_H

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
    QUESTLEVEL_AREA1_STAGE1 = 1,
    QUESTLEVEL_AREA1_STAGE2 = 2,
    QUESTLEVEL_AREA1_STAGE3 = 3,
    QUESTLEVEL_AREA1_STAGE4 = 4,
    QUESTLEVEL_AREA2_STAGE1 = 5,
    QUESTLEVEL_AREA2_STAGE2 = 6,
    QUESTLEVEL_AREA2_STAGE3 = 7,
    QUESTLEVEL_AREA2_STAGE4 = 8,
    QUESTLEVEL_AREA3_STAGE1 = 9,
    QUESTLEVEL_AREA3_STAGE2 = 0xa,
    QUESTLEVEL_AREA3_STAGE3 = 0xb,
    QUESTLEVEL_AREA3_STAGE4 = 0xc,
    QUESTLEVEL_AREA4_STAGE1 = 0xd,
    QUESTLEVEL_AREA4_STAGE2 = 0xe,
    QUESTLEVEL_AREA4_STAGE3 = 0xf,
    QUESTLEVEL_AREA4_STAGE4 = 0x10,
    QUESTLEVEL_AREA5_STAGE1 = 0x11,
    QUESTLEVEL_AREA5_STAGE2 = 0x12,
    QUESTLEVEL_AREA5_STAGE3 = 0x13,
    QUESTLEVEL_AREA5_STAGE4 = 0x14,
    QUESTLEVEL_AREA6_STAGE1 = 0x15,
    QUESTLEVEL_AREA6_STAGE2 = 0x16,
    QUESTLEVEL_AREA6_STAGE3 = 0x17,
    QUESTLEVEL_AREA6_STAGE4 = 0x18,
    QUESTLEVEL_AREA7_STAGE1 = 0x19,
    QUESTLEVEL_AREA7_STAGE2 = 0x1a,
    QUESTLEVEL_AREA7_STAGE3 = 0x1b,
    QUESTLEVEL_AREA7_STAGE4 = 0x1c,
    QUESTLEVEL_AREA8_STAGE1 = 0x1d,
    QUESTLEVEL_AREA8_STAGE2 = 0x1e,
    QUESTLEVEL_AREA8_STAGE3 = 0x1f,
    QUESTLEVEL_AREA8_STAGE4 = 0x20,
    // The retail area initializer has four resource slots in the deliberate
    // campaign/training gap. Gameplay navigation skips them, but the slots are
    // still members of the level-ordinal domain.
    QUESTLEVEL_RESERVED_33 = 0x21,
    QUESTLEVEL_RESERVED_34 = 0x22,
    QUESTLEVEL_RESERVED_35 = 0x23,
    QUESTLEVEL_RESERVED_36 = 0x24,
    // Completion boundaries used by the menu's progress tests. Progress stores
    // the last completed level, so each boundary shares that level's ordinal.
    QUESTLEVEL_AREA1_STAGE1_END = QUESTLEVEL_AREA1_STAGE1,
    QUESTLEVEL_AREA1_STAGE2_END = QUESTLEVEL_AREA1_STAGE2,
    QUESTLEVEL_AREA1_STAGE3_END = QUESTLEVEL_AREA1_STAGE3,
    QUESTLEVEL_AREA1_STAGE4_END = QUESTLEVEL_AREA1_STAGE4,
    QUESTLEVEL_AREA2_STAGE1_END = QUESTLEVEL_AREA2_STAGE1,
    QUESTLEVEL_AREA2_STAGE2_END = QUESTLEVEL_AREA2_STAGE2,
    QUESTLEVEL_AREA2_STAGE3_END = QUESTLEVEL_AREA2_STAGE3,
    QUESTLEVEL_AREA2_STAGE4_END = QUESTLEVEL_AREA2_STAGE4,
    QUESTLEVEL_AREA3_STAGE1_END = QUESTLEVEL_AREA3_STAGE1,
    QUESTLEVEL_AREA3_STAGE2_END = QUESTLEVEL_AREA3_STAGE2,
    QUESTLEVEL_AREA3_STAGE3_END = QUESTLEVEL_AREA3_STAGE3,
    QUESTLEVEL_AREA3_STAGE4_END = QUESTLEVEL_AREA3_STAGE4,
    QUESTLEVEL_AREA4_STAGE1_END = QUESTLEVEL_AREA4_STAGE1,
    QUESTLEVEL_AREA4_STAGE2_END = QUESTLEVEL_AREA4_STAGE2,
    QUESTLEVEL_AREA4_STAGE3_END = QUESTLEVEL_AREA4_STAGE3,
    QUESTLEVEL_AREA4_STAGE4_END = QUESTLEVEL_AREA4_STAGE4,
    QUESTLEVEL_AREA5_STAGE1_END = QUESTLEVEL_AREA5_STAGE1,
    QUESTLEVEL_AREA5_STAGE2_END = QUESTLEVEL_AREA5_STAGE2,
    QUESTLEVEL_AREA5_STAGE3_END = QUESTLEVEL_AREA5_STAGE3,
    QUESTLEVEL_AREA5_STAGE4_END = QUESTLEVEL_AREA5_STAGE4,
    QUESTLEVEL_AREA6_STAGE1_END = QUESTLEVEL_AREA6_STAGE1,
    QUESTLEVEL_AREA6_STAGE2_END = QUESTLEVEL_AREA6_STAGE2,
    QUESTLEVEL_AREA6_STAGE3_END = QUESTLEVEL_AREA6_STAGE3,
    QUESTLEVEL_AREA6_STAGE4_END = QUESTLEVEL_AREA6_STAGE4,
    QUESTLEVEL_AREA7_STAGE1_END = QUESTLEVEL_AREA7_STAGE1,
    QUESTLEVEL_AREA7_STAGE2_END = QUESTLEVEL_AREA7_STAGE2,
    QUESTLEVEL_AREA7_STAGE3_END = QUESTLEVEL_AREA7_STAGE3,
    QUESTLEVEL_AREA7_STAGE4_END = QUESTLEVEL_AREA7_STAGE4,
    QUESTLEVEL_AREA8_STAGE1_END = QUESTLEVEL_AREA8_STAGE1,
    QUESTLEVEL_AREA8_STAGE2_END = QUESTLEVEL_AREA8_STAGE2,
    QUESTLEVEL_AREA8_STAGE3_END = QUESTLEVEL_AREA8_STAGE3,
    QUESTLEVEL_AREA8_STAGE4_END = QUESTLEVEL_AREA8_STAGE4,
    QUESTLEVEL_FIRST = QUESTLEVEL_AREA1_STAGE1,
    // Last playable campaign level: QUESTLEVEL_PER_AREA * AREA_COUNT.
    QUESTLEVEL_CAMPAIGN_LAST = QUESTLEVEL_AREA8_STAGE4,
    // Exclusive upper bound for playable campaign levels; the first ordinal
    // in the deliberate gap before training.
    QUESTLEVEL_CAMPAIGN_END = QUESTLEVEL_CAMPAIGN_LAST + 1,
    // The highest ordinal the menu will still treat as in-campaign progress.
    // Every stage guard pairs its own lower bound with `> QUESTLEVEL_LAST`.
    QUESTLEVEL_LAST = QUESTLEVEL_RESERVED_36,
    QUESTLEVEL_TRAINING_STAGE1 = 0x25,
    QUESTLEVEL_TRAINING_STAGE2 = 0x26,
    QUESTLEVEL_TRAINING_STAGE3 = 0x27,
    QUESTLEVEL_TRAINING_STAGE4 = 0x28,
    QUESTLEVEL_TRAINING_FIRST = QUESTLEVEL_TRAINING_STAGE1,
    QUESTLEVEL_TRAINING_LAST = QUESTLEVEL_TRAINING_STAGE4,
    // Not a level: CGruntzMgr::Post accepts it as the top of its range and then
    // rewrites it to QUESTLEVEL_FIRST, so it is the "start over" request.
    QUESTLEVEL_RESTART = 0x29,
    QUESTLEVEL_TRAINING_END = QUESTLEVEL_RESTART,
    // The top of the range CGruntzMgr::Post will accept, which is that sentinel
    // rather than a level - so the bound gets its own name at the same value.
    QUESTLEVEL_POST_LAST = QUESTLEVEL_RESTART
GZ_ENUM_END(QuestLevel)

GZ_ENUM_CONST_BEGIN(QuestLevelConstants)
// Stages per area, the step between one area's first level and the next's.
    QUESTLEVEL_PER_AREA = 4
GZ_ENUM_CONST_END(QuestLevelConstants)

#endif // GRUNTZ_GRUNTZ_QUESTLEVEL_H

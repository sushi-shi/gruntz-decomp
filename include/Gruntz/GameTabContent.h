#ifndef GRUNTZ_GRUNTZ_GAMETABCONTENT_H
#define GRUNTZ_GRUNTZ_GAMETABCONTENT_H

#include <Enums.h>

// Which contents CStatusBarMgr builds inside TAB_GAME. The ordinary game menu
// uses the tab's id; the end-of-level view uses the mission-status command id.
GZ_ENUM_BEGIN(GameTabContent)
    GAME_TAB_MENU = 5,
    GAME_TAB_MISSION_STATUS = 0x1fb
GZ_ENUM_END(GameTabContent)

#endif // GRUNTZ_GRUNTZ_GAMETABCONTENT_H

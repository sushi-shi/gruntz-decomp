#ifndef GRUNTZ_CUSTOMWORLDINFODLG_H
#define GRUNTZ_CUSTOMWORLDINFODLG_H

#include <rva.h>

struct WwdLevelInfoSrc {
    i32 IsValidWwd(const char* path, void* outHeader); // 0x160530 (def in GameLevel.cpp)
};
SIZE_UNKNOWN();

struct WwdWorldHolder {
    char m_pad00[0x24];
    WwdLevelInfoSrc* m_24; // +0x24  level-info source
};
SIZE_UNKNOWN();

extern char g_dotDot[]; // 0x0020cf90 ".." (def in CustomWorldDialog.cpp)

#endif // GRUNTZ_CUSTOMWORLDINFODLG_H

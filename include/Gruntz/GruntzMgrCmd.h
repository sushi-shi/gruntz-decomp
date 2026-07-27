// GruntzMgrCmd.h - the GruntzMgrCmd TU's external declarations.
#ifndef GRUNTZ_GRUNTZMGRCMD_H
#define GRUNTZ_GRUNTZMGRCMD_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

class CGruntzMgr;
namespace Utils {
    class RegistryHelper;
}

i32 ParseSerial(CGruntzMgr* mgr, char* s); // 0x0d210 (SerialObjectFactory.cpp)
void SaveFrontBufferShot(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    i32 arg7
);

#endif // GRUNTZ_GRUNTZMGRCMD_H

#ifndef GRUNTZ_GRUNTZMGRCMD_H
#define GRUNTZ_GRUNTZMGRCMD_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

class CGruntzMgr;
namespace Utils {
    class RegistryHelper;
}

i32 ParseSerial(CGruntzMgr* mgr, char* s);
void SaveFrontBufferShot(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    i32 saveFlag
);

#endif // GRUNTZ_GRUNTZMGRCMD_H

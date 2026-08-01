#ifndef GRUNTZ_SAVEFRONTBUFFERSHOT_H
#define GRUNTZ_SAVEFRONTBUFFERSHOT_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

class CGruntzMgr;
namespace Utils {
    class RegistryHelper;
}

void SaveFrontBufferShotImpl(
    Utils::RegistryHelper* bute,
    CGruntzMgr* mgr,
    i32 w,
    i32 h,
    char* name,
    i32 saveFlag
);

#endif // GRUNTZ_SAVEFRONTBUFFERSHOT_H

// SaveFrontBufferShot.h - the SaveFrontBufferShot TU's external declarations.
#ifndef GRUNTZ_SAVEFRONTBUFFERSHOT_H
#define GRUNTZ_SAVEFRONTBUFFERSHOT_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
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
    void* arg7
);

#endif // GRUNTZ_SAVEFRONTBUFFERSHOT_H

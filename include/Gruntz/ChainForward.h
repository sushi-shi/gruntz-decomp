#ifndef GRUNTZ_GRUNTZ_CHAINFORWARD_H
#define GRUNTZ_GRUNTZ_CHAINFORWARD_H

#include <Ints.h>

class CGruntzMgr;
namespace Utils {
    class RegistryHelper;
}

// The two screenshot forwarders (ChainForward.cpp). Both return the
// SaveScreenshot result; a null link in the surface chain returns 0 (retail's
// `test eax,eax / jne / ret` guards return the failing null pointer itself).
i32 ChainForward14(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
); // 0x114f50
i32 ChainForward(
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 width,
    i32 height,
    char* name,
    i32 saveFlag
); // 0x114fa0

#endif // GRUNTZ_GRUNTZ_CHAINFORWARD_H

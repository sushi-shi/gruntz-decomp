// SaveScreenshot.h - owner header for SaveScreenshot (0x114ff0, SaveScreenshot.cpp),
// the in-game "dump the screen to Gruntz<NNNN>.BMP" helper. Declared here (not as a
// per-TU extern) so the ChainForward pre-guards forward to it with the real,
// reloc-masked signature. Forward-decls carry the exact class/struct keywords the
// retail mangling needs (?SaveScreenshot@@YAHPAVCDDSurface@@PAVRegistryHelper@Utils@@
// PAUCGameRegistry@@HHPADPAX@Z).
#ifndef GRUNTZ_SAVESCREENSHOT_H
#define GRUNTZ_SAVESCREENSHOT_H

#include <Ints.h>

class CDDSurface;
namespace Utils {
    class RegistryHelper;
}
struct CGameRegistry;

i32 SaveScreenshot(
    CDDSurface* src,
    Utils::RegistryHelper* bute,
    CGameRegistry* owner,
    i32 arg4,
    i32 arg5,
    char* name,
    void* arg7
); // 0x114ff0

#endif // GRUNTZ_SAVESCREENSHOT_H

#ifndef GRUNTZ_SAVEFRONTBUFFERSHOT_H
#define GRUNTZ_SAVEFRONTBUFFERSHOT_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

class CGruntzMgr;
class CRegMgr;

i32 SaveFrontBufferShotImpl(CRegMgr* reg, CGruntzMgr* mgr, i32 w, i32 h, char* name, i32 saveFlag);

#endif // GRUNTZ_SAVEFRONTBUFFERSHOT_H

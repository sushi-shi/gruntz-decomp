#ifndef GRUNTZ_GRUNTZMGRCMD_H
#define GRUNTZ_GRUNTZMGRCMD_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

class CGruntzMgr;
class CRegMgr;

i32 RestoreGameFromFile(CGruntzMgr* mgr, char* path);
void SaveFrontBufferShot(CRegMgr* reg, CGruntzMgr* mgr, i32 w, i32 h, char* name, i32 saveFlag);

#endif // GRUNTZ_GRUNTZMGRCMD_H

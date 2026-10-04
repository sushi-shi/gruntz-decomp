#ifndef GRUNTZ_GRUNTZMGRCMD_H
#define GRUNTZ_GRUNTZMGRCMD_H

#include <Ints.h>

#include <Enums.h>
#include <Ints.h>

class CGruntzMgr;
class Settings;

i32 RestoreGameFromFile(CGruntzMgr* mgr, char* path);
void SaveFrontBufferShot(Settings* reg, CGruntzMgr* mgr, i32 w, i32 h, char* name, i32 saveFlag);

#endif

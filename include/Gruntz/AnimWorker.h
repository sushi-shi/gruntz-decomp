#ifndef GRUNTZ_ANIMWORKER_H
#define GRUNTZ_ANIMWORKER_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/XferArchive.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <DDrawMgr/AnimWorkerObj.h>

class CUserLogic;

inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

#endif // GRUNTZ_ANIMWORKER_H

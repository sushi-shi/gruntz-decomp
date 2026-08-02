#ifndef GRUNTZ_ANIMWORKER_H
#define GRUNTZ_ANIMWORKER_H

#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/XferArchive.h>
#include <Ints.h>
#include <Wwd/WwdGameObjectFamily.h>

class CUserLogic;

inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

#endif // GRUNTZ_ANIMWORKER_H

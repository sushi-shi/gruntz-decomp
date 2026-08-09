#ifndef GRUNTZ_MOVINGLOGICGRUNTSCALEINLINE_H
#define GRUNTZ_MOVINGLOGICGRUNTSCALEINLINE_H

#include <Gruntz/Grunt.h>

inline CMovingLogic::CMovingLogic(CGameObject* owner, EGruntScale) : CUserLogic(owner) {
    InitOwner(g_val_1e9738);
}

#endif // GRUNTZ_MOVINGLOGICGRUNTSCALEINLINE_H

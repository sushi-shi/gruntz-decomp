#ifndef GRUNTZ_USERLOGICCTOREMIT_H
#define GRUNTZ_USERLOGICCTOREMIT_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

struct CGameObject;

extern "C" i32 LogicAttackFactory(CGameObject* obj);
extern "C" i32 LogicBumpFactory(CGameObject* obj);

#endif // GRUNTZ_USERLOGICCTOREMIT_H

#ifndef GRUNTZ_GRUNTINDICATORSPRITE_H
#define GRUNTZ_GRUNTINDICATORSPRITE_H

#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

struct CIndicatorSyncHelper {};
SIZE_UNKNOWN();
extern "C" u32 g_engineFrameDelta; // canonical _g_6bf3bc @ 0x6bf3bc (draw-delta mirror)

#include <Bute/ButeMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype

struct CIndicatorActReg : public CActReg {};
SIZE_UNKNOWN();

extern CIndicatorActReg g_healthActReg;   // 0x644d80
extern CIndicatorActReg g_powerupActReg;  // 0x644d30
extern CIndicatorActReg g_selectedActReg; // 0x644da8
extern CIndicatorActReg g_toyActReg;      // 0x644d58

#endif // GRUNTZ_GRUNTINDICATORSPRITE_H

// CEyeCandyAni.h - the animated eyecandy game-object (C:\Proj\Gruntz), a
// CUserLogic leaf (RTTI .?AVCEyeCandyAni@@). The 1-arg ctor (0xac870) folds the
// shared CUserLogic(obj) prologue, binds the "A" bute node + cycle geometry, then
// runs the shared eyecandy z-clamp/BigActHeight tail (cf. CEyeCandy / CBehindCandyAni
// in UserLogic.cpp). Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CEYECANDYANI_H
#define GRUNTZ_CEYECANDYANI_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic, CGameObject, g_buteMgr

SIZE_UNKNOWN(CEyeCandyAni);
class CEyeCandyAni : public CUserLogic {
public:
    CEyeCandyAni(CGameObject* obj); // 0xac870
    LogicTypeId GetTypeTag();       // 0x00ff00 (vtable slot 2: returns the logic-type id 0x3f4)
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00ff20 (vtable slot 1: two-chain Serialize)

    i32 m_savedGeoId; // +0x40  geometry id (m_38->m_geoId snapshot)
};

#endif // GRUNTZ_CEYECANDYANI_H

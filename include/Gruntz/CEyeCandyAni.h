// CEyeCandyAni.h - the animated eyecandy game-object (C:\Proj\Gruntz), a
// CUserLogic leaf (RTTI .?AVCEyeCandyAni@@). The 1-arg ctor (0xac870) folds the
// shared CUserLogic(obj) prologue, binds the "A" bute node + cycle geometry, then
// runs the shared eyecandy z-clamp/BigActHeight tail (cf. CEyeCandy / CBehindCandyAni
// in UserLogic.cpp). Offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CEYECANDYANI_H
#define GRUNTZ_CEYECANDYANI_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic, CGameObject, g_buteMgr

SIZE_UNKNOWN(CEyeCandyAni);
class CEyeCandyAni : public CUserLogic {
public:
    CEyeCandyAni(CGameObject* obj); // 0xac870

    i32 m_40; // +0x40  geometry id (m_38->m_1b4 snapshot)
};

#endif // GRUNTZ_CEYECANDYANI_H

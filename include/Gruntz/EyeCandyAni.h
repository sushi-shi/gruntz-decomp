// EyeCandyAni.h - the animated eyecandy game-object (C:\Proj\Gruntz), a
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
    TILE_LOGIC_TAIL
public:
    virtual ~CEyeCandyAni() OVERRIDE; // slot 0
    CEyeCandyAni(CGameObject* obj);   // 0xac870
    // 0x0000ff00 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000ff00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE { return LOGIC_EYECANDYANI; }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00ff20 (vtable slot 1: two-chain Serialize)
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry + the class's coordinate registry (g_eyeCandyActReg
    // @0x646060). The SAME archetype as CFrontCandyAni::RegisterActs (0x0ad310).
    static void RegisterActs(); // 0x0acd10
    // Re-target the bound object's animation sub-object (m_38 + 0x1a0) to the
    // current draw-delta (g_6bf3bc) and return 0.
    i32 AdvanceAnim(); // 0x0acf10

    i32 m_savedGeoId; // +0x40  geometry id (m_38->m_geoId snapshot)
};
VTBL(CEyeCandyAni, 0x001e8334);

#endif // GRUNTZ_CEYECANDYANI_H

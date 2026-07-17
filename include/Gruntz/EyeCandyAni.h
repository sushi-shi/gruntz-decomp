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

class CEyeCandyAni : public CUserLogic, public CWapX {
public:
public:
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    CEyeCandyAni(CGameObject* obj);   // 0xac870
    // 0x0000ff00 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000ff00, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EYECANDYANI;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // Resolve the registry entry for id; run its bound handler as a PMF on this
    // (ResolveEntry inlined twice). 0x0acbb0.
    virtual void FireActivation(i32 id) OVERRIDE;
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry + the class's coordinate registry (g_eyeCandyActReg
    // @0x646060). The SAME archetype as CFrontCandyAni::RegisterActs (0x0ad310).
    static void RegisterActs(); // 0x0acd10
    // Re-target the bound object's animation sub-object (m_38 + 0x1a0) to the
    // current draw-delta (g_engineFrameDelta) and return 0.
    i32 AdvanceAnim(); // 0x0acf10
};
VTBL(CEyeCandyAni, 0x001e8334);
SIZE(CEyeCandyAni, 0x54);

// The per-coordinate activation registry entry (g_eyeCandyDispatch's element): its
// first dword receives the per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this
// single-inheritance class). RunAct/RegisterActs cast the CActReg entry to this. A
// faithful 4-byte PMF record, hoisted out of FrontCandyAni.cpp.
typedef i32 (CUserLogic::*EyeCandyHandler)();
struct CEyeCandyActEntry {
    EyeCandyHandler m_fn;
};
SIZE_UNKNOWN(CEyeCandyActEntry);

#endif // GRUNTZ_CEYECANDYANI_H

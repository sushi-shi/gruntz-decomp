#ifndef GRUNTZ_CSPOTLIGHT_H
#define GRUNTZ_CSPOTLIGHT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (+ CFileMemBase / CGameObject)

class CSpotLight : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in SpotLightCtor.cpp)
    RVA(0x00012ff0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SPOTLIGHT;
    }
public:
    CSpotLight(CGameObject* obj); // 0xb1200
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    // The vtable slot-4 (UserLogicVfunc2) activation dispatcher body (0x0b1630);
    // a plain method - the base placeholder blocks the int-arg OVERRIDE spelling.
    virtual void FireActivation(i32 id) OVERRIDE;
    // The per-tick laser/rotation update (0x0b1af0).
    i32 Tick();
    // The per-tick offset-rotation + grid-cell "A" bute re-resolve (0x0b1ee0);
    // body in SpotLight.cpp.
    int Update();

    char m_pad54[0x58 - 0x54];
    double m_58;          // +0x58  per-tick angular rate (Update: angleStep advancing m_90)
    double m_60;          // +0x60  computed/world offset X (Update result m_70/m_78 + rotation)
    double m_68;          // +0x68  computed/world offset Y
    double m_70;          // +0x70  anchor/base X (target screen pos or grid-snapped seed)
    double m_78;          // +0x78  anchor/base Y
    double m_80;          // +0x80  rotation offset X (m_70 - snapped origin)
    double m_88;          // +0x88  rotation offset Y (m_78 - snapped origin)
    double m_90;          // +0x90  running rotation angle (seeded pi or 0)
    CWwdGameObjectA* m_focus; // +0x98  the serialized focus object (GetTypeId()==5 gate; a
                          //         real CGameObject - id at +0x188, coords at +0x5c/+0x60)
    i32 m_9c;             // +0x9c
    i32 m_a0;             // +0xa0
    i32 m_a4;             // +0xa4
};
SIZE(0xa8);

struct CSpotActEntry {
    i32 (CUserLogic::*m_fn)();
};
SIZE_UNKNOWN();

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" void* Probe_32ce(i32 x, i32 y, void* rect, i32* outA, i32* outB, i32 flag);
extern "C" void Activate_4322(void* target, i32 f);
extern "C" i32 SoundPlay_1360d0(i32 a, i32 b, i32 c, i32 d);
extern u8 g_randSeeded; // 0x6c127d  (canonical C++ linkage; see <Gruntz/Random.h>)
extern i32 g_randSeed;  // 0x6c1288  (canonical C++ linkage; see <Gruntz/Random.h>)
extern u32 (*g_pTimeGetTime)();        // 0x6c4650
extern char s_LEVEL_UFOHAZARDLASER[];  // 0x611c54 "LEVEL_UFOHAZARDLASER%d"

#endif // GRUNTZ_CSPOTLIGHT_H

// StatusBarSprite.h - the status-bar sprite tile-logic leaf (C:\Proj\Gruntz), a
// CUserLogic game-object (RTTI game class, vtable 0x5e7fc4). Extracted from the former
// StatusBarSpriteActs.cpp-local view so the leaf dtor (0x11b80) homes onto the real
// class. Only offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CSTATUSBARSPRITE_H
#define GRUNTZ_CSTATUSBARSPRITE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CStatusBarSprite : CUserLogic)

class CStatusBarSprite : public CUserLogic {
public:
    virtual ~CStatusBarSprite() OVERRIDE;                              // slot 0
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CStatusBarSprite(CGameObject* obj); // 0x10c230
    static void InitActReg();           // 0x10c430
    void FireActivation(i32 coord);     // 0x10c4b0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs();         // 0x10c610
    i32 AdvanceAnim();                  // 0x10c810 (the per-frame handler PMF; body in the stub TU)

    CAniElement* m_40;                  // +0x40  saved active-anim descriptor (ctor snapshot)
    char m_pad44[0x54 - 0x44]; // +0x44  (unmodeled leaf tail; size 0x54 proven from
    //         the state pump's `new CStatusBarSprite` = operator new(0x54))
};
VTBL(CStatusBarSprite, 0x1e7fc4);
SIZE(CStatusBarSprite, 0x54);

// The activation-registry handler-entry record (the .data CActReg row): its first
// dword receives the per-frame handler PMF (4-byte code ptr on this complete
// single-inheritance class).
typedef i32 (CStatusBarSprite::*StatusBarSpriteHandler)();
struct CStatusBarSpriteActEntry {
    StatusBarSpriteHandler m_fn;
};
SIZE_UNKNOWN(CStatusBarSpriteActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CSTATUSBARSPRITE_H

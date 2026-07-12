// VoiceTrigger.h - the voice-trigger tile-logic game object (C:\Proj\Gruntz).
//
// CVoiceTrigger : CUserLogic - a method-only leaf (no data members beyond the
// CUserLogic base; the bound object is the inherited m_10/m_38). Its methods are
// split across two TUs: the no-arg ctor + GetTypeTag in src/Gruntz/UserLogic.cpp,
// and the 1-arg ctor + Init/RegisterActs + Tick + dtor in src/Gruntz/VoiceTrigger.cpp.
// This header unifies the two per-TU redeclarations (matching-neutral: no members,
// only the CUserLogic dtor slot is overridden). Only offsets/code bytes are
// load-bearing; names are placeholders.
#ifndef GRUNTZ_CVOICETRIGGER_H
#define GRUNTZ_CVOICETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CVoiceTrigger : CUserLogic)

SIZE(CVoiceTrigger, 0x54);
class CVoiceTrigger : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CVoiceTrigger();                 // 0x013470 (no-arg ctor; body in UserLogic.cpp)
    CVoiceTrigger(CGameObject* obj); // 0x119b50 (1-arg leaf ctor; body in VoiceTrigger.cpp)
    // 0x000133b0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x000133b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_VOICETRIGGER;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x0134e0 (vtable slot 1: serialize chain)
    static void InitActReg();                     // 0x11a320 (constructs g_vtrigColl @0x651500)
    void FireActivation(i32 coord);    // 0x11a3a0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs();        // 0x11a500 (binds Tick to the activation key "A"; static:
                                       //  no this, called this-less by the game-object factory)
    i32 Tick();                        // 0x11a700
    virtual ~CVoiceTrigger() OVERRIDE; // 0x0135a0 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];         // +0x40  (unmodeled leaf tail; size 0x54 proven from
                                       //         the state pump's `new CVoiceTrigger` = new(0x54))
};
VTBL(CVoiceTrigger, 0x001e885c);

#endif // GRUNTZ_CVOICETRIGGER_H

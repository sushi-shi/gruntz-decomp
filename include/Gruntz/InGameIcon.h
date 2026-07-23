#ifndef GRUNTZ_GRUNTZ_CINGAMEICON_H
#define GRUNTZ_GRUNTZ_CINGAMEICON_H

#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h> // CObject/CArchive base + <windows.h>

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr) // *0x24556c canonical singleton

struct CIconMapHolder {
    char m_pad00[0x10];
    CMapStringToPtr m_10map; // +0x10  the lookup table (Lookup 0x1b8438 = CMapStringToPtr)
};
SIZE_UNKNOWN();
struct CGameRegMapHolder {
    char m_pad00[0x28];
    CIconMapHolder* m_28; // +0x28  the map holder (Lookup table at +0x10)
};
SIZE_UNKNOWN();

#include <Gruntz/CurPlayer.h> // g_curPlayer (the current local player index)

#include <Gruntz/SoundState.h> // g_sndCueTag (the cue-item id) + g_sndEnabled

extern "C" u32 g_frameTime; // DAT_00645588  (the running game clock stamped into +0x58)

#include <Gruntz/SerialCounter.h> // g_serialCounter (the serialize sequence counter)

class CInGameIcon : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011cb0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMEICON;
    } // slot 2
public:
    CInGameIcon(CGameObject* obj); // 0x095b10  (the HUD-icon builder ctor)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    // Configure the icon's sprite/animation for a category ("GAME_TREASURE" /
    // "GAME_POWERUP" / "GAME_CURSE"; 0 for the initial reset). Reached through an
    // ILT thunk (0x3440), reloc-masked.
    void SetupSprite(const char* cat); // -> 0x3440

    i32 HandleInput(); // 0x097680
    virtual void FireActivation(i32 id)
        OVERRIDE; // 0x097880 (dispatch g_iconActionTable[id] on this)
    // (RunState @0x097de0 is GONE from here: the ILT bytes prove it is CToyPeek's
    // vtable slot 4 - see <Gruntz/ToyPeek.h>. It never was a CInGameIcon method.)
    i32 RefreshCell();                  // 0x098340
    i32 PeekCycle();                    // 0x0984b0 (peek-timer random-sprite refresh)
    i32 PlaceAt(i32 idx, i32 gridBase); // 0x0986b0
    i32 Reposition();                   // 0x098a90 (drift re-place refresh)
    void SetField54(i32 v);             // 0x099b10

    // --- CInGameIcon own fields (+0x44/+0x68..+0x74 roles still unproven) ---
    i32 m_cmapId;        // +0x54  registry-CMap lookup result (SetField54; place gate)
    i32 m_driftPos;      // +0x58  drift-tracked position lo (i64 {m_driftPos:m_driftPosHi})
    i32 m_driftPosHi;    // +0x5c  drift-tracked position hi
    i32 m_driftThresh;   // +0x60  drift threshold lo (i64 {m_driftThresh:m_driftThreshHi})
    i32 m_driftThreshHi; // +0x64  drift threshold hi
    i32 m_68;            // +0x68  icon idle-timer LO (i64 pair w/ m_6c; interleaved keep)
    i32 m_6c;            // +0x6c  icon idle-timer HI
    i32 m_70;            // +0x70  icon idle-window LO (i64 pair w/ m_74)
    i32 m_74;            // +0x74  icon idle-window HI
    CWwdGameObjectA* m_glitterSprite; // +0x78  glitter overlay FX sprite (A-kind)
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*IconActHandler)();

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).

#include <Gruntz/LogicFnTable.h> // CActReg (the dispatch-table shell)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---

#endif // GRUNTZ_GRUNTZ_CINGAMEICON_H

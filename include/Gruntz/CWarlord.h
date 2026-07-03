// CWarlord.h - the AI fort-warlord game object (RTTI .?AVCWarlord@@), a
// CUserLogic-derived leaf. This header models ONLY what the three home-TU
// methods (~CWarlord, the slot-4 state dispatcher, and the anim re-arm) touch.
//
// Hierarchy (RTTI-recovered, see UserLogic.h):
//   CUserBase  <- CUserLogic  <- CWarlord   (vftable 0x5e7404)
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing. CWarlord adds, past the 0x40 CUserLogic base:
//   m_38   (inherited CUserLogic::m_38, the anim player ptr - reused by 0x44bb0)
//   CString m_54   the warlord's own destructible string member (drives the dtor)
//
// Engine callees/globals are reloc-masked (declared no-body / extern).
#ifndef GRUNTZ_CWARLORD_H
#define GRUNTZ_CWARLORD_H

#include <Mfc.h> // CObject / CString (MFC TU - must precede <windows.h>)
#include <Gruntz/CGameRegistry.h>

#include <Gruntz/UserLogic.h> // CUserBase / CUserLogic / EngStr / CGameObject

#include <rva.h>

// ---------------------------------------------------------------------------
// The geometry sub-player at +0x1a0 on the bound game object (reached as
// (char*)CUserLogic::m_38 + 0x1a0). SetGeoSourceR (engine FUN_0055c360, __thiscall
// ret 4) re-arms it against a geometry source; the anim re-arm method also polls
// its +0x20 / +0x28 state words. Modeled no-body so the `ecx=&sub; call` reloc-masks.
// ---------------------------------------------------------------------------
class CWarlordAnimSub {
public:
    i32 SetGeoSourceR(i32 src); // 0x15c360

    char m_pad00[0x20];
    i32 m_20; // +0x20  state word (== 0 gates the moving-anim re-arm)
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28  state word (!= 0 gates the moving-anim re-arm)
};

// The global default geometry source the re-arm consumes (DATA 0x2bf3bc).
DATA(0x002bf3bc)
extern i32 g_defaultGeo;

// The running game clock (low 32 bits of a 64-bit counter at 0x645588; the high
// half lives in the next word, read together as __int64 in the cooldown clamp).
extern "C" u32 g_645588;

// ---------------------------------------------------------------------------
// The game-registry singleton's threat/spatial helper (g_gameReg->m_68): nearest
// squared distance from (x, y) over the warlord's owner index. Engine
// FUN_0047d1d0, __thiscall ret 0xc. External/no-body so the call reloc-masks.
// ---------------------------------------------------------------------------
class CRegThreatHelper {
public:
    i32 NearestEnemyDist(i32 owner, i32 x, i32 y);
};

// The registry's battle-event sink (g_gameReg->m_cueSink): fires a fort battle-cry /
// event message. Engine __thiscall at RVA 0x11b7c0. External/no-body.
class CRegBattleEvent {
public:
    void PostBattleEvent(i32 id, i32 event, i32 a, i32 b, i32 c);
};

// The single-player "level done?" chain: g_gameReg->m_curState (a level/mission object)
// -> m_3f4 (its objective tracker) -> m_4c (non-zero "complete" flag).
struct CWarlordObjective {
    char m_pad00[0x4c];
    i32 m_4c; // +0x4c  completion flag (0 = still playing)
};
struct CWarlordMission {
    char m_pad00[0x3f4];
    CWarlordObjective* m_3f4; // +0x3f4  objective tracker
};

// ---------------------------------------------------------------------------
// CGameRegistry - the big game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @
// 0x64556c). Only the offsets the warlord's per-tick update touches are modeled.
// ---------------------------------------------------------------------------
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CWarlord
// ---------------------------------------------------------------------------
class CWarlord : public CUserLogic {
public:
    CWarlord(i32);                // 0x42d40 (the warlord ctor: base init + name/state setup)
    virtual ~CWarlord() OVERRIDE; // 0x107f0 (the home-TU dtor: ~CString(m_54) + the base teardown)

    // construct the file-static per-action handler table (g_actionTable @0x644610)
    // over the fixed [2000, 2010] range via the shared registry ctor (0x408710).
    static void InitActReg(); // 0x445c0

    // slot-4 override of an inherited CUserLogic virtual: the animation-state
    // dispatcher over the file-static table.
    i32 ResolveState(i32 key); // 0x44640 (homed by RVA; non-virtual to keep the
                               // dtor's vtable-stamp codegen aligned with retail)

    // re-arm the moving animation off the global geo source (0x44bb0).
    i32 RearmMoving(); // 0x44bb0

    // per-tick warlord threat/attribute updates (0x44c00 / 0x44d10).
    i32 LoadAttributes();
    i32 LoadAttributes2();

    // spawn the fort splash particles + arm the panic timer (0x44f80).
    void BuildFortSplashParticles();

    // raise the fort alert when an enemy is inside the panic radius (0x45270).
    void NotifyFortUnderAttack();

    // engine tail helpers (__thiscall), reached via ILT thunks. External/no-body.
    void ResolveMovingAnimation();    // 0x45100
    void ResolveIdleAnimation();      // 0x45960
    void ResolveBattlecryAnimation(); // 0x45b60
    void RaiseBattleAlert();          // 0x457b0  (panic-radius alert variant)

    // Past the 0x40 CUserLogic base. m_38 is the inherited CUserLogic::m_38
    // (anim player); CString m_54 is CWarlord's own destructible member; the
    // 64-bit cooldown stamp/window live at m_88/m_8c and m_90/m_94.
    char m_pad40[0x54 - 0x40];
    CString m_54; // +0x54  destructible string member
    char m_pad58[0x88 - 0x58];
    i32 m_88; // +0x88  cooldown stamp lo (64-bit with m_8c)
    i32 m_8c; // +0x8c  cooldown stamp hi
    i32 m_90; // +0x90  cooldown window lo (64-bit with m_94)
    i32 m_94; // +0x94  cooldown window hi
};

#endif // GRUNTZ_CWARLORD_H

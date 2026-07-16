// Warlord.h - the AI fort-warlord game object (RTTI .?AVCWarlord@@), a
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
#include <Gruntz/GameRegistry.h>

#include <Gruntz/UserLogic.h> // CUserBase / CUserLogic / EngStr / CGameObject

#include <rva.h>
extern "C" CGameRegistry* g_gameReg; // *0x24556c canonical singleton

// ---------------------------------------------------------------------------
// The geometry sub-player at +0x1a0 on the bound game object (reached as
// (char*)CUserLogic::m_38 + 0x1a0). SetGeoSourceR (engine FUN_0055c360, __thiscall
// ret 4) re-arms it against a geometry source; the anim re-arm method also polls
// its +0x20 / +0x28 state words. Modeled no-body so the `ecx=&sub; call` reloc-masks.
// ---------------------------------------------------------------------------
class CWarlordAnimSub {
public:
    char m_pad00[0x20];
    i32 m_20; // +0x20  state word (== 0 gates the moving-anim re-arm)
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28  state word (!= 0 gates the moving-anim re-arm)
};

// The global default geometry source the re-arm consumes (0x6bf3bc). Bound to the
// canonical extern "C" _g_6bf3bc (the tree-wide keep-last winner, DATA'd in
// tilelogicpump); the old C++-mangled g_defaultGeo alias was reloc-UNBOUND.
extern "C" u32 g_engineFrameDelta;

// The running game clock (low 32 bits of a 64-bit counter at 0x645588; the high
// half lives in the next word, read together as __int64 in the cooldown clamp).
extern "C" u32 g_frameTime;

// ---------------------------------------------------------------------------
// The game-registry singleton's threat/spatial helper (g_gameReg->m_cmdGrid, a
// CTriggerMgr): the nearest-enemy squared distance is CTriggerMgr::NearestCellDist
// (0x7d1d0), called directly on m_cmdGrid (see LoadAttributes). This view carries
// ONLY the fort battle-cue timer sub-block AdvanceMovingAnim arms (armed on the
// per-frame moving tick): m_288 the cue-armed gate, m_290/m_294 the 64-bit start
// stamp (g_frameTime), m_298/m_29c the window (0x3e8 ms), m_2a0 the cue-active flag.
// @identity-TODO: these +0x290/+0x2a0 cue fields overlap CTriggerMgr's overlay-
// descriptor / m_pendingFx modeling (a conflation to reconcile cross-lane).
// ---------------------------------------------------------------------------
class CRegThreatHelper {
public:
    char m_pad00[0x288];
    i32 m_288; // +0x288  cue-armed gate
    char m_pad28c[0x290 - 0x28c];
    i64 m_stamp;  // +0x290  cue start-stamp (lo=g_frameTime, hi=0)
    i64 m_window; // +0x298  cue window (lo=0x3e8, hi=0)
    i32 m_2a0;    // +0x2a0  cue-active flag
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
    CWarlordObjective* m_objective; // +0x3f4  objective tracker
};

// ---------------------------------------------------------------------------
// CGameRegistry - the big game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @
// 0x64556c). Only the offsets the warlord's per-tick update touches are modeled.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CWarlord
// ---------------------------------------------------------------------------
class CWarlord : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x000107a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARLORD;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
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

    // a second per-state moving-anim re-arm handler dispatched from the warlord
    // anim-state table; byte-identical body to RearmMoving. (0x44f30)
    i32 RearmMoving2(); // 0x44f30

    // per-frame moving-state action handler (0x44e70): advance the +0x1a0 anim
    // sub-mgr, and once it goes idle (m_28!=0 && m_20==0), when the fort cue is
    // armed for the current player re-stamp the battle-cue timer (m_290 clock,
    // m_298=0x3e8 window, m_2a0/m_29c/m_294=0), then re-resolve the moving anim.
    i32 AdvanceMovingAnim(); // 0x44e70

    // per-tick warlord threat/attribute updates (0x44c00 / 0x44d10).
    i32 LoadAttributes();
    i32 LoadAttributes2();

    // spawn the fort splash particles + arm the panic timer (0x44f80).
    void BuildFortSplashParticles();

    // raise the fort alert when an enemy is inside the panic radius (0x45270).
    void NotifyFortUnderAttack();

    // The moving/idle/battlecry anim resolves (0x45100/0x45960/0x45b60) are real
    // CGrunt methods (the warlord `this` is a CGrunt receiver at these sites) - the
    // callers cast to CGrunt::* so the rel32 binds; no fake CWarlord shadow decl.
    void RaiseBattleAlert(); // 0x457b0  (panic-radius alert variant)

    // Past the 0x40 CUserLogic base. m_38 is the inherited CUserLogic::m_38
    // (anim player); CString m_54 is CWarlord's own destructible member; the
    // 64-bit cooldown stamp/window live at m_88/m_8c and m_90/m_94.
    char m_pad40[0x54 - 0x40];
    CString m_54; // +0x54  destructible string member (the resolved "WARLORDZ_<owner>" name)
    // The ctor resolves the eleven per-state animation handles by looking each
    // "GRUNTZ_<owner>_<state>" key up in the bound object's embedded name->handle map
    // and stashing the result here (0 when absent).
    // The eleven per-state anim descriptors, each looked up as a CAniElement* out of
    // the bound object's m_animRegistry CMapStringToPtr (macro WARLORD_ANIM_LOOKUP:
    // `dst = (CAniElement*)h`, the sole driver; CreateAniEntry_1528d0 returns CAniElement*).
    CAniElement* m_animIdle1;      // +0x58
    CAniElement* m_animIdle2;      // +0x5c
    CAniElement* m_animIdle3;      // +0x60
    CAniElement* m_animIdle4;      // +0x64
    CAniElement* m_animBattlecry1; // +0x68
    CAniElement* m_animBattlecry2; // +0x6c
    CAniElement* m_animBattlecry3; // +0x70
    CAniElement* m_animJoy;        // +0x74
    CAniElement* m_animDeath;      // +0x78
    CAniElement* m_animMoving;     // +0x7c
    CAniElement* m_animPanic;      // +0x80
    char m_pad84[0x88 - 0x84]; // +0x84
    // The threat-cooldown timer: a 64-bit start stamp (m_cooldownStamp) and window
    // (m_cooldownWindow), each stored as a manually zero-extended lo/hi i32 pair so
    // the elapsed compare runs 64-bit; retail emits separate 32-bit stores.
    i32 m_cooldownStampLo;  // +0x88
    i32 m_cooldownStampHi;  // +0x8c
    i32 m_cooldownWindowLo; // +0x90
    i32 m_cooldownWindowHi; // +0x94
    // A second 64-bit stamp/window timer pair (zeroed in the ctor prologue and again
    // just before the initial moving-anim resolve).
    i32 m_timer2StampLo;  // +0x98
    i32 m_timer2StampHi;  // +0x9c
    i32 m_timer2WindowLo; // +0xa0
    i32 m_timer2WindowHi; // +0xa4
    i32 m_a8;             // +0xa8
    // +0xac  the warlord battle-event tag (0x442..0x445 per owner KING/NAPOLEAN/PATTON/VIKING)
    i32 m_ownerTag;
};
VTBL(CWarlord, 0x1e7404);
SIZE(CWarlord, 0xb0);

#endif // GRUNTZ_CWARLORD_H

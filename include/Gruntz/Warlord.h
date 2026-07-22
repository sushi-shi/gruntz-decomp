#ifndef GRUNTZ_CWARLORD_H
#define GRUNTZ_CWARLORD_H

#include <Mfc.h> // CObject / CString (MFC TU - must precede <windows.h>)
#include <Gruntz/GameRegistry.h>

#include <Gruntz/UserLogic.h> // CUserBase / CUserLogic / EngStr / CGameObject

#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)

class CWarlordAnimSub {
public:
    char m_pad00[0x20];
    i32 m_20; // +0x20  state word (== 0 gates the moving-anim re-arm)
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28  state word (!= 0 gates the moving-anim re-arm)
};
SIZE_UNKNOWN();

extern "C" u32 g_engineFrameDelta;

extern "C" u32 g_frameTime;

// ---------------------------------------------------------------------------
// The game-registry singleton's threat/spatial helper (g_gameReg->m_cmdGrid, a
// CTriggerMgr): the nearest-enemy squared distance is CTriggerMgr::NearestCellDist
// (0x7d1d0), called directly on m_cmdGrid (see LoadAttributes). This view carries
// ONLY the fort battle-cue timer sub-block AdvanceMovingAnim arms (armed on the
// per-frame moving tick): m_288 the cue-armed gate, m_290/m_294 the 64-bit start
// (CRegThreatHelper DISSOLVED 2026-07-21: the "cue fields" were CTriggerMgr's own
// m_phase/m_timerBase/m_timerWindow/m_pendingFx - the panic-timer blocks now write
// the real members through the typed m_cmdGrid.)

// (CRegBattleEvent DISSOLVED 2026-07-20: PostBattleEvent IS CGruntSpawnConfig::Cue on the
// +0x60 m_cueSink - the warlord fort-alert path now calls reg->m_cueSink->Cue directly.)

// (CWarlordMission/CWarlordObjective DISSOLVED 2026-07-21: the "mission" was CPlay
// and the "objective tracker" its m_frameMarker CTimer - the +0x4c completion flag
// is CTimer::m_currentMs.)

class CWarlord : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x000107a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WARLORD;
    } // slot 2
public:
    CWarlord(i32); // 0x42d40 (the warlord ctor: base init + name/state setup)
    // NO user-declared destructor: retail's ~CWarlord (0x107f0) is the COMPILER-
    // GENERATED one. cl 5.0 elides the most-derived vptr re-stamp in an IMPLICIT
    // dtor (it knows no user body can observe the vptr) but always emits it for a
    // user-declared one - even an empty `{}`. Declaring it was the mis-model behind
    // the old "unreachable restamp" wall. The implicit dtor still destroys the
    // CString m_54 + chains the base teardown, and is still virtual (CUserBase's is);
    // Warlord.cpp's ctor emits the vtable -> ??_GCWarlord -> ??1CWarlord, so the
    // body lands in this obj and is pinned by @rva-symbol there.
    // docs/patterns/eh-dtor-vptr-restamp-presence.md

    // construct the file-static per-action handler table (g_actionTable @0x644610)
    // over the fixed [2000, 2010] range via the shared registry ctor (0x408710).
    static void InitActReg(); // 0x445c0

    // slot-4 override of an inherited CUserLogic virtual: the animation-state
    // dispatcher over the file-static table.
    virtual void FireActivation(i32 id) OVERRIDE; // 0x44640 (homed by RVA; non-virtual to keep the
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

    // The moving/idle/battlecry anim resolvers (0x45100/0x45960/0x45b60) are CWarlord's.
    // This header used to claim they "are real CGrunt methods (the warlord `this` is a
    // CGrunt receiver at these sites)" - that was an assertion with no caller evidence
    // behind it, and the callers refute it: every retail caller of all three is a
    // CWarlord method, 0x45100's include ??0CWarlord (the ctor, which can only call its
    // own class's/bases' methods), and CWarlord's RTTI chain (CUserLogic/CUserBase/CWapX)
    // has no CGrunt. They stay declared on CGrunt for now ONLY because their bodies use
    // animation members (m_animPlayer/m_activeAnimDesc/m_*GeoSrc) that CWarlord does not
    // model yet - see the blocker write-up in Warlord.cpp's banner. Not a wall: a
    // dependency on the CGrunt-spine conversion Grunt.h tracks.
    void RaiseBattleAlert(); // 0x457b0  (panic-radius alert variant)

    // Past the 0x40 CUserLogic base. m_38 is the inherited CUserLogic::m_38
    // (anim player); CString m_54 is CWarlord's own destructible member; the
    // 64-bit cooldown stamp/window live at m_88/m_8c and m_90/m_94.
    CString m_54; // +0x54  destructible string member (the resolved "WARLORDZ_<owner>" name)
    // The ctor resolves the eleven per-state animation handles by looking each
    // "GRUNTZ_<owner>_<state>" key up in the bound object's embedded name->handle map
    // and stashing the result here (0 when absent).
    // The eleven per-state anim descriptors, each looked up as a CAniElement* out of
    // the bound object's m_animRegistry CMapStringToPtr (macro WARLORD_ANIM_LOOKUP:
    // `dst = (CAniElement*)h`, the sole driver; CreateAniEntry returns CAniElement*).
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
    char m_pad84[0x88 - 0x84];     // +0x84
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
SIZE(0xb0);

#endif // GRUNTZ_CWARLORD_H

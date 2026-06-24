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

#include <Gruntz/UserLogic.h> // CUserBase / CUserLogic / EngStr / CGameObject

#include <rva.h>

// ---------------------------------------------------------------------------
// The geometry sub-player embedded in the anim player at +0x1a0. SetGeoSourceR
// (engine FUN_0055c360, __thiscall ret 4) re-arms it against a geometry source;
// the anim re-arm method also polls its +0x20 / +0x28 state words. Modeled
// no-body so the `ecx=&sub; call` reloc-masks.
// ---------------------------------------------------------------------------
class CWarlordAnimSub {
public:
    int SetGeoSourceR(int src); // 0x15c360

    char m_pad00[0x20];
    int m_20; // +0x20  state word (== 0 gates the moving-anim re-arm)
    char m_pad24[0x28 - 0x24];
    int m_28; // +0x28  state word (!= 0 gates the moving-anim re-arm)
};

class CWarlordAnimPlayer {
public:
    char m_pad00[0x1a0];
    CWarlordAnimSub m_1a0; // +0x1a0  geometry sub-player
};

// The global default geometry source the re-arm consumes (DATA 0x2bf3bc).
DATA(0x002bf3bc)
extern int g_defaultGeo;

// ---------------------------------------------------------------------------
// CWarlord
// ---------------------------------------------------------------------------
class CWarlord : public CUserLogic {
public:
    ~CWarlord(); // 0x107f0 (the home-TU dtor: ~CString(m_54) + the base teardown)

    // slot-4 override of an inherited CUserLogic virtual: the animation-state
    // dispatcher over the file-static table.
    int ResolveState(int key); // 0x44640 (homed by RVA; non-virtual to keep the
                               // dtor's vtable-stamp codegen aligned with retail)

    // re-arm the moving animation off the global geo source (0x44bb0).
    int RearmMoving(); // 0x44bb0

    // engine tail helper (__thiscall), reached via the 0x45100 ILT thunk.
    void ResolveMovingAnimation(); // 0x45100

    // Past the 0x40 CUserLogic base. m_38 is the inherited CUserLogic::m_38
    // (anim player); CString m_54 is CWarlord's own destructible member.
    char m_pad40[0x54 - 0x40];
    CString m_54; // +0x54  destructible string member
};

#endif // GRUNTZ_CWARLORD_H

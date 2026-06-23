#include <rva.h>
// CWarlord.cpp - CWarlord, the AI fort-warlord entity (RTTI CWarlord).
//
// CWarlord::LoadAttributes is the warlord's per-tick attribute/threat update:
//   1. Re-arm the geometry sub-player off the global default geo source
//      (m_38->m_1a0.SetGeoSourceR(g_defaultGeo)); bail (return 0) if not ready.
//   2. Skip the threat scan when the registry is in single-player mode
//      (g_gameReg->m_134 == 1).
//   3. Otherwise measure the nearest-enemy distance via a registry helper
//      (g_gameReg->m_68->NearestEnemyDist(...)) against the "Warlordz/PanicRadius"
//      CButeMgr config (default 64); if inside the panic radius, raise the fort
//      alert (NotifyFortUnderAttack) and return.
//   4. Past a cooldown window (the 64-bit g_645588 game-clock clamp vs m_88/m_90),
//      randomly resolve an idle (rand()%10 < 5) or battlecry animation.
//
// Returns int 0 on every path (Ghidra mislabeled the return as void; the body's
// xor eax,eax on each exit recovers the true int-returning shape).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing. Plain /O2 /MT leaf (no destructible stack object, no /GX frame).

// ---------------------------------------------------------------------------
// External engine globals (reloc-masked DATA symbols).
// ---------------------------------------------------------------------------

// The global CButeMgr text-config tree (the singleton). Modeled minimally (only
// the GetIntDef getter the warlord uses) so the `ecx=&g_buteMgr; call GetIntDef`
// shape reloc-masks against the already-matched CButeMgr::GetIntDef (butemgr unit).
// Win32-only here (the stub aggregate forbids <Mfc.h> after <windows.h>).
class CButeMgr {
public:
    int GetIntDef(char* tag, char* key, int def);
};

DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The global default geometry source the sub-player setter consumes.
DATA(0x002bf3bc)
extern int g_defaultGeo;

// The running game clock (low 32 bits of a 64-bit counter; the high half lives
// in the byte after, read together as __int64 in the cooldown clamp).
extern "C" unsigned int g_645588;

// The "Warlordz" config group + the "PanicRadius" key (original source string
// literals; objdiff matches these .data relocations by value).
#define s_Warlordz "Warlordz"
#define s_PanicRadius "PanicRadius"

// ---------------------------------------------------------------------------
// The geometry sub-player @player+0x1a0. SetGeoSourceR returns 1 when ready
// (engine FUN_0055c360, __thiscall ret 4).
// ---------------------------------------------------------------------------
class CWarlordAnimSub {
public:
    int SetGeoSourceR(int src);
};

class CWarlordAnimPlayer {
public:
    char m_pad00[0x1a0];
    CWarlordAnimSub m_1a0; // +0x1a0  geometry sub-player
};

// ---------------------------------------------------------------------------
// The game-registry singleton's threat/spatial helper (g_gameReg->m_68, read via
// raw offset off the shared CGameReg defined in ApiCallers.cpp - that struct's
// m_134 covers the game-mode gate; +0x68 falls inside its padding so the helper
// is reached by offset). g_gameReg/CGameReg are the shared aggregate definitions.
// ---------------------------------------------------------------------------
class CRegThreatHelper {
public:
    // engine FUN_0047d1d0, __thiscall ret 0xc - nearest-enemy squared distance
    // from (x, y) over the warlord's owner index.
    int NearestEnemyDist(int owner, int x, int y);
};

// ---------------------------------------------------------------------------
// The warlord's owner/state sub-object at CWarlord+0x10.
// ---------------------------------------------------------------------------
struct CWarlordOwner {
    char m_pad00[0x5c];
    int m_5c; // +0x5c  position x
    int m_60; // +0x60  position y
    char m_pad64[0x124 - 0x64];
    int m_124; // +0x124  owner index
};

// ---------------------------------------------------------------------------
// CWarlord
// ---------------------------------------------------------------------------
class CWarlord {
public:
    CWarlord(int);
    int LoadAttributes();
    void LoadAttributes2();

    // tail helpers (engine, __thiscall).
    void NotifyFortUnderAttack();      // 0x45270
    void ResolveIdleAnimation();       // 0x45960
    void ResolveBattlecryAnimation();  // 0x45b60

    char m_pad00[0x10];
    CWarlordOwner* m_10;        // +0x10  owner/state sub-object
    char m_pad14[0x38 - 0x14];
    CWarlordAnimPlayer* m_38;  // +0x38  animation player
    char m_pad3c[0x88 - 0x3c];
    int m_88; // +0x88  cooldown stamp lo (64-bit with m_8c)
    int m_8c; // +0x8c  cooldown stamp hi
    int m_90; // +0x90  cooldown window lo (64-bit with m_94)
    int m_94; // +0x94  cooldown window hi
};

extern "C" int rand(void);

// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x00042d40, 0x73e)
CWarlord::CWarlord(int) {}

RVA(0x00044c00, 0xc6)
int CWarlord::LoadAttributes() {
    if (m_38->m_1a0.SetGeoSourceR(g_defaultGeo) != 1) {
        return 0;
    }

    CGameReg* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CWarlordOwner* o = m_10;
        CRegThreatHelper* helper = *(CRegThreatHelper**)((char*)reg + 0x68);
        int dist = helper->NearestEnemyDist(o->m_124, o->m_5c, o->m_60);
        if (dist < g_buteMgr.GetIntDef(s_Warlordz, s_PanicRadius, 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if ((__int64)(unsigned)g_645588 - *(__int64*)&m_88 >= *(__int64*)&m_90) {
        if (rand() % 10 < 5) {
            ResolveIdleAnimation();
            return 0;
        }
        ResolveBattlecryAnimation();
    }
    return 0;
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00044d10, 0x106)
void CWarlord::LoadAttributes2() {}

#include <rva.h>
// CAttract.cpp - CAttract, the attract/title-screen state driver.
//
// LoadTitleConfig(mode) wires up the attract sequence:
//   * mode != 2: pick a random TITLE state index off the game registry
//     (g_gameReg->m_80 % g_attractStateCount + 1), format the "STATEZ_ATTRACT"
//     and "TITLE%d" names, resolve the attract state by name (m_08->LookupState),
//     swap it into m_2c, run the title fade-in (FUN_004fa1f0), then push the
//     "Menu"/"BrightnessPercent" CButeMgr value into the menu brightness sink.
//   * mode == 2: take the menu-exit path (the m_18 brightness sink) instead.
//   In both cases the menu page is (re)built (FUN_004fa8f0), the cursor is forced
//   visible (PTR_ShowCursor loop), and the final stage is committed
//   (FUN_004a05a0). Returns 1 on success, 0 on either early-out.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing. Plain /O2 /MT leaf (sprintf + thiscall dispatch, no /GX frame).

// ---------------------------------------------------------------------------
// External engine globals (reloc-masked DATA symbols).
// ---------------------------------------------------------------------------

// The CButeMgr text-config singleton. Bound here under a TU-local name
// (g_attractButeMgr / CAttractButeMgr) so it does not collide with CWarlord.cpp's
// `class CButeMgr` + `extern CButeMgr g_buteMgr` in the same All.cpp aggregate TU.
// The GetIntDef call is a reloc-masked rel32 and the singleton store a masked
// DIR32, so naming is matching-neutral against the matched butemgr getter.
class CAttractButeMgr {
public:
    int GetIntDef(char* tag, char* key, int def);
};

DATA(0x002453d8)
extern CAttractButeMgr g_attractButeMgr;

// The game registry singleton (g_gameReg, CGameReg*) is already declared in the
// aggregate by ApiCallers.cpp; its +0x80 attract counter is reached by raw
// offset off that shared pointer (the field falls inside CGameReg's padding).

// The attract-state count divisor (DAT_00645534, a writable global int).
DATA(0x00245534)
extern int g_attractStateCount;

// The "ShowCursor" Win32 import slot the engine calls indirectly to force the
// cursor visible (PTR_ShowCursor_006c44c4).
typedef int(__stdcall* ShowCursorFn)(int);
DATA(0x002c44c4)
extern ShowCursorFn g_ShowCursor;

// Source string literals (objdiff matches these .data relocations by value).
#define s_STATEZ_ATTRACT "STATEZ_ATTRACT"
#define s_TITLE_d "TITLE%d"
#define s_Menu "Menu"
#define s_BrightnessPercent "BrightnessPercent"
#define s_SOUNDZ "SOUNDZ"
#define s_ATTRACT "ATTRACT"
#define s_UNDERSCORE "_"

extern "C" int sprintf(char* buf, const char* fmt, ...);

// ---------------------------------------------------------------------------
// The video-mode sub-object at CAttract+0x4. Its first method (engine
// FUN_0048ddd0, __thiscall ret 4 — the RestoreVideoMode shape, called with 0)
// re-asserts the display mode when (re)entering the attract scene.
// ---------------------------------------------------------------------------
class CAttractVideo {
public:
    int RestoreVideoMode(int save);
};

// ---------------------------------------------------------------------------
// The attract state machine at CAttract+0x8 (engine FUN_0053c030, __thiscall
// ret 4): resolves an attract state object from its name.
// ---------------------------------------------------------------------------
class CAttractState; // resolved state object (LookupState result)

class CAttractStateMgr {
public:
    CAttractState* LookupState(char* name);
};

// The resolved attract state object. EnterAttractMode loads its "SOUNDZ" set
// (engine FUN_0053a230, __thiscall ret 4), yielding an opaque sound handle that
// is then registered on the menu page.
class CAttractState {
public:
    void* LoadSoundz(char* name);
};

// ---------------------------------------------------------------------------
// The menu/brightness sink chain rooted at CAttract+0xc. m_04's sub-objects hold
// the brightness targets (their m_2c) that take the CButeMgr brightness value,
// and m_04 itself drives the page enter/exit transitions (engine __thiscall,
// ret 0). The brightness setter is engine FUN_0053f460 (__thiscall ret 8).
// ---------------------------------------------------------------------------
class CMenuBrightnessTarget {
public:
    void SetBrightness(int value, int flags);
};

struct CMenuBrightnessHolder {
    char m_pad00[0x2c];
    CMenuBrightnessTarget* m_2c; // +0x2c  brightness target
};

class CMenuPage {
public:
    void TransEnter(); // FUN_00558e40  (mode == 2: enter, runs first)
    void TransTitle(); // FUN_00558e90  (mode != 2: after brightness)
    void TransExit();  // FUN_00558ee0  (mode == 2: after brightness)

    char m_pad00[0x14];
    CMenuBrightnessHolder* m_14; // +0x14  title brightness holder
    CMenuBrightnessHolder* m_18; // +0x18  menu  brightness holder
};

// The attract registrar at CMenuRoot+0x28: EnterAttractMode hands it the loaded
// sound handle plus the "ATTRACT"/"_" tags (engine FUN_00557ee0, __thiscall
// ret 0xc) so the attract page is wired into the active menu.
class CAttractRegistrar {
public:
    int Register(void* sound, char* type, char* sep);
};

struct CMenuRoot {
    char m_pad00[0x4];
    CMenuPage* m_04; // +0x4  active menu page
    char m_pad08[0x28 - 0x8];
    CAttractRegistrar* m_28; // +0x28  attract page registrar
};

// The title-brightness target's preset/reset method (engine FUN_0053e760,
// __thiscall ret 4) called once before the fade with arg 0 — the second method
// on the same +0x2c brightness target whose SetBrightness LoadTitleConfig drives.
class CMenuBrightnessReset {
public:
    void Reset(int value);
};

// ---------------------------------------------------------------------------
// CAttract — a polymorphic engine state. The vptr sits at +0x0 (inside the
// former m_pad00[8]); Activate() dispatches its own vtable slot 3 (+0xc) as the
// pre-flight gate. Three placeholder virtuals (slots 0..2, empty inline, no
// RVA) put the gate at the right index so `mov eax,[this]; call [eax+0xc]`
// falls out reloc-masked (docs/patterns/dummy-virtual-slots.md). Member offsets
// are preserved (vptr replaces the first 4 pad bytes; m_08 stays at +0x8).
// ---------------------------------------------------------------------------
class CAttract {
public:
    virtual void vf_slot0();
    virtual void vf_slot1();
    virtual void vf_slot2();
    virtual int vf_OnActivate(); // slot 3 (+0xc) — the pre-flight gate

    int EnterAttractMode(int a, int b, int mode);
    void vfunc_10(int);
    int LoadTitleConfig(int mode);
    int Activate();

    // The pre-flight gate for EnterAttractMode (engine FUN_004f9ea0, non-virtual
    // __thiscall ret 0xc, reached via ILT thunk): readies the scene from the
    // three caller args; a zero result aborts the entry.
    int LoadAttractScene(int a, int b, int mode); // FUN_004f9ea0

    // engine tail helpers (__thiscall, reached via ILT thunks).
    int FadeInTitle(char* name, int a, int b, int c, int d, int e); // FUN_004fa1f0
    int BuildMenuPage(int x, int w, int h, int flag);               // FUN_004fa8f0
    void CommitStage();                                             // FUN_004a05a0

    CAttractVideo* m_04;    // +0x4   video-mode sub-object (vptr occupies +0x0)
    CAttractStateMgr* m_08; // +0x8   attract state machine
    CMenuRoot* m_0c;        // +0xc   menu root
    char m_pad10[0x2c - 0x10];
    CAttractState* m_2c; // +0x2c  active attract state slot (scratch)
    char m_pad30[0x1b8 - 0x30];
    int m_1b8; // +0x1b8  attract-entry flag (always cleared on entry)
    int m_1bc; // +0x1bc  attract-active flag (clear when mode == 3, else set)
};

inline void CAttract::vf_slot0() {}
inline void CAttract::vf_slot1() {}
inline void CAttract::vf_slot2() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00039160, 0x46)
void CAttract::vfunc_10(int) {}

// CAttract::LoadTitleConfig - configure the attract/title sequence.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; retail emits a separate inline `xor eax,eax` for the FadeInTitle
// fail return-0, the recompile reuses the already-zero eax. Not steerable by source.
RVA(0x000a03f0, 0x14b)
int CAttract::LoadTitleConfig(int mode) {
    char stateName[0x20];
    char titleName[0x20];

    if (mode != 2) {
        int idx = *(int*)((char*)g_gameReg + 0x80) % g_attractStateCount + 1;
        sprintf(stateName, s_STATEZ_ATTRACT);
        sprintf(titleName, s_TITLE_d, idx);

        CAttractState* saved = m_2c;
        CAttractState* state = m_08->LookupState(stateName);
        m_2c = state;
        if (state == 0) {
            return 0;
        }

        int faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
        m_2c = saved;
        if (faded == 0) {
            return 0;
        }

        CMenuBrightnessTarget* tgt = m_0c->m_04->m_14->m_2c;
        tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
        m_0c->m_04->TransTitle();
    } else {
        m_0c->m_04->TransEnter();
        CMenuBrightnessTarget* tgt = m_0c->m_04->m_18->m_2c;
        tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
        m_0c->m_04->TransExit();
    }

    BuildMenuPage(0x50, 0x3e8, 0, 1);
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(1) < 0) {
        do {
        } while (showCursor(1) < 0);
    }
    CommitStage();
    return 1;
}

// CAttract::Activate - virtual attract-screen (re)entry. Gates on slot-3
// (vf_OnActivate); if it fails, returns that result. Otherwise resets the title
// brightness target, picks a random TITLE state off the registry, resolves it
// (m_08->LookupState), runs the title fade, sets menu brightness, transitions
// the page (TransTitle), rebuilds the menu page, forces the cursor visible, and
// returns 1. Mirrors LoadTitleConfig(mode != 2) sans the final CommitStage.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; on the FadeInTitle fail path retail emits a fresh inline
// `xor eax,eax` (return 0) while the recompile reuses the already-zero FadeInTitle
// result in eax (bare pop/ret). Same non-steerable wall as the sibling LoadTitleConfig.
RVA(0x000a0a30, 0x110)
int CAttract::Activate() {
    char stateName[0x20];
    char titleName[0x20];

    int gate = vf_OnActivate();
    if (gate == 0) {
        return gate;
    }

    ((CMenuBrightnessReset*)m_0c->m_04->m_14->m_2c)->Reset(0);

    int idx = *(int*)((char*)g_gameReg + 0x80) % g_attractStateCount + 1;
    sprintf(stateName, s_STATEZ_ATTRACT);
    sprintf(titleName, s_TITLE_d, idx);

    CAttractState* saved = m_2c;
    CAttractState* state = m_08->LookupState(stateName);
    m_2c = state;
    if (state == 0) {
        return 0;
    }

    int faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = saved;
    if (faded == 0) {
        return 0;
    }

    CMenuBrightnessTarget* tgt = m_0c->m_04->m_14->m_2c;
    tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
    m_0c->m_04->TransTitle();

    BuildMenuPage(0x50, 0x3e8, 0, 1);
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(1) < 0) {
        do {
        } while (showCursor(1) < 0);
    }
    return 1;
}

// CAttract::EnterAttractMode - enter (or re-enter) the attract scene.
// Gates on LoadAttractScene(a, b, mode); on failure returns that result.
// Otherwise hides the cursor (decrement the display count until negative),
// re-asserts the video mode, resolves the "STATEZ_ATTRACT" state (stored into
// m_2c), loads its "SOUNDZ" set, registers the resulting sound handle on the
// menu page under the "ATTRACT"/"_" tags, hides the cursor again, then sets the
// entry flags: m_1b8 is always cleared, and m_1bc is cleared when mode == 3
// (else set to 1). Returns 1 on success, 0 on any early-out.
RVA(0x00013fb0, 0xd5)
int CAttract::EnterAttractMode(int a, int b, int mode) {
    if (LoadAttractScene(a, b, mode) == 0) {
        return 0;
    }

    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }

    m_04->RestoreVideoMode(0);

    CAttractState* state = m_08->LookupState(s_STATEZ_ATTRACT);
    m_2c = state;
    if (state == 0) {
        return 0;
    }

    void* sound = state->LoadSoundz(s_SOUNDZ);
    if (sound == 0) {
        return 0;
    }

    m_0c->m_28->Register(sound, s_ATTRACT, s_UNDERSCORE);

    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }

    if (mode == 3) {
        m_1bc = 0;
        m_1b8 = 0;
    } else {
        m_1bc = 1;
        m_1b8 = 0;
    }
    return 1;
}

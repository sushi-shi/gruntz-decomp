// CAttract.cpp - CAttract, the attract/title-screen game-state (RTTI
// .?AVCAttract@@, vtable @0x5ea194; a CState leaf). Promoted out of the
// src/Stub/ aggregate into its own EH unit because the `??1` destructor carries
// the /GX SEH frame. See CAttract.h for the layout + the sub-object models.
//
// Slots it implements (vtable not diffed; the slot order is anchored by CState):
//   ~CAttract()          0x08cd90  slot 0  EH `??1`  (vtable restore + base chain)
//   ReleaseResources()   0x0140d0  slot 2  resource release (Free + Release + base)
//   Vslot07()            0x0147b0  slot 7  host/paint poll (base paint -> flip/blit)
//   FrameSlot28(i32)     0x014340  slot 10 per-frame voice poll
//   EnterAttractMode     0x013fb0  slot 1  (reached non-virtually; ret 0xc)
// Non-virtual title/menu logic: RefreshTitle / LoadTitleConfig / Activate /
// RunTitle. Field names are placeholders; only OFFSETS + code bytes matter.
#include <Gruntz/CString.h> // MFC CString (the title-roll formats into one); MFC-first
#include <Gruntz/CAttract.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// External engine globals (reloc-masked DATA symbols).
// ---------------------------------------------------------------------------

// The CButeMgr text-config singleton, bound under a TU-local name so it does not
// collide with the `class CButeMgr` definitions in other units.
class CAttractButeMgr {
public:
    i32 GetIntDef(char* tag, char* key, i32 def);
};

DATA(0x002453d8)
extern CAttractButeMgr g_attractButeMgr;

// The game registry singleton; its +0x80 attract counter is reached by raw offset.
struct CGameReg;
DATA(0x00245460)
extern CGameReg* g_gameReg;

// The attract-state count divisor (DAT_00645534, a writable global int).
DATA(0x00245534)
extern i32 g_attractStateCount;

// The "ShowCursor" Win32 import slot (PTR_ShowCursor_006c44c4).
typedef i32(__stdcall* ShowCursorFn)(i32);
DATA(0x002c44c4)
extern ShowCursorFn g_ShowCursor;

// Source string literals (objdiff matches these .data relocations by value).
#define s_STATEZ_ATTRACT "STATEZ_ATTRACT"
#define s_TITLE_d "TITLE%d"
#define s_TITLE "TITLE"
#define s_Menu "Menu"
#define s_BrightnessPercent "BrightnessPercent"
#define s_SOUNDZ "SOUNDZ"
#define s_ATTRACT "ATTRACT"
#define s_UNDERSCORE "_"

extern "C" i32 sprintf(char* buf, const char* fmt, ...);

// ===========================================================================
// Virtual-slot overrides.
// ===========================================================================

// CAttract::ReleaseResources() (slot 2 / +0x8, 0x0140d0): free the registrar's
// pooled resource (if any), release the attract page ("ATTRACT"/"_"), then chain
// the base CState resource teardown. The menu root (m_c) is re-read for the
// Release access (retail does not cache it).
RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    CAttractRegistrar* reg = ((CMenuRoot*)m_c)->m_28;
    if (reg->m_2c) {
        reg->m_2c->Free();
    }
    ((CMenuRoot*)m_c)->m_28->Release(s_ATTRACT, s_UNDERSCORE);
    CState::ReleaseResources();
}

// CAttract::FrameSlot28(arg) (slot 10 / +0x28, 0x014340): per-frame voice poll.
// If the host's voice (m_1b8->m_10) is playing, (re)start it (Restart(0,0x1f4,1)),
// then if it is still playing stop the registrar's pooled resource (Stop(-1)) and
// loop while the voice keeps reporting playing. Returns 1.
// @early-stop
// regalloc back-edge coin-flip (docs/patterns/zero-register-pinning.md): body
// byte-identical except the final loop-back IsPlaying load - retail re-reads
// m_1b8 through eax (8b 86 .. 8b 48 10), the recompile through ecx (8b 8e .. 8b 49
// 10). A pure allocator choice on the do-while back-edge; no source lever flips it.
RVA(0x00014340, 0x71)
i32 CAttract::FrameSlot28(i32 arg) {
    if (m_1b8 == 0) {
        return 1;
    }
    if (!m_1b8->m_10->IsPlaying()) {
        return 1;
    }
    m_1b8->m_10->Restart(0, 0x1f4, 1);
    if (!m_1b8->m_10->IsPlaying()) {
        return 1;
    }
    do {
        CAttractPooledRes* r = ((CMenuRoot*)m_c)->m_28->m_2c;
        if (r) {
            r->Stop(-1);
        }
    } while (m_1b8->m_10->IsPlaying());
    return 1;
}

// CAttract::RollTitleByPage (0x14520): gate on the menu page's IsLoaded; if loaded,
// hide the cursor, pick a random TITLE%d index off the game-reg attract counter, and
// run that title sequence. The CString format local forces the /GX EH frame.
RVA(0x00014520, 0xc3)
i32 CAttract::RollTitleByPage() {
    if (((CMenuRoot*)m_c)->m_04->IsLoaded() == 0) {
        return 0;
    }
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    i32 idx = *(i32*)((char*)g_gameReg + 0x80) % g_attractStateCount + 1;
    CString s;
    s.Format(s_TITLE_d, idx);
    return RunTitleSeq((char*)(const char*)s, 0, 0, 1, 0);
}

// CAttract::RollTitleByV3 (0x14630): identical to RollTitleByPage but gated on the
// slot-3 virtual (Vfunc3) instead of the page IsLoaded.
RVA(0x00014630, 0xbd)
i32 CAttract::RollTitleByV3() {
    if (Vfunc3() == 0) {
        return 0;
    }
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    i32 idx = *(i32*)((char*)g_gameReg + 0x80) % g_attractStateCount + 1;
    CString s;
    s.Format(s_TITLE_d, idx);
    return RunTitleSeq((char*)(const char*)s, 0, 0, 1, 0);
}

// CAttract::Vslot07() (slot 7 / +0x1c, 0x0147b0): the host/paint poll. Gate on the
// slot-3 virtual (Vfunc3); bail if the menu root (m_c) is null; run the base
// CState::Vslot07() paint; force the cursor hidden; flip the render target; blit
// the title frame onto the menu page. Returns 1.
RVA(0x000147b0, 0x6a)
i32 CAttract::Vslot07() {
    if (!Vfunc3()) {
        return 0;
    }
    if (!m_c) {
        return 0;
    }
    if (!CState::Vslot07()) {
        return 0;
    }
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    ((CMenuRoot*)m_c)->m_04->m_10->m_2c->Flip(0);
    ((CMenuRoot*)m_c)->m_04->BlitFrom(((CMenuRoot*)m_c)->m_04->m_14);
    return 1;
}

// CAttract::~CAttract() (`??1`, 0x08cd90): the EH-framed destructor. MSVC emits
// the CAttract-vtable restore + slot-2 release (ReleaseResources, statically
// bound) + CState-vtable restore + base cleanup; the body just runs the release.
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}

// ===========================================================================
// Non-virtual attract/title logic.
// ===========================================================================

// CAttract::RunTitle(...) (0x0fa300, 5 args, ret 0x14): the title-render entry.
// Bail (0) if the menu root (m_c), state machine (m_8), or active state (m_2c) is
// null; otherwise flip the menu page's render target and return 1.
// @early-stop
// regalloc chain-staging coin-flip (docs/patterns/zero-register-pinning.md): body
// byte-identical except ONE modrm in the m_04->m_10->m_2c->Flip chain - retail
// stages the penultimate deref through eax (8b 40 10) then ecx, the recompile
// switches to ecx one deref early (8b 48 10). The SAME inline chain matches in
// Vslot07 (different surrounding pressure) - a pure allocator choice, no source lever.
RVA(0x000fa300, 0x3a)
i32 CAttract::RunTitle(i32 a, i32 b, i32 c, i32 d, i32 e) {
    if (!m_c) {
        return 0;
    }
    if (!m_8) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    ((CMenuRoot*)m_c)->m_04->m_10->m_2c->Flip(0);
    return 1;
}

// CAttract::EnterAttractMode - enter (or re-enter) the attract scene.
// Gates on LoadAttractScene(a, b, mode); on failure returns that result.
// Otherwise hides the cursor, re-asserts the video mode, resolves the
// "STATEZ_ATTRACT" state (stored into m_2c), loads its "SOUNDZ" set, registers
// the sound handle on the menu page under the "ATTRACT"/"_" tags, hides the
// cursor again, then sets the entry flags: m_1b8 is always cleared, m_1bc is
// cleared when mode == 3 (else set to 1). Returns 1 on success, 0 on early-out.
RVA(0x00013fb0, 0xd5)
i32 CAttract::EnterAttractMode(i32 a, i32 b, i32 mode) {
    if (LoadAttractScene(a, b, mode) == 0) {
        return 0;
    }

    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }

    ((CAttractVideo*)m_4)->RestoreVideoMode(0);

    CAttractState* state = ((CAttractStateMgr*)m_8)->LookupState(s_STATEZ_ATTRACT);
    m_2c = (i32)state;
    if (state == 0) {
        return 0;
    }

    void* sound = state->LoadSoundz(s_SOUNDZ);
    if (sound == 0) {
        return 0;
    }

    ((CMenuRoot*)m_c)->m_28->Register(sound, s_ATTRACT, s_UNDERSCORE);

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

// CAttract::RefreshTitle - re-prime the attract title scene. Resets the scene
// slot off m_4->m_48 (PrimeScene then RestoreScene), re-resolves the
// "STATEZ_ATTRACT" state into m_2c, runs the title sequence with the bare "TITLE"
// tag, and returns 1.
RVA(0x00039160, 0x46)
i32 CAttract::RefreshTitle(i32 unused) {
    ((CAttractVideo*)m_4)->m_48->PrimeScene();
    ((CAttractVideo*)m_4)->m_48->RestoreScene();
    m_2c = (i32)((CAttractStateMgr*)m_8)->LookupState(s_STATEZ_ATTRACT);
    RunTitleSeq(s_TITLE, 0, 0, 1, 0);
    return 1;
}

// CAttract::LoadTitleConfig - configure the attract/title sequence.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; retail emits a separate inline `xor eax,eax` for the FadeInTitle
// fail return-0, the recompile reuses the already-zero eax. Not steerable by source.
RVA(0x000a03f0, 0x14b)
i32 CAttract::LoadTitleConfig(i32 mode) {
    char stateName[0x20];
    char titleName[0x20];

    if (mode != 2) {
        i32 idx = *(i32*)((char*)g_gameReg + 0x80) % g_attractStateCount + 1;
        sprintf(stateName, s_STATEZ_ATTRACT);
        sprintf(titleName, s_TITLE_d, idx);

        CAttractState* saved = (CAttractState*)m_2c;
        CAttractState* state = ((CAttractStateMgr*)m_8)->LookupState(stateName);
        m_2c = (i32)state;
        if (state == 0) {
            return 0;
        }

        i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
        m_2c = (i32)saved;
        if (faded == 0) {
            return 0;
        }

        CMenuBrightnessTarget* tgt = ((CMenuRoot*)m_c)->m_04->m_14->m_2c;
        tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
        ((CMenuRoot*)m_c)->m_04->TransTitle();
    } else {
        ((CMenuRoot*)m_c)->m_04->TransEnter();
        CMenuBrightnessTarget* tgt = ((CMenuRoot*)m_c)->m_04->m_18->m_2c;
        tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
        ((CMenuRoot*)m_c)->m_04->TransExit();
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
// (Vfunc3); if it fails, returns that result. Otherwise resets the title
// brightness target, picks a random TITLE state off the registry, resolves it,
// runs the title fade, sets menu brightness, transitions the page, rebuilds the
// menu page, forces the cursor visible, and returns 1.
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md):
// body byte-exact; on the FadeInTitle fail path retail emits a fresh inline
// `xor eax,eax` (return 0) while the recompile reuses the already-zero FadeInTitle
// result in eax. Same non-steerable wall as the sibling LoadTitleConfig.
RVA(0x000a0a30, 0x110)
i32 CAttract::Activate() {
    char stateName[0x20];
    char titleName[0x20];

    i32 gate = Vfunc3();
    if (gate == 0) {
        return gate;
    }

    ((CMenuBrightnessReset*)((CMenuRoot*)m_c)->m_04->m_14->m_2c)->Reset(0);

    i32 idx = *(i32*)((char*)g_gameReg + 0x80) % g_attractStateCount + 1;
    sprintf(stateName, s_STATEZ_ATTRACT);
    sprintf(titleName, s_TITLE_d, idx);

    CAttractState* saved = (CAttractState*)m_2c;
    CAttractState* state = ((CAttractStateMgr*)m_8)->LookupState(stateName);
    m_2c = (i32)state;
    if (state == 0) {
        return 0;
    }

    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (i32)saved;
    if (faded == 0) {
        return 0;
    }

    CMenuBrightnessTarget* tgt = ((CMenuRoot*)m_c)->m_04->m_14->m_2c;
    tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
    ((CMenuRoot*)m_c)->m_04->TransTitle();

    BuildMenuPage(0x50, 0x3e8, 0, 1);
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(1) < 0) {
        do {
        } while (showCursor(1) < 0);
    }
    return 1;
}

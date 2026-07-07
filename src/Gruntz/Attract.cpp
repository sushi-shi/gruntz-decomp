// Attract.cpp - CAttract, the attract/title-screen game-state (RTTI
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
#include <Gruntz/String.h> // MFC CString (the title-roll formats into one); MFC-first
#include <Gruntz/GruntzMgr.h>
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Gruntz/Attract.h>
#include <Gruntz/GameRegistry.h>       // the ONE game-registry shape (CGameRegistry / g_gameReg)
#include <DDrawMgr/DDrawSubMgrPages.h> // the ONE CDDrawSubMgrPages shape (Method_158b40)
#include <ddraw.h>                     // IDirectDrawSurface (the flip surface's raw +0x8 COM iface)
#include <rva.h>
#include <Globals.h>

#include <stdio.h> // sprintf (the "\SCREENZ\%s" path formatter)

// The attract-cue registrar IS a CDDrawSubMgrLeafScan (header-less); local decl (exact arg types).
class DirNode;
class CDDrawSubMgrLeafScan {
public:
    i32 ScanTree_157ee0(DirNode* n, const char* key, const char* g); // 0x157ee0
    i32 RemoveKeysEqual_157c70(const char* key, const char* g);      // 0x157c70
};

// ---------------------------------------------------------------------------
// External engine globals (reloc-masked DATA symbols).
// ---------------------------------------------------------------------------

// The CButeMgr text-config singleton (canonical CButeMgr, include/Bute/ButeMgr.h);
// bound under a TU-local variable name (same 0x6453d8 datum as g_buteMgr).
DATA(0x002453d8)
extern CButeMgr g_attractButeMgr;

// The game registry singleton (canonical CGameRegistry, <Gruntz/GameRegistry.h>): its
// +0x80 launch counter (m_numRuns, "Num_Runs") selects the TITLE state. The retail
// reads it off the canonical g_gameReg pointer at ds:0x64556c (verified in InputVirtual/
// Activate/LoadTitleConfig: mov ecx,ds:0x64556c; mov eax,[ecx+0x80]); the DATA() RVA
// below (0x245460) is a pre-existing mis-transcription that stays reloc-masked (operand
// masked in objdiff, code bytes unaffected) - fixing it to 0x24556c is a separate P6
// symbol-pairing concern, out of this view-fold's scope.
DATA(0x00245460)
extern CGameRegistry* g_gameReg;

// The attract-state count divisor (DAT_00645534, a writable global int).
DATA(0x00245534)
extern i32 g_attractStateCount;

// The "ShowCursor" Win32 import slot (PTR_ShowCursor_006c44c4).
typedef i32(WINAPI* ShowCursorFn)(i32);
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
#define s_SCREENZ_PCT_S "\\SCREENZ\\%s"

// FadeInTitle's resolved-state object (m_2c re-typed): its ResolveScreen
// (FUN_00520120) maps the "\SCREENZ\%s" path + a screen-type tag to a fade page.
class CAttractScreenObj {
public:
    void* ResolveScreen(char* path, void* tag); // 0x120120
};

// The screen-type tag (DAT_00504358) ResolveScreen keys off.

// The menu page worker (m_c->m_04 re-typed): its fader (0x158b40, ret 8) runs the
// title fade, returning non-zero when the fade is still in progress. Named to retail
// (?Method_158b40@CDDrawSubMgrPages) so the call pairs exactly. See CDDrawSubMgrPages.h.

// ---------------------------------------------------------------------------
// FramePoll (0x143e0) sub-object models.
// ---------------------------------------------------------------------------

// The render-busy check dispatches IDirectDrawSurface::IsLost (slot 24, +0x60,
// __stdcall self-on-stack) on the flip surface's held DirectDraw COM interface
// (CDDSurface::m_8) to see whether the page still needs a restore.

// The per-frame actor list (global pointer DAT_00645574): m_count at +0x4 and an
// inline array of actor pointers at +0x8. Each actor's slot-4 (+0x10) virtual is
// the per-frame Update; its +0x2ac flags word raises 0x100 to request the exit.
class AttractActor {
public:
    virtual void Vslot00();
    virtual void Vslot01();
    virtual void Vslot02();
    virtual void Vslot03();
    virtual void Update(); // slot 4 (+0x10)
    char m_pad04[0x2ac - 0x4];
    i32 m_2ac; // +0x2ac flags
};
struct AttractActorList {
    char m_pad00[0x4];
    i32 m_count;             // +0x04
    AttractActor* m_data[1]; // +0x08  inline pointer array
};

// The per-frame time delta (countdown source for m_idleTimer). C linkage so the symbol
// pairs with the target's _g_645584 (the convention across the gamemode units).
extern "C" {
    DATA(0x00245584)
    extern u32 g_645584;
}

// The owner back-ptr (CState::m_4) viewed as the game manager: ReportError fires a
// WM_COMMAND on idle (0x8006/0x3e8) and m_4->m_4 chains to the top-level HWND.
struct AttractWndHolder {
    char m_pad00[0x4];
    HWND m_4; // +0x04  top-level HWND
};

// PostMessageA reached through the IAT slot (matches the engine's ff15 indirect).
typedef i32(WINAPI* PostMessageFn)(void* hwnd, u32 msg, u32 wparam, i32 lparam);
DATA(0x002c44c8)
extern PostMessageFn g_pPostMessageA;

// ===========================================================================
// Virtual-slot overrides.
// ===========================================================================

// CAttract::ReleaseResources() (slot 2 / +0x8, 0x0140d0): free the registrar's
// pooled resource (if any), release the attract page ("ATTRACT"/"_"), then chain
// the base CState resource teardown. The menu root (m_c) is re-read for the
// Release access (retail does not cache it).
RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    CAttractRegistrar* reg = menuRoot()->m_28;
    if (reg->m_2c) {
        reg->m_2c->Free();
    }
    ((CDDrawSubMgrLeafScan*)menuRoot()->m_28)->RemoveKeysEqual_157c70(s_ATTRACT, s_UNDERSCORE);
    CState::ReleaseResources();
}

// CAttract::FrameSlot28(arg) (slot 10 / +0x28, 0x014340): per-frame voice poll.
// If the host's voice (m_host->m_10) is playing, (re)start it (Restart(0,0x1f4,1)),
// then if it is still playing stop the registrar's pooled resource (Stop(-1)) and
// loop while the voice keeps reporting playing. Returns 1.
// @early-stop
// regalloc back-edge coin-flip (docs/patterns/zero-register-pinning.md): body
// byte-identical except the final loop-back IsPlaying load - retail re-reads
// m_host through eax (8b 86 .. 8b 48 10), the recompile through ecx (8b 8e .. 8b 49
// 10). A pure allocator choice on the do-while back-edge; no source lever flips it.
RVA(0x00014340, 0x71)
i32 CAttract::FrameSlot28(i32 arg) {
    if (m_host == 0) {
        return 1;
    }
    if (!m_host->m_10->IsPlaying()) {
        return 1;
    }
    m_host->m_10->Restart(0, 0x1f4, 1);
    if (!m_host->m_10->IsPlaying()) {
        return 1;
    }
    do {
        CAttractPooledRes* r = menuRoot()->m_28->m_2c;
        if (r) {
            r->Stop(-1);
        }
    } while (m_host->m_10->IsPlaying());
    return 1;
}

// CAttract::Render (slot 5 / +0x14, 0x143e0): the attract-mode per-frame poll/draw.
// If the page's render-busy object reports idle AND the InputVirtual slot reports
// idle, report the exit error (0x8006/0x3e8) and bail. Otherwise stop the registrar's
// pooled resource, tick the m_idleTimer timeout down by the frame delta, run every
// actor's Update(), and if any actor raised its 0x100 flag post the exit WM_COMMAND.
// Code byte-identical to retail (~97% fuzzy = reloc-masked plateau): the residual
// is purely cross-unit/IAT symbol-naming on three reloc operands - ReportError (a
// bare delinker label), 0x136e20 (already owned by DirectSoundMgr::winapi_136e20_
// timeGetTime; the sibling FrameSlot28 names it CAttractPooledRes::Stop too), and
// the PostMessageA IAT call (target bakes a bare absolute 0x6c44c8, no symbol).
// Not source-steerable; topic:scoring-artifact (docs/matching-patterns.md).
// @early-stop
// reloc-masked IAT/cross-unit operands only (see above); code bytes byte-exact.
RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy = menuRoot()->m_04->m_10->m_2c->m_8;
    if (busy == 0 || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(0x8006, 0x3e8);
            return 0;
        }
    }

    CAttractPooledRes* res = menuRoot()->m_28->m_2c;
    if (res) {
        res->Stop(-1);
    }

    if (g_645584 < m_idleTimer) {
        m_idleTimer -= g_645584;
    } else {
        m_idleTimer = 0;
    }

    AttractActorList* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_data[i]->Update();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_data[i]->m_2ac & 0x100) {
            g_pPostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            return 1;
        }
    }
    return 1;
}

// 0x3c220: an attract-family per-frame idle poll (PLACEHOLDER class; m_4 owner /
// m_520 idle timer). Pump (0x20db) the host, scan g_actorList for the 0x100 exit
// flag (post WM_COMMAND 0x8023 on the first match), count the idle timer m_520 down
// by the frame delta, and post WM_COMMAND 0x8027 when it expires. Returns 1.
// @early-stop
// 99.55%: every opcode/offset/branch is byte-identical. The residual is (1) the
// same PostMessageA IAT-absolute scoring artifact the sibling FramePoll documents
// (target bakes the bare 0x6c44c8, no symbol) and (2) a register-coloring coin-flip
// (pm <-> ebx/edi vs the 0x100 mask) - the documented regalloc back-edge wall
// (docs/patterns/zero-register-pinning.md). Not source-steerable; deferred.
struct CAttractIdlePoll {
    char _00[4];
    CGruntzMgr* m_4; // +0x04
    char _08[0x520 - 0x8];
    u32 m_520;   // +0x520  idle timer
    void Pump(); // 0x20db (ILT thunk; reloc-masked)
    i32 Poll();  // 0x3c220
};

RVA(0x0003c220, 0xa4)
i32 CAttractIdlePoll::Poll() {
    Pump();
    PostMessageFn pm = g_pPostMessageA;
    AttractActorList* list = g_actorList;
    i32 n = list->m_count;
    for (i32 i = 0; i < n; i++) {
        if (list->m_data[i]->m_2ac & 0x100) {
            pm(m_4->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            break;
        }
    }
    if (g_645584 >= m_520) {
        m_520 = 0;
    } else {
        m_520 -= g_645584;
    }
    if (m_520 == 0) {
        pm(m_4->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
    }
    return 1;
}

// CAttract::InputVirtual (slot 8 / +0x20, 0x14520): the random-title roll gated on
// the menu page's IsLoaded; if loaded, hide the cursor, pick a random TITLE%d index
// off the game-reg attract counter, and run that title sequence. The CString format
// local forces the /GX EH frame. (Render polls this slot each frame.)
RVA(0x00014520, 0xc3)
i32 CAttract::InputVirtual() {
    if (menuRoot()->m_04->IsLoaded() == 0) {
        return 0;
    }
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format(s_TITLE_d, idx);
    return RunTitleSeq(s, 0, 0, 1, 0);
}

// CAttract::Vslot06 (slot 6 / +0x18, 0x14630): identical to the InputVirtual roll but
// gated on the slot-3 virtual (Vfunc3) instead of the page IsLoaded.
RVA(0x00014630, 0xbd)
i32 CAttract::Vslot06() {
    if (Vfunc3() == 0) {
        return 0;
    }
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format(s_TITLE_d, idx);
    return RunTitleSeq(s, 0, 0, 1, 0);
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
    menuRoot()->m_04->m_10->m_2c->Flip(0);
    menuRoot()->m_04->BlitFrom(menuRoot()->m_04->m_14);
    return 1;
}

// CAttract::Update() (slot 4 / +0x10, 0x08cd40): the attract state's ID tag = 2.
RVA(0x0008cd40, 0x6)
GameStateId CAttract::Update() {
    return GAMESTATE_ATTRACT;
}

// CAttract::Vslot0e(a, b, c) (slot 14 / +0x38, 0x14770): post the exit WM_COMMAND
// (0x8023) to the top-level HWND (m_4->m_gameWnd->m_hwnd) unconditionally, then return 1.
RVA(0x00014770, 0x24)
i32 CAttract::Vslot0e(i32, i32, i32) {
    g_pPostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
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
    menuRoot()->m_04->m_10->m_2c->Flip(0);
    return 1;
}

// CAttract::FadeInTitle(name, a, b, c, d, e) (0x0fa1f0, 6 args, ret 0x18): resolve the
// "\SCREENZ\<name>" fade page off m_2c (with the screen-type tag), then run the page
// worker's fade (mode 2 when `e`, else 1); on `e` retry once with mode 1. ret 1 on a
// started fade, else 0.
// @early-stop
// frame-reservation + reloc wall (~74%): logic + offsets byte-exact; retail reserves an
// 0x40 frame (0xc outgoing-arg scratch below the 0x34 buf) where our cl reserves only the
// 0x34 buf, and the ResolveScreen callee (FUN_00520120) is an unnamed body that can't pair
// (reloc-masked DIR32). Not source-steerable. topic:wall.
RVA(0x000fa1f0, 0xc6)
i32 CAttract::FadeInTitle(const char* name, i32 a, i32 b, i32 c, i32 d, i32 e) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    if (!m_c) {
        return 0;
    }
    if (!m_8) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    char buf[0x34];
    sprintf(buf, s_SCREENZ_PCT_S, name);
    void* page = screenObj()->ResolveScreen(buf, &g_screenTag);
    if (page == 0) {
        return 0;
    }
    CDDrawSubMgrPages* w = (CDDrawSubMgrPages*)menuRoot()->m_04;
    if (w->Method_158b40((i32)page, e != 0 ? 2 : 1) != 0) {
        return 1;
    }
    if (e == 0) {
        return 1;
    }
    if (w->Method_158b40((i32)page, 1) != 0) {
        return 1;
    }
    return 0;
}

// CAttract::RunTitleSeq(name, a, b, c, d) (0x0fa350, 5 args, ret 0x14): the title-roll
// entry. Bail (0) if the menu root/state-machine/active-state is null; FadeInTitle the
// screen (mode 0); on success return RunTitle() != 0.
RVA(0x000fa350, 0x84)
i32 CAttract::RunTitleSeq(const char* name, i32 a, i32 b, i32 c, i32 d) {
    if (!m_c) {
        return 0;
    }
    if (!m_8) {
        return 0;
    }
    if (!m_2c) {
        return 0;
    }
    if (FadeInTitle(name, a, b, c, d, 0) == 0) {
        return 0;
    }
    return RunTitle((i32)name, a, b, c, d) != 0;
}

// CAttract::EnterAttractMode - enter (or re-enter) the attract scene.
// Gates on LoadAttractScene(a, b, mode); on failure returns that result.
// Otherwise hides the cursor, re-asserts the video mode, resolves the
// "STATEZ_ATTRACT" state (stored into m_2c), loads its "SOUNDZ" set, registers
// the sound handle on the menu page under the "ATTRACT"/"_" tags, hides the
// cursor again, then sets the entry flags: m_host is always cleared, m_activeFlag is
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

    ((CGruntzMgr*)video())->RestoreVideoMode(0);

    CAttractState* state = stateMgr()->LookupState(s_STATEZ_ATTRACT);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }

    void* sound = state->LoadSoundz(s_SOUNDZ);
    if (sound == 0) {
        return 0;
    }

    ((CDDrawSubMgrLeafScan*)menuRoot()->m_28)
        ->ScanTree_157ee0((DirNode*)sound, s_ATTRACT, s_UNDERSCORE);

    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }

    if (mode == 3) {
        m_activeFlag = 0;
        m_host = 0;
    } else {
        m_activeFlag = 1;
        m_host = 0;
    }
    return 1;
}

// CAttract::RefreshTitle - re-prime the attract title scene. Resets the scene
// slot off m_4->m_48 (PrimeScene then RestoreScene), re-resolves the
// "STATEZ_ATTRACT" state into m_2c, runs the title sequence with the bare "TITLE"
// tag, and returns 1.
RVA(0x00039160, 0x46)
i32 CAttract::RefreshTitle(i32 unused) {
    video()->m_48->PrimeScene();
    video()->m_48->RestoreScene();
    m_2c = (CResSource*)stateMgr()->LookupState(s_STATEZ_ATTRACT);
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
        i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
        sprintf(stateName, s_STATEZ_ATTRACT);
        sprintf(titleName, s_TITLE_d, idx);

        CAttractState* saved = attractState();
        CAttractState* state = stateMgr()->LookupState(stateName);
        m_2c = (CResSource*)state;
        if (state == 0) {
            return 0;
        }

        i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
        m_2c = (CResSource*)saved;
        if (faded == 0) {
            return 0;
        }

        CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_14->m_2c;
        tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
        menuRoot()->m_04->TransTitle();
    } else {
        menuRoot()->m_04->TransEnter();
        CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_18->m_2c;
        tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
        menuRoot()->m_04->TransExit();
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

    ((CMenuBrightnessReset*)menuRoot()->m_04->m_14->m_2c)->Reset(0);

    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, s_STATEZ_ATTRACT);
    sprintf(titleName, s_TITLE_d, idx);

    CAttractState* saved = attractState();
    CAttractState* state = stateMgr()->LookupState(stateName);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }

    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (CResSource*)saved;
    if (faded == 0) {
        return 0;
    }

    CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_14->m_2c;
    tgt->SetBrightness(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), 0);
    menuRoot()->m_04->TransTitle();

    BuildMenuPage(0x50, 0x3e8, 0, 1);
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(1) < 0) {
        do {
        } while (showCursor(1) < 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// The top-window paint poll (0x0fac70), re-homed from the ApiCaller stubs. This
// is a NON-VIRTUAL shared base-class method: CAttract::Vslot07 (0x0147b0),
// CMultiBootyState::ReadyAndPaint (0x01ce30, gamemode) and CGuardedDispatch::Run
// (0x01f870, boundarymisc) each `mov ecx,<this>; call 0x1136` on their OWN this,
// so +0x04 (a wnd chain -> top-level HWND at m_4->m_gameWnd->m_hwnd) is a base-layout
// field they all share. If the window is present, it runs a null Begin/EndPaint
// cycle and returns 1. Homed here (CAttract, the first/RTTI-named caller); the
// common base class it truly belongs to is not yet recovered. Offsets + code
// bytes load-bearing.
struct StatePaintWnd {
    char m_pad0[4];
    StatePaintWnd* m_4; // +0x04
};
struct StatePaintHost {
    char m_pad0[4];
    StatePaintWnd* m_4; // +0x04
    i32 Paint();
};
SIZE_UNKNOWN(StatePaintWnd);
SIZE_UNKNOWN(StatePaintHost);
RVA(0x000fac70, 0x4c)
i32 StatePaintHost::Paint() {
    if (!m_4) {
        return 0;
    }
    if (!m_4->m_4) {
        return 0;
    }
    PAINTSTRUCT ps;
    BeginPaint((HWND)m_4->m_4->m_4, &ps);
    EndPaint((HWND)m_4->m_4->m_4, &ps);
    return 1;
}

SIZE_UNKNOWN(AttractActor);
SIZE_UNKNOWN(AttractActorList);
SIZE_UNKNOWN(AttractWndHolder);
SIZE_UNKNOWN(CAttract);
SIZE_UNKNOWN(CAttractHost);
SIZE_UNKNOWN(CAttractIdlePoll);
SIZE_UNKNOWN(CAttractPooledRes);
SIZE_UNKNOWN(CAttractRegistrar);
SIZE_UNKNOWN(CAttractSceneSlot);
SIZE_UNKNOWN(CAttractScreenObj);
SIZE_UNKNOWN(CAttractState);
SIZE_UNKNOWN(CAttractStateMgr);
SIZE_UNKNOWN(CAttractVideo);
SIZE_UNKNOWN(CAttractVoice);
SIZE_UNKNOWN(CMenuBrightnessHolder);
SIZE_UNKNOWN(CMenuBrightnessReset);
SIZE_UNKNOWN(CMenuBrightnessTarget);
SIZE_UNKNOWN(CMenuRenderM10);
SIZE_UNKNOWN(CMenuRoot);

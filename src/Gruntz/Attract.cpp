// Attract.cpp - CAttract, the attract/title-screen game-state (RTTI
// .?AVCAttract@@, vtable @0x5ea194; a CState leaf) PLUS the state-services
// interval TU at retail .text [0x0fa1f0 .. 0x0fb328].
//
// wave2-H merge: the former attract(0xfa1f0 fns) + soundfxemitter +
// statedrawtext + mgrobjserialize units were a WOVEN single interval
// (TU_MIGRATION 0x0fa1f0, weave 0.36 - the CSoundFxEmitter transition emitters,
// the CState draw/shade helpers and the CMgrPersistObj serialize family
// interleave function-by-function), one original obj. The former standalone
// SaveRecordLoad.cpp's SaveRecord (0xfaff0) was a SECOND view of the same
// CMgrPersistObj (identical +0x0c..+0x1b0 layout; its Load is the read-side
// twin of Save @0xfb1c0) - folded onto the one struct here. The CAttract core
// (0x13fb0 band) already lived here; this file therefore carries the attract
// TU's two intervals until a further identity split is proven.
// NB: CPreviewState::LoadScreen (0xfab90) also belongs to this interval but
// stays in LevelPreview.cpp for now - moving it needs CPreviewState homed into
// a shared header first (it is that TU's local class); deferred, see the
// wave2-H report.
//
// CAttract slots (vtable not diffed; the slot order is anchored by CState):
//   ~CAttract()          0x08cd90  slot 0  EH `??1`  (vtable restore + base chain)
//   ReleaseResources()   0x0140d0  slot 2  resource release (Free + Release + base)
//   Vslot07()            0x0147b0  slot 7  host/paint poll (base paint -> flip/blit)
//   FrameSlot28(i32)     0x014340  slot 10 per-frame voice poll
//   EnterAttractMode     0x013fb0  slot 1  (reached non-virtually; ret 0xc)
// Field names are placeholders; only OFFSETS + code bytes matter.
#include <Gruntz/String.h> // MFC CString (the title-roll formats into one); MFC-first
#include <Gruntz/GruntzMgr.h>
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Gruntz/Attract.h>
#include <Gruntz/GameRegistry.h> // the ONE game-registry shape (CGameRegistry / g_gameReg)
#include <Gruntz/AttractActor.h> // the shared per-frame g_actorList view (also used by CDemo/CHelpState)
#include <Gruntz/ResMgr.h>       // CDrawTarget (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDrawSubMgrPages.h> // the ONE CDDrawSubMgrPages shape (Method_158b40)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair/m_frontPair->m_surface)
#include <DDrawMgr/DDSurface.h>        // the frame surface CDDSurface (m_10->m_2c: Flip + m_8)
#include <ddraw.h>                     // IDirectDrawSurface (the flip surface's raw +0x8 COM iface)
#include <Gruntz/SoundFxEmitter.h>     // CSoundFxEmitter (the screen-transition emitters)
#include <Gruntz/Fader.h>              // CFaderMgr / CFader / CFxModeT2/T3
#include <Bute/SymParser.h>            // CSymParser (CMgrPersistObj::m_rezLocator ResolvePath)
#include <Gruntz/SerialArchive.h>      // the ONE shared archive stream (Read@+0x2c / Write@+0x30)
#include <rva.h>
#include <Globals.h>

#include <stdio.h>  // sprintf (the "\SCREENZ\%s" path formatter)
#include <string.h> // inline /Oi strlen (repnz scasb) for the Vslot17 TextOutA
// DirectSoundMgr (IsPlaying/CloneAndPlay) + SoundDevice (PurgeVoiceList) now come from the
// real <Dsndmgr/SoundDevice.h>, pulled through <Gruntz/SoundCue.h> (via GameRegistry.h).

// The attract-cue registrar IS a CDDrawSubMgrLeafScan (header-less); local decl (exact arg types).
class DirNode;
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree/RemoveKeysEqual)

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
// Activate/LoadTitleConfig: mov ecx,ds:0x64556c; mov eax,[ecx+0x80]). Canonical DATA at
// 0x24556c (the CMgrPersistObj::Save m_world gate is the same object).
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

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

// AttractActor / AttractActorList (the per-frame g_actorList view) now live in the
// shared <Gruntz/AttractActor.h> (folded out of this TU; also used by CDemo/CHelpState).

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

// The random-pick target string (DAT_0060b5bc) and the shared empty string (0x6293f4)
// the ATTRACT_TITLE key is built from; the sound-enabled gate; the cached timeGetTime /
// wsprintfA import fn-ptrs the title roll reaches through. All reloc-masked.
DATA(0x0020b5bc)
extern char s_dat60b5bc[];
DATA(0x002293f4)
extern char g_emptyString[];
DATA(0x0021ab20)
extern i32 g_sndEnabled;
DATA(0x002c4650)
extern u32(WINAPI* g_pTimeGetTime)();
DATA(0x002c44c0)
extern int(WINAPI* g_pwsprintfA)(char* buf, const char* fmt, ...);
#define s_ATTRACT_TITLE_s "ATTRACT_TITLE%s"

// ===========================================================================
// The CAttract 0x13fb0 core band.
// ===========================================================================

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

    owner()->RestoreVideoMode(0);

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

// CAttract::ReleaseResources() (slot 2 / +0x8, 0x0140d0): free the registrar's
// pooled resource (if any), release the attract page ("ATTRACT"/"_"), then chain
// the base CState resource teardown. The menu root (m_c) is re-read for the
// Release access (retail does not cache it).
RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    CAttractRegistrar* reg = menuRoot()->m_28;
    if (reg->m_2c) {
        ((SoundStream*)reg->m_2c)->Stop();
    }
    ((CDDrawSubMgrLeafScan*)menuRoot()->m_28)->RemoveKeysEqual_157c70(s_ATTRACT, s_UNDERSCORE);
    CState::ReleaseResources();
}

// CAttract::Vslot09(arg) (slot 9 / +0x24, 0x014120): the full attract title-screen
// entry (/GX EH frame from the CString format local). Hide the cursor, roll a random
// TITLE%d and run it (as the siblings do), advance the active menu page (Method_158c70),
// then - via the inline MS-CRT LCG (== Rng::Next, seeded through the cached timeGetTime
// fn-ptr) - build a random "ATTRACT_TITLE%s" key, look it up in the registrar's
// CMapStringToOb (m_28+0x10) to (re)acquire the host/sound sub-object (m_host), (re)play
// its voice + latch the idle timeout (or a 0x1f40 default), then poke each g_actorList
// actor's slot-5 virtual. Returns 1. Re-homed from src/Stub/GapFunctions.cpp.
// @early-stop
// 98.3%: the whole 425B body - the /GX frame, the ShowCursor roll, the TITLE%d format +
// RunTitleSeq, the page fade, the inline MS-CRT LCG + %2 pick, the wsprintfA, the
// CMapStringToOb::Lookup host (re)acquire, the sound/idle branch and the actor slot-5
// loop - is byte-faithful. The only residue is a pair of register-selection coin-flips:
// retail seats m_c in eax across the Lookup argument setup where cl uses ecx, and loads
// the actor's vptr through eax where cl uses edx. A pure regalloc choice (operand-order /
// spelling variations do not flip it; docs/patterns/zero-register-pinning.md family).
// Deferred to the final sweep.
RVA(0x00014120, 0x1a9)
i32 CAttract::Vslot09(i32 arg) {
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(0) >= 0) {
        do {
        } while (showCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format(s_TITLE_d, idx);
    RunTitleSeq(s, 0, 0, 1, 0);
    CDDrawSubMgrPages* page = (CDDrawSubMgrPages*)menuRoot()->m_04;
    page->Method_158c70(page->m_backPair);

    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = g_pTimeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    i32 r = (g_randSeed >> 0x10) & 0x7fff;
    const char* pick = (r % 2) ? s_dat60b5bc : g_emptyString;

    char buf[0x40];
    g_pwsprintfA(buf, s_ATTRACT_TITLE_s, pick);

    CMapStringToOb* map = (CMapStringToOb*)((char*)menuRoot()->m_28 + 0x10);
    CObject* found = 0;
    map->Lookup(buf, found);
    m_host = (CAttractHost*)found;
    if (found != 0 && m_activeFlag != 0) {
        if (g_sndEnabled) {
            ((DirectSoundMgr*)m_host->m_10)->ApplyAndPlay(0x64, 0, 0, 0);
        }
        m_idleTimer = ((DirectSoundMgr*)m_host->m_10)->m_durationMs + 0x2710;
    } else {
        m_idleTimer = 0x1f40;
    }

    AttractActorList* list = g_actorList;
    for (i32 i = 0; i < list->m_count; i++) {
        list->m_data[i]->Vslot05();
    }
    return 1;
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
    if (!((DirectSoundMgr*)m_host->m_10)->IsPlaying()) {
        return 1;
    }
    ((DirectSoundMgr*)m_host->m_10)->CloneAndPlay(0, 0x1f4, 1);
    if (!((DirectSoundMgr*)m_host->m_10)->IsPlaying()) {
        return 1;
    }
    do {
        CAttractPooledRes* r = menuRoot()->m_28->m_2c;
        if (r) {
            ((SoundDevice*)r)->PurgeVoiceList(-1);
        }
    } while (((DirectSoundMgr*)m_host->m_10)->IsPlaying());
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
        ((SoundDevice*)res)->PurgeVoiceList(-1);
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

// 0x3c220 (the attract-family per-frame idle poll) is CDemo's vtable slot 5 -
// re-homed to Demo.cpp as CDemo::Render (proven: the .rdata data-ref at
// ??_7CDemo@@6B@+0x14 + the m_520 idle-timer match). The former CAttractIdlePoll
// placeholder view is gone.

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

// CAttract::Vslot0c (slot 12 / +0x30, 0x14720): keydown handler - on ESC/SPACE/ENTER
// post the exit WM_COMMAND (0x8023) to the top-level HWND. (Re-homed from ApiCallers
// CmdHost_014720.)
RVA(0x00014720, 0x37)
i32 CAttract::Vslot0c(i32 code, i32 unused) {
    if (code == 0x20 || code == 0xd || code == 0x1b) {
        g_pPostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

// CAttract::Vslot0e(a, b, c) (slot 14 / +0x38, 0x14770): post the exit WM_COMMAND
// (0x8023) to the top-level HWND (m_4->m_gameWnd->m_hwnd) unconditionally, then return 1.
RVA(0x00014770, 0x24)
i32 CAttract::Vslot0e(i32, i32, i32) {
    g_pPostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    return 1;
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

// CAttract::Update (0x0008cd40) is now an inline member in the header.

// CAttract::RefreshTitle - re-prime the attract title scene. Resets the scene
// slot off m_4->m_48 (PrimeScene then RestoreScene), re-resolves the
// "STATEZ_ATTRACT" state into m_2c, runs the title sequence with the bare "TITLE"
// tag, and returns 1.
RVA(0x00039160, 0x46)
i32 CAttract::RefreshTitle(i32 unused) {
    ((CGruntzSoundZ*)video()->m_48)->IsPlaying();
    ((CGruntzSoundZ*)video()->m_48)->StopAndFlush();
    m_2c = (CResSource*)stateMgr()->LookupState(s_STATEZ_ATTRACT);
    RunTitleSeq(s_TITLE, 0, 0, 1, 0);
    return 1;
}

// CAttract::~CAttract() (`??1`, 0x08cd90): the EH-framed destructor. MSVC emits
// the CAttract-vtable restore + slot-2 release (ReleaseResources, statically
// bound) + CState-vtable restore + base cleanup; the body just runs the release.
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
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
        ((CDDSurface*)tgt)
            ->ShadeRect(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), (tagRECT*)0);
        menuRoot()->m_04->TransTitle();
    } else {
        menuRoot()->m_04->TransEnter();
        CMenuBrightnessTarget* tgt = menuRoot()->m_04->m_18->m_2c;
        ((CDDSurface*)tgt)
            ->ShadeRect(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), (tagRECT*)0);
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
    ((CDDSurface*)tgt)
        ->ShadeRect(g_attractButeMgr.GetIntDef(s_Menu, s_BrightnessPercent, 0x32), (tagRECT*)0);
    menuRoot()->m_04->TransTitle();

    BuildMenuPage(0x50, 0x3e8, 0, 1);
    ShowCursorFn showCursor = g_ShowCursor;
    if (showCursor(1) < 0) {
        do {
        } while (showCursor(1) < 0);
    }
    return 1;
}

// ===========================================================================
// The state-services interval [0x0fa1f0 .. 0x0fb328].
// ===========================================================================

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

// ---------------------------------------------------------------------------
// CSoundFxEmitter - five sibling sound-effect/screen-transition emitters
// (0xfa410, 0xfa550, 0xfa790, 0xfa8f0, 0xfaa60). See CSoundFxEmitter.h for the
// recovered class/chain layout. Each: gate on the resource chain, fill a
// CFxModeT2/T3 transition descriptor on the stack, register it with the CFaderMgr,
// then - per g_fxDirectGate - apply the channel op now or defer it through the new
// fader, and finally Remove the fader. All callees are reloc-masked externs.
// ---------------------------------------------------------------------------

// 0xfa410: single-channel type-2 emitter (4 args).
RVA(0x000fa410, 0xf5)
i32 CSoundFxEmitter::Method_fa410(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_worker->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_10 = 1;
    t.m_18 = a1;
    t.m_1c = a2;
    t.m_04 = (i32)chan;
    t.m_08 = 0;
    CFader* f = mgr->Add(1, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        ActiveWait(a3);
        m_resChain->m_worker->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(a3, a4, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// 0xfa550: two-channel type-2 emitter (4 args). Blt channel A onto channel B.
// @early-stop
// 98.7% - logic byte-faithful. Residual is store/arg scheduling: cl hoists the
// m_18=a1 temp store before the Add-arg push where retail hoists m_1c=a2, and the
// deferred winapi_17e620 branch picks ecx/edx vs retail's eax/ecx for the two arg
// temporaries. Both are /O2 instruction-scheduling choices, not source-steerable.
RVA(0x000fa550, 0x10c)
i32 CSoundFxEmitter::Method_fa550(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_worker->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_1c = a2;
    t.m_10 = 0;
    t.m_18 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(1, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        ActiveWait(a3);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a3, a4, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// ---------------------------------------------------------------------------
// CState::Vslot17 (0x0fa6b0, vtable slot 23 / +0x5c, inherited by every state):
// the frame-surface GDI text overlay - draw `str` at (x,y) onto the frame
// surface via GDI (GetDC on the IDirectDrawSurface, SetBkMode/SetTextColor,
// TextOutA(x,y,str), ReleaseDC). Bails (0) on a null string, a missing frame
// surface, or a failed GetDC; returns 1 on success. Inline /Oi strlen (repnz
// scasb) feeds TextOutA's length. All states inherit this one body.
// ---------------------------------------------------------------------------
RVA(0x000fa6b0, 0xa7)
i32 CState::Vslot17(i32 x, i32 y, char* str, i32 color, i32 bkMode) {
    if (str == 0) {
        return 0;
    }
    CDDSurface* s = m_c->m_drawTarget->m_10->m_2c;
    if (s == 0) {
        return 0;
    }
    HDC hdc = 0;
    s->m_8->GetDC(&hdc);
    if (hdc == 0) {
        return 0;
    }
    SetBkMode(hdc, bkMode);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, str, strlen(str));
    s->m_8->ReleaseDC(hdc);
    return 1;
}

// 0xfa790: two-channel type-3 emitter (3 args).
// @early-stop
// 99.1% - logic byte-faithful. Residual is the chanA/chanB esi<->edi regalloc
// swap (docs/patterns/zero-register-pinning.md family): retail gives the
// longer-lived cached channel (chanB) the preferred callee-saved esi and the
// re-derived channel (chanA) edi, while cl's greedy allocator assigns them the
// other way round; identical structure, a few push/mov reg bytes differ. Not
// source-steerable (computation order is pinned by the chain walk).
RVA(0x000fa790, 0x104)
i32 CSoundFxEmitter::Method_fa790(i32 a1, i32 a2, i32 a3) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_worker->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 0;
    t.m_10 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a2, a3, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// 0xfa8f0: two-channel type-3 emitter (4 args); channel B chosen via a4 +
// CDDrawWorkerMgr::Method_158d20. No bank-stop bracketing on this variant.
// @early-stop
// 98.4% - logic byte-faithful. Same chanA/chanB esi<->edi regalloc swap as
// 0xfa790 plus the deferred-branch arg-temp register choice (see those notes);
// /O2 scheduling/regalloc, not source-steerable.
RVA(0x000fa8f0, 0x118)
i32 CSoundFxEmitter::Method_fa8f0(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDrawSurfacePair* holderB;
    if (a4 != 0 && m_resChain->m_worker->Method_158d20() != 0) {
        holderB = m_resChain->m_worker->m_overlayPair;
    } else {
        holderB = m_resChain->m_worker->m_backPair;
    }
    CDDSurface* chanB = holderB->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 0;
    t.m_10 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    if (g_fxDirectGate != 0) {
        ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a2, a3, 0);
    }
    mgr->Remove(f);
    return 1;
}

// 0xfaa60: single-channel type-3 emitter (3 args).
RVA(0x000faa60, 0xed)
i32 CSoundFxEmitter::Method_faa60(i32 a1, i32 a2, i32 a3) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_worker->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 1;
    t.m_10 = a1;
    t.m_04 = (i32)chan;
    t.m_08 = 0;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(a2, a3, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// (CPreviewState::LoadScreen @0x0fab90 belongs here; parked in LevelPreview.cpp
// until CPreviewState is homed into a shared header - see the file comment.)

// ---------------------------------------------------------------------------
// CState::Vslot07 (slot 7 / +0x1c, 0x0fac70): the base-state top-window paint poll.
// RECOVERED IDENTITY (was the StatePaintHost placeholder homed at CAttract): its
// .rdata data-ref is ??_7CState@@6B@+0x1c, and CCreditsState/CSplashState/CHelpState/
// CDemo/CMulti/CPlay all inherit this slot; the callers CAttract::Vslot07 (0x147b0),
// CMultiBootyState::ReadyAndPaint (0x1ce30) and CGuardedDispatch::Run (0x1f870) each
// call CState::Vslot07() on their own `this`. If the owner's game window is present, it
// runs a null Begin/EndPaint cycle and returns 1. Declared in <Gruntz/State.h>; its
// out-of-line COMDAT lands here (retail sited it inside the state-services TU).
// ---------------------------------------------------------------------------
RVA(0x000fac70, 0x4c)
i32 CState::Vslot07() {
    if (!m_4) {
        return 0;
    }
    if (!m_4->m_gameWnd) {
        return 0;
    }
    PAINTSTRUCT ps;
    BeginPaint(m_4->m_gameWnd->m_hwnd, &ps);
    EndPaint(m_4->m_gameWnd->m_hwnd, &ps);
    return 1;
}

// ===========================================================================
// CMgrPersistObj - the persisted game-mgr object (ex MgrObjSerialize.cpp +
// SaveRecordLoad.cpp; the two files carried DUPLICATE views of this one object
// - identical +0x0c..+0x1b0 layouts - now one struct). Its Save/Load stream the
// fields through the shared WAP32 CSerialArchive slots (Read @+0x2c / Write
// @+0x30); Init drives the "loading imagez" splash + GAME_IMAGEZ load. Offsets
// + code bytes are load-bearing; field/class names are best-guess placeholders.
// ===========================================================================

DATA(0x0024e35c)
extern i32 g_64e35c; // 0x64e35c "splash drawn" latch

// The image-set the object loads into (m_levelData->m_imageSet). This is a FOREIGN
// engine class: its ??_7 and slots 0..18 are unreconstructed engine code, so the
// honest model is the ONE dispatched slot - Load at vtable slot 19 (+0x4c),
// declared-only. obj->Load() -> call [eax+0x4c].
struct CMgrImageSet {
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Slot18();
    virtual i32 Load(char* path, const char* a, const char* b); // slot 19 (+0x4c)
};

// The level-data object (m_levelData) and the renderer it owns.
struct CLevelData {
    char pad00[4];
    CDDrawSubMgrPages* m_ready; // +0x04 (the Method_158bc0 readiness gate)
    char pad08[0x10 - 0x08];
    CMgrImageSet* m_imageSet; // +0x10
};

// The display owner (m_displayMgr): its m_8c/m_90 are blitted into the splash params.
struct CDisplayMgr {
    char pad00[0x8c];
    i32 m_8c; // +0x8c
    i32 m_90; // +0x90
};

// The on-screen splash params block built on the stack for EngStr_DrawText; its
// leading slot is the loaded caption CString.
struct SplashParams {
    CString text; // +0x00
    i32 m_04;     // +0x04
    i32 m_08, m_0c, m_10, m_14;
};
void EngStr_DrawText(
    CLevelData* lvl,
    SplashParams* a,
    i32* b,
    i32 size,
    i32 e,
    i32 f,
    i32 g,
    i32 h,
    i32 i
); // 0x115440

// The persisted object. Only the serialized fields are named. NB the Save/Load
// method names are recovered-symbol placeholders; the archive object drives the
// actual transfer direction, only the +0x2c/+0x30 slot offsets are load-bearing.
struct CMgrPersistObj {
    i32 m_00;                  // +0x00
    CDisplayMgr* m_displayMgr; // +0x04
    CSymParser* m_rezLocator;  // +0x08
    CLevelData* m_levelData;   // +0x0c
    char m_pad10[0x1c - 0x10];
    i32 m_1c, m_20, m_24;
    char m_pad28[0x38 - 0x28];
    i32 m_38, m_3c, m_40, m_44, m_48;
    char m_4c[0x100]; // 0x4c..0x14c
    i32 m_14c, m_150, m_154, m_158, m_15c;
    char m_pad160[0x168 - 0x160];
    char m_168[0x10];
    char m_178[0x10];
    char m_188[0x10];
    char m_198[0x10];
    i32 m_1a8, m_1ac, m_1b0;

    i32 Save(CSerialArchive* w); // 0x0fb1c0 (+0x2c slot pass)
    i32 Load(CSerialArchive* s); // 0x0faff0 (+0x30 slot pass; ex SaveRecord::Load)
    i32 Init();                  // 0x0face0
};

// @early-stop
// /GX frame-packing artifact (~96%): the instruction stream is byte-faithful, but
// retail reserves `sub esp,0x14` and builds the splash block's tail two dwords in
// the transient arg-push area, where this cl reserves the whole block (`sub esp,
// 0x1c`), shifting every esp-relative displacement by 8; plus the EH scope-table
// symbol is named/represented differently by the delinker.  Logic is complete.
// CMgrPersistObj::Init: hide the cursor, gate on the level being ready,
// draw the "loading imagez" splash (once), resolve the GAME_IMAGEZ rez and load it
// into the image-set, then mark the object started.
RVA(0x000face0, 0x17c)
i32 CMgrPersistObj::Init() {
    if (m_levelData == 0) {
        return 0;
    }
    ShowCursorFn sc = g_ShowCursor;
    while (sc(0) >= 0)
        ;
    if (m_levelData->m_ready->Method_158bc0() == 0) {
        return 0;
    }
    if (g_64e35c == 0) {
        SplashParams sp;
        sp.text.LoadString(0x81a9);
        sp.m_08 = m_displayMgr->m_8c;
        sp.m_0c = m_displayMgr->m_90;
        sp.m_10 = 0;
        sp.m_14 = 0;
        EngStr_DrawText(m_levelData, &sp, &sp.m_04, 0x78, 1, 0xff, 0, 0xff, 1);
    }
    while (sc(0) >= 0)
        ;
    g_64e35c = 0;
    char* path = (char*)m_rezLocator->ResolvePath("GAME_IMAGEZ");
    if (path == 0) {
        return 0;
    }
    if (m_levelData->m_imageSet->Load(path, "GAME", "_") == -1) {
        return 0;
    }
    m_1a8 = 0;
    m_1ac = 1;
    m_1b0 = 0;
    return 1;
}

// -------------------------------------------------------------------------
// 0x0faec0 (spatially re-homed from src/Stub/ReconBatch2.cpp). Per-frame
// present/refresh of the bound view: if the suppress latch is set, clear it and
// return; else Refresh the back pair, ShadeRect the back surface with arg0, Flip
// the front surface, then Refresh again. @orphan: reached from CGruntzMgr::
// RunModalDialog/ExitModalUI on an unidentified modal-UI presenter object.
// (g_suppress_64e360 - the present-suppress latch - comes from <Globals.h>.)
struct Mid_faec0 {
    char m_pad0[0x4];
    CDDrawSubMgrPages* m_4; // +0x04
};
struct PresentHost_faec0 { // owner unidentified (only inbound edge: ILT thunk 0x1ec9)
    char m_pad0[0xc];
    Mid_faec0* m_c; // +0x0c
    void Present(i32 arg0);
};
RVA(0x000faec0, 0x67)
void PresentHost_faec0::Present(i32 arg0) {
    if (g_suppress_64e360 != 0) {
        g_suppress_64e360 = 0;
        return;
    }
    m_c->m_4->Method_158c70(m_c->m_4->m_backPair);
    m_c->m_4->m_backPair->m_surface->ShadeRect(arg0, (RECT*)0);
    m_c->m_4->m_frontPair->m_surface->Flip((CDDSurface*)0);
    m_c->m_4->Method_158c70(m_c->m_4->m_backPair);
}

// CState::ShadeScreen (0x0faf50): consume the g_suppress latch (return its old value)
// on the frame it is armed; otherwise shade the whole draw surface `pct` percent.
RVA(0x000faf50, 0x31)
i32 CState::ShadeScreen(i32 pct) {
    i32 v = g_suppress_64e360;
    if (v != 0) {
        g_suppress_64e360 = 0;
        return v;
    }
    return m_c->m_drawTarget->m_14->m_2c->ShadeRect(pct, 0);
}

// -------------------------------------------------------------------------
// 0x0fafa0 (spatially re-homed from src/Stub/ReconBatch2.cpp). __stdcall(4)
// validity gate: returns 0 if arg0 is null; for kind 4 / 7 validates arg0 through
// a per-kind checker (return 0 on fail); otherwise (and on success) returns 1.
i32 __stdcall Check4_2ce8(i32 h); // 0x0faff0 (kind 4)
i32 __stdcall Check7_36bb(i32 h); // 0x0fb1c0 (kind 7)
// @early-stop
// regalloc wall (topic:wall topic:regalloc): the switch body (cmp 4 je / cmp 7
// jne / kind7 inline / kind4 trailing) is byte-identical to retail; residual is
// a0 landing in ecx (cl) vs eax (retail) - so the a0==0 return needs an extra
// xor eax, and the push/cmp register encodings shift. Not source-steerable. ~93.3%.
RVA(0x000fafa0, 0x3b)
i32 __stdcall Validate_fafa0(i32 a0, i32 kind, i32 a2, i32 a3) {
    if (a0 == 0) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (Check4_2ce8(a0) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Check7_36bb(a0) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// CMgrPersistObj::Load (0x0faff0, ex SaveRecord::Load - the second view of this
// object, now folded): gate on the archive + the +0x0c level-data slot, then
// stream the same persisted fields through the archive's +0x30 slot (the m_1b0
// tail word is NOT part of this pass).
RVA(0x000faff0, 0x163)
i32 CMgrPersistObj::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!m_levelData) {
        return 0;
    }
    s->Write(&m_1c, 4);
    s->Write(&m_20, 4);
    s->Write(&m_24, 4);
    s->Write(&m_38, 4);
    s->Write(&m_3c, 4);
    s->Write(&m_40, 4);
    s->Write(&m_44, 4);
    s->Write(&m_48, 4);
    s->Write(m_4c, 0x100);
    s->Write(&m_14c, 4);
    s->Write(&m_150, 4);
    s->Write(&m_154, 4);
    s->Write(&m_158, 4);
    s->Write(&m_15c, 4);
    s->Write(m_168, 0x10);
    s->Write(m_178, 0x10);
    s->Write(m_188, 0x10);
    s->Write(m_198, 0x10);
    s->Write(&m_1a8, 4);
    s->Write(&m_1ac, 4);
    return 1;
}

// CMgrPersistObj::Save: gate on the writer + the settings singleton, then
// stream every persisted field through the writer's Read(buf,len) virtual.
RVA(0x000fb1c0, 0x168)
i32 CMgrPersistObj::Save(CSerialArchive* w) {
    if (w == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    w->Read(&m_1c, 4);
    w->Read(&m_20, 4);
    w->Read(&m_24, 4);
    w->Read(&m_38, 4);
    w->Read(&m_3c, 4);
    w->Read(&m_40, 4);
    w->Read(&m_44, 4);
    w->Read(&m_48, 4);
    w->Read(m_4c, 0x100);
    w->Read(&m_14c, 4);
    w->Read(&m_150, 4);
    w->Read(&m_154, 4);
    w->Read(&m_158, 4);
    w->Read(&m_15c, 4);
    w->Read(m_168, 0x10);
    w->Read(m_178, 0x10);
    w->Read(m_188, 0x10);
    w->Read(m_198, 0x10);
    w->Read(&m_1a8, 4);
    w->Read(&m_1ac, 4);
    w->Read(&m_1b0, 4);
    return 1;
}

SIZE_UNKNOWN(AttractWndHolder);
SIZE_UNKNOWN(CAttract);
SIZE_UNKNOWN(CAttractHost);
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
SIZE_UNKNOWN(CSoundFxEmitter);
SIZE_UNKNOWN(CDDrawSurfacePair);
SIZE_UNKNOWN(FxResource);
SIZE_UNKNOWN(CDisplayMgr);
SIZE_UNKNOWN(CMgrImageSet);
SIZE_UNKNOWN(CLevelData);
SIZE_UNKNOWN(CMgrPersistObj);
SIZE_UNKNOWN(CRezLocator);
SIZE_UNKNOWN(SplashParams);
SIZE_UNKNOWN(Mid_faec0);
SIZE_UNKNOWN(PresentHost_faec0);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// AttractState.cpp - the CAttract state-machine CORE obj: retail .text run
// [0x013fb0 .. 0x014819] (the out-of-line CState vtable-slot overrides) plus the
// COMDAT-pooled EH destructor (~CAttract @0x08cd90). This is the class-identity
// TU for CAttract (RTTI .?AVCAttract@@, vtable @0x5ea194; a CState leaf): 10 of
// its 12 CState-slot overrides live in this one contiguous obj run, and the ??_G
// scalar-deleting dtor + the ??1 destructor anchor the vtable emission here.
//
// Carved out of the former conflated Attract.cpp (REHOME/holding-TU drain, D5):
// that file wove TWO separate original objs - this CAttract state core (0x13fb0
// band) and the attract state-services interval (0x0fa1f0 band, CAttract title/
// fade + CSoundFxEmitter + CState helpers + CMgrPersistObj serialize), which now
// stays in Attract.cpp. Owner proven by the CAttract vtable (sema class CAttract:
// slots 1,2,5,6,7,8,9,10,12,14 map to this band; slot 0 ??1 -> 0x08cd90).
//
// CAttract slots implemented here (order anchored by CState):
//   ~CAttract()          0x08cd90  slot 0  EH ??1 (vtable restore + base chain)
//   EnterAttractMode     0x013fb0  slot 1  (reached non-virtually; ret 0xc)
//   ReleaseResources()   0x0140d0  slot 2  resource release (Free + Release + base)
//   Vslot09(i32)         0x014120  slot 9  title-screen entry (/GX)
//   FrameSlot28(i32)     0x014340  slot 10 per-frame voice poll
//   Render()             0x0143e0  slot 5  per-frame poll/draw
//   InputVirtual()       0x014520  slot 8  random-title roll (page gate)
//   Vslot06()            0x014630  slot 6  random-title roll (Vfunc3 gate)
//   Vslot0c(i32,i32)     0x014720  slot 12 keydown handler
//   Vslot0e(i32,i32,i32) 0x014770  slot 14 post-exit command
//   Vslot07()            0x0147b0  slot 7  host/paint poll
// Field names are placeholders; only OFFSETS + code bytes matter.
#include <Gruntz/String.h> // MFC CString (Vslot09's CMapStringToOb/CObject); MFC-first
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Attract.h>
#include <Bute/SymParser.h> // CSymParser (m_8: ResolvePath 0x13c030) + CSymTab (m_2c: FindSub 0x13a230)
#include <Gruntz/GameRegistry.h> // CGameRegistry / g_gameReg (+ SoundCue chain: DirectSoundMgr/SoundDevice/SoundStream)
#include <Gruntz/AttractActor.h>       // the shared per-frame g_actorList view
#include <Gruntz/ResMgr.h>             // CDrawTarget (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (Vslot09 Method_158c70)
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair/m_frontPair->m_surface)
#include <DDrawMgr/DDSurface.h>        // CDDSurface (Vslot07 Flip; m_10->m_2c)
#include <ddraw.h>                     // IDirectDrawSurface (Render busy IsLost)
#include <rva.h>
#include <Globals.h> // Vslot09: g_randSeeded / g_randSeed

// The attract-cue registrar IS a CDDrawSubMgrLeafScan (header-less); local decl.
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ScanTree/RemoveKeysEqual)

// ---------------------------------------------------------------------------
// External engine globals (reloc-masked DATA symbols).
// ---------------------------------------------------------------------------

// The game registry singleton (canonical CGameRegistry): its +0x80 launch counter
// (m_numRuns) selects the TITLE state. Canonical DATA at 0x24556c.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The attract-state count divisor (DAT_00645534, a writable global int). extern "C"
// so it emits the canonical `_g_645534` - the single name bound at 0x245534 (home
// `multi`); the ex C++-mangled g_attractStateCount lost the per-rva keep-last dedup.
extern "C" i32 g_attractStateCount;

// ShowCursor is the real USER32 import (<Mfc.h>); its IAT slot @0x6c44c4.

// PostMessageA reached through the IAT slot (matches the engine's ff15 indirect).
// extern "C" so the reloc emits `_g_pPostMessageA` - the canonical name bound at
// 0x2c44c8 (home `sbi_rectonly`); the C++-mangled view was the dedup loser.

// The per-frame time delta (countdown source for m_idleTimer). C linkage so the
// symbol pairs with the target's _g_645584 (the convention across the gamemode units).
extern "C" {
    DATA(0x00245584)
    extern u32 g_frameDelta;
}

// The random-pick target string (DAT_0060b5bc) and the shared empty string
// (0x6293f4) the ATTRACT_TITLE key is built from; the sound-enabled gate; the cached
// timeGetTime / wsprintfA import fn-ptrs the title roll reaches through. All reloc-masked.
DATA(0x0020b5bc)
char s_dat60b5bc[] = "2";
DATA(0x002293f4)
extern char g_emptyString[];
DATA(0x0021ab20)
extern i32 g_sndEnabled;

// Source string literals (objdiff matches these .data relocations by value).
#define s_STATEZ_ATTRACT "STATEZ_ATTRACT"
#define s_TITLE_d "TITLE%d"
#define s_TITLE "TITLE"
#define s_SOUNDZ "SOUNDZ"
#define s_ATTRACT "ATTRACT"
#define s_UNDERSCORE "_"
#define s_ATTRACT_TITLE_s "ATTRACT_TITLE%s"

// ===========================================================================
// The CAttract 0x13fb0 core band.
// ===========================================================================

// CAttract::EnterAttractMode - enter (or re-enter) the attract scene.
// Gates on CState::LoadGameAssetNamespaces(a, b, mode); on failure returns that result.
// Otherwise hides the cursor, re-asserts the video mode, resolves the
// "STATEZ_ATTRACT" state (stored into m_2c), loads its "SOUNDZ" set, registers
// the sound handle on the menu page under the "ATTRACT"/"_" tags, hides the
// cursor again, then sets the entry flags: m_host is always cleared, m_activeFlag is
// cleared when mode == 3 (else set to 1). Returns 1 on success, 0 on early-out.
RVA(0x00013fb0, 0xd5)
i32 CAttract::EnterAttractMode(i32 a, i32 b, i32 mode) {
    if (LoadGameAssetNamespaces(a, b, mode) == 0) {
        return 0;
    }

    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg (the ex-g_ShowCursor fn-ptr global hand-modeled that exact idiom).
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    owner()->RestoreVideoMode(0);

    CSymTab* state = (CSymTab*)stateMgr()->ResolvePath(s_STATEZ_ATTRACT);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }

    void* sound = state->FindSub(s_SOUNDZ);
    if (sound == 0) {
        return 0;
    }

    ((CDDrawSubMgrLeafScan*)menuRoot()->m_28)
        ->ScanTree_157ee0((CSymTab*)sound, s_ATTRACT, s_UNDERSCORE);

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
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
    // The base teardown is CGameModeBase::BaseCleanup (0xfa150), not a distinct
    // CState::ReleaseResources body - same (CGameModeBase*)this bridge the CState dtor uses.
    ((CGameModeBase*)this)->BaseCleanup();
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
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg (the ex-g_ShowCursor fn-ptr global hand-modeled that exact idiom).
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
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
        seed = ::timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    i32 r = (g_randSeed >> 0x10) & 0x7fff;
    const char* pick = (r % 2) ? s_dat60b5bc : g_emptyString;

    char buf[0x40];
    ::wsprintfA(buf, s_ATTRACT_TITLE_s, pick);

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

    if (g_frameDelta < m_idleTimer) {
        m_idleTimer -= g_frameDelta;
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
            ::PostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            return 1;
        }
    }
    return 1;
}

// CAttract::InputVirtual (slot 8 / +0x20, 0x14520): the random-title roll gated on
// the menu page's IsLoaded; if loaded, hide the cursor, pick a random TITLE%d index
// off the game-reg attract counter, and run that title sequence. The CString format
// local forces the /GX EH frame. (Render polls this slot each frame.)
RVA(0x00014520, 0xc3)
i32 CAttract::InputVirtual() {
    // The page "loaded?" gate is CDDrawSubMgrPages::Method_158bc0 (0x158bc0), reached
    // through the page's real class (the CMenuPage view's IsLoaded @0x158bc0 == this).
    if (((CDDrawSubMgrPages*)menuRoot()->m_04)->Method_158bc0() == 0) {
        return 0;
    }
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg (the ex-g_ShowCursor fn-ptr global hand-modeled that exact idiom).
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
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
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg (the ex-g_ShowCursor fn-ptr global hand-modeled that exact idiom).
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
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
        ::PostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

// CAttract::Vslot0e(a, b, c) (slot 14 / +0x38, 0x14770): post the exit WM_COMMAND
// (0x8023) to the top-level HWND (m_4->m_gameWnd->m_hwnd) unconditionally, then return 1.
RVA(0x00014770, 0x24)
i32 CAttract::Vslot0e(i32, i32, i32) {
    ::PostMessageA(owner()->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
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
    // ShowCursor: real USER32 import (<Mfc.h>); called 2x/body -> cl caches the __imp__
    // slot in a reg (the ex-g_ShowCursor fn-ptr global hand-modeled that exact idiom).
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    menuRoot()->m_04->m_10->m_2c->Flip(0);
    menuRoot()->m_04->BlitFrom(menuRoot()->m_04->m_14);
    return 1;
}

// CAttract::Update (slot 4, 0x08cd40): the state-id probe. Defined HERE rather than
// inline in <Gruntz/Attract.h> because GruntzMgr.cpp also constructs CAttract; an RVA()
// on an inline header body gets emitted as a COMDAT in every such TU, both claim this
// rva, and merge_labels re-attributes the symbol away from this unit.
RVA(0x0008cd40, 0x6)
GameStateId CAttract::Update() {
    return GAMESTATE_ATTRACT;
}

// CAttract::~CAttract() (`??1`, 0x08cd90): the EH-framed destructor (COMDAT-pooled
// into the 0x08cxxx dtor band). MSVC emits the CAttract-vtable restore + slot-2
// release (ReleaseResources, statically bound) + CState-vtable restore + base
// cleanup; the body just runs the release. This TU is the CAttract vtable-emission
// anchor (??_7CAttract@@6B@ + the ??_G scalar-deleting dtor at 0x08cd60).
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}

SIZE(CAttract, 0x1c0); // retail operator-new size (TransitionState 0x8bacf)
SIZE_UNKNOWN(CAttractHost);
SIZE_UNKNOWN(CAttractPooledRes);
SIZE_UNKNOWN(CAttractRegistrar);
SIZE_UNKNOWN(CAttractVideo);
SIZE_UNKNOWN(CAttractVoice);
SIZE_UNKNOWN(CMenuRoot);
SIZE_UNKNOWN(CDDrawSurfacePair);

// MenuState.cpp - CMenuState, the front-end menu game-state (C:\Proj\Gruntz).
// Split out of the former GameMode.cpp god-TU (per-class TU cut): CMenuState now owns
// its full method set here; the CState base implementation stays in
// GameMode.cpp, the sibling states (CCreditsState/CBootyState/CMultiBootyState) in
// their own TUs. The ~CMenuState `??1` (with the CState ctor) is the class's vtable +
// inline-virtual (Update) emission anchor - it stays in this TU with the rest of
// CMenuState.
//
// This obj's ordinary .text contribution is the CONTIGUOUS run 0x9fe50..0xa0e57.
// The former MenuStateAssets.cpp was an artificial split of it (0x9fe50 sits at the
// run's head) and has been dissolved back in, along with CChatBox::Init (0xa0280) -
// which is bracketed on both sides by CMenuState methods inside this very run, so
// retail's menu compiland is what contributes that byte (every other CChatBox
// method lives in the chat box's own block 0xe2000 away at 0x182ab0..0x183246).
//
// Functions, ascending retail-RVA order:
//   CMenuState::~CMenuState   @0x08ce60 - COMDAT-pooled dtor (outside the run).
//   CMenuState::LoadGameAssetNamespaces @0x09fe50 - the MENU asset loader.
//   CChatBox::Init            @0x0a0280 - the menu UI object's field-zeroing init.
//   ReleaseResources / StartMusic / StopMusicChain / FrameSlot28 /
//   Render / Vslot0c / Vslot0e / Vslot10  @0x0a02c0.. - teardown + per-frame draw.
//   CMenuState::ReadyGate     @0x0a0d40 - the &&-chained ready/transition probe.
//   CMenuState::BuildVersionString @0x0a0d80 - the on-screen version banner.
//
// CMenuState : CState (RTTI .?AVCMenuState@@); the class body lives in
// <Gruntz/GameMode.h>. Only offsets / control IDs / code bytes are load-bearing;
// names are placeholders for the recovered engine identities.
#include <Gruntz/GameMode.h>
#include <Gruntz/MenuVersion.h> // g_versionMajor/Mid/Minor (owner-only decl header)
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LeafCue.h>         // canonical LeafCue (CMenuState::m_1bc menu-music cue)
#include <Gruntz/BattlezData.h>     // the REAL stats object (was the CHudStats view)
#include <Gruntz/GruntzMgr.h>       // CGruntzMgr (the game-manager singleton; one true shape)
#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst (StartMusic/StopMusicChain ConfigureItem)
#include <Gruntz/WwdGameReg.h>      // g_gameReg (StartMusic music gate)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ReleaseResources leaf keys)
#include <DDrawMgr/DDrawSubMgrPages.h>    // CDDrawSubMgrPages (FrameSlot28 flush)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (was a GameMode.cpp local view)
#include <DDrawMgr/DDSurface.h>           // CDDSurface Flip (FrameSlot28)

#include <rva.h>
#include <Bute/SymParser.h> // canonical CSymParser + CSymTab (LoadGameAssetNamespaces ResolvePath)
#include <Image/CImage.h>   // g_resourceInstallActive
#include <Gruntz/ChatBox.h> // canonical CChatBox (m_1b4 menu UI object; Init lives here)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CImageRegistry (m_c->m_imageRegistry)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <DDrawMgr/DDrawWorkerList.h>  // renderer B - the real CDDrawWorkerList (ClearWorkers)
#include <Win32.h>                     // IsDlgButtonChecked + HWND (real USER32 header)
#include <Globals.h>

// ---------------------------------------------------------------------------
// The game-manager singleton (*g_gameReg) - the one true CGruntzMgr shape lives in
// <Gruntz/GruntzMgr.h>. (The options-dialog reader/writer family that also taps it
// is homed in src/Gruntz/VideoConfig.cpp, the options-dialogs TU.)
extern "C" {
}

// The Render menu-entity list (aliases g_actorList @0x245574) and the version-string
// RECT source (aliases g_versionRectL @0x245cc8). Declared in GameMode.h; the DATA
// binding lives here (the .cpp) so Render's absolute loads reloc-bind to the real RVAs.
// DEFINED here (with storage), not merely declared: 0x645574 had NO definition anywhere
// under EITHER of its two former names (?g_actorList@@3PAUAttractActorList@@A from
// demo/helpstate/attractstate, _g_645574 from here/splashstate/creditsstate), so every
// reference to it was an unresolved external. One symbol, one definition; the shared decl
// is in <Gruntz/AttractActor.h>.
extern "C" {
    DATA(0x00245574)
    AttractActorList* g_actorList = 0;
}
DATA(0x00245cc8)
extern "C" {
    CGMVerRect g_versionRect; // .bss - zero at load
}

// ===========================================================================
// CMenuState per-frame + teardown methods (moved from the former GameMode.cpp
// god-TU). The ~CMenuState `??1` anchors the CMenuState vtable/inline-virtual
// emission in this TU.
// ===========================================================================

// CBootyState::FormatHudText (0x1af70) is NOT a CMenuState method and 0x1af70 is
// not in this TU's block - it is homed in its own class's TU,
// src/Gruntz/BootyStateActivate.cpp (inside that obj's 0x18c90..0x1f928 run).

// The options-dialog helper family (LoadGameOptionsToDialog @0x36860,
// ReadMenuOptionsDialog @0x36a30, the OnToggle* handlers @0x36d00.., and the
// ApiCallerStubs scroll helpers @0x36e50/0x36ec0/0x371e0) lives in its home TU
// per the interval dossier (#10c): src/Gruntz/VideoConfig.cpp (the options-
// dialogs TU). The real CMenuState core below stays here.

// ---------------------------------------------------------------------------
// CMenuState teardown + per-frame draw (moved from GameMode.cpp). The ~CMenuState
// `??1` anchors the CMenuState vtable/inline-virtual emission in this TU.
// ---------------------------------------------------------------------------

// The owning game-manager (CState::m_4, a real CGruntzMgr*) reached through the
// gamemode-local CGMOwner reduced view (same helper the sibling state TUs use).
static inline CGMOwner* Owner(CState* s) {
    return reinterpret_cast<CGMOwner*>(s->m_4);
}

// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// The renderer's ClearWorkers @0x163c60 comes from the real CDDrawWorkerList
// (<DDrawMgr/DDrawWorkerList.h> - the m_workerList type; the stale local decl-only
// shadow class is gone).

// The menu music controller (CMenuState+0x1bc) IS the canonical LeafCue
// (<Gruntz/LeafCue.h>) - the 0x1c leaf-scan sound-cue element: its DSoundCloneInst m_10
// player (IsPlaying 0x1353f0 / CloneAndPlay 0x135660 inherited from DirectSoundMgr,
// ConfigureItem 0x1360d0 its own) plus the draw-clock gate m_14 (last) / m_18 (interval).
// The former CMenuMusic view is dissolved onto it; DSoundCloneInst : DSoundBaseSub :
// DirectSoundMgr so m_10-> reaches every method cast-free.

// The draw-clock mirror + the reentrancy gate the menu music poll save/restores.
extern "C" u32 g_killCueClock; // draw-clock mirror

// StartMusic reads the game registry through its WwdGameReg view (m_10 presence gate,
// m_11c configured item); same 0x24556c singleton as g_gameReg, typed WwdGameReg.

// The COMDAT-pooled deleting dtor: 0x8ce60 is not in this TU's own 0x9fe50..
// 0xa0e57 run - the linker groups the ??1/??_G COMDATs into the 0x8xxxx pool,
// away from the obj's ordinary .text. Listed first so file order still ascends.
// CMenuState::~CMenuState() (`??1`, 0x8ce60): run the menu teardown then chain the base.
// ReleaseResources/the base ~CState are statically bound in the dtor.
RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

// -------------------------------------------------------------------------
// The MENU asset loader + the chat-box init it news, homed from the former
// MenuStateAssets.cpp: 0x9fe50 and 0xa0280 sit in this TU's own contiguous
// 0x9fe50..0xa0d80 .text run, ahead of ReleaseResources (0xa02c0) below.
// -------------------------------------------------------------------------
// FUN_00402fcc __cdecl: commit the menu UI object (ret BOOL).
i32 MenuCommit(CChatBox* obj, i32 idx); // 0x402fcc

// CMenuState::LoadGameAssetNamespaces (0x09fe50, 835 B; the
// slot-1 override, ex "LoadAssets"), the MENU game-
// state asset loader.  Sibling of the CHelpState / GameLevelState slot-1 loaders:
// chains the base namespace loader, registers the "MENU" IMAGEZ+SOUNDZ namespaces
// through the m_c->m_imageRegistry (vtable +0x48) / m_c->m_soundRegistry registries, primes the state
// core (m_c->m_4 IsReady/Init), then heap-allocates the menu HUD object (CPtrList +
// two CString members) and wires its MENU_CURSOR/SELECT/ACTIVATE/MENU keys + the
// MENU_ACTIVATE / MENU_MENU sound cues.  The destructible CPtrList/CString members
// of the heap object give the routine its /GX exception frame.
//
// Only offsets / code bytes are load-bearing; every engine callee is a reloc-
// masked external (no body).

// The image registry reached through this->m_c->m_imageRegistry is the canonical CImageRegistry
// (<Gruntz/ResMgr.h>): non-virtual Has + the vtable-slot-18 (+0x48) Install. Shared,
// so no local view.

// --- the sound-cue lookup ---
// The MENU_ACTIVATE / MENU_MENU cues are looked up in the LeafScan cache's embedded
// CMapStringToPtr (+0x10, Lookup @0x1b8438 - mfc_class-confirmed CMapStringToPtr, NOT
// CMapStringToOb).  The map VALUE is the canonical LeafCue (<Gruntz/LeafCue.h>, the
// 0x1c-byte element the LeafScan factory CreateEntry @0x157d70 news): its DSoundCloneInst
// m_10 player carries the cached DS-buffer duration at m_durationMs (+0x28). The map
// values are LeafCue / DSoundCloneInst (see the LeafCue.h header verdict).

// The CState base facets are the ONE real classes (State.h) - this TU no longer keeps
// per-TU views of any of them:
//   * m_4  (CGruntzMgr)            - RestoreVideoMode; its CGameMgr base carries
//                                    m_gameWnd (+0x04), whose m_hwnd (+0x04) is the
//                                    HWND the menu layout is seeded with.
//   * m_8  (CBankMgr)              - reached as CSymParser for ResolvePath.
//   * m_c  (CDDrawSurfaceMgr)  - the resource holder: m_drawTarget (+0x04) page
//                                    pump, m_10 image registry, m_28 sound registry.
//   * m_2c (CResSource)            - the registered STATEZ_MENU object.
//   * m_1b4 (CChatBox)             - the heap menu-UI object this routine builds.

// The global mgr singleton (*0x24556c): its resource holder's +0x28 sound registry
// carries the shared cue map the MENU_MENU cue is resolved from. That holder slot
// (CDDrawSurfaceMgr::m_28) is a genuinely heterogeneous void* - other TUs view it
// as a sound-set (HbSndSet) or a mute gate - so it is cast to the sound-registry view
// at this one use-site (the authentic proven-heterogeneous-slot cast).

// The heap-allocated MENU UI object (0x7c bytes) IS the canonical CChatBox
// (<Gruntz/ChatBox.h>): its CPtrList m_nodeList (+0x24), m_activeNode (+0x40) and the
// two CString row keys (+0x44/+0x48) are exactly the members this routine constructs
// and writes, and GameMode.h already types the receiving slot `CChatBox* m_1b4`.  The
// destructible CPtrList/CString members give LoadAssets its /GX EH frame.  The former
// MenuHudObj view is gone; its own comments already named every method as CChatBox's
// (Init 0xa0280 via thunk 0x10c8, AdvanceRow0 0x182df0, AdvanceRow1 0x182e60).

// FUN_00402fcc __cdecl: commit the menu UI object (ret BOOL).
i32 MenuCommit(CChatBox* obj, i32 idx); // 0x402fcc

// The menu-region seeder's `this` record (0x182ab0) IS the CChatBox LoadAssets just
// newed - now dissolved onto CChatBox::InitRegion (<Gruntz/ChatBox.h>). The three
// canonical changes the old view was blocked on are all resolved in ChatBox.h now
// (m_page is CDDrawSurfaceMgr*, +0x08 is a real RECT m_rect8, m_wrapFlag is i32).

// CMenuState is the canonical <Gruntz/GameMode.h> `CMenuState : CState`. The MENU
// asset loader reaches the CState base region through the SAME facets the game-state
// hierarchy documents (CState.h: the +0x04 owner and +0x0c CDDrawSurfaceMgr holder are downcast
// to each TU's local facet views): m_4 (CGruntzMgr owner) -> MenuRoot cursor gate,
// m_8 (CBankMgr) -> MenuRegSet, m_c (CDDrawSurfaceMgr) -> MenuAssetMgr resource holder, m_2c
// (CResSource) -> the STATEZ_MENU MenuRegObj. m_1b4 (CGMMenuUI) is the heap MenuHudObj
// the routine builds. Only offsets / code bytes are load-bearing.

// @early-stop
// frame-layout / regalloc wall: complete + correct body - instruction
// selection, the guarded registry-call chain, the heap CChatBox CPtrList+2 CString
// construction + EH trylevel, the MENU_CURSOR/SELECT/ACTIVATE keys and the two sound
// finds all match retail (verified instruction-by-instruction; the ARG_MISMATCH
// rows are the reloc-name scoring artifact).  Residual: retail frame-allocates 0x10
// of locals while this /O2 recompile allocates 0x14 (one extra slot), yielding a +4
// cascade across every [esp+N] operand, plus the base-loader arg push scheduling and
// the new-obj-vs-EH-state interleave - all allocator choices, not source-steerable.
// See docs/patterns/zero-register-pinning.md + identical-return-epilogue-tailmerge.md.
RVA(0x0009fe50, 0x343)
i32 CMenuState::LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) {
    if (a3 == 0) {
        return 0;
    }
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (!CState::LoadGameAssetNamespaces(a2, a3, a3)) {
        return 0;
    }
    m_4->RestoreVideoMode(0);
    m_2c = static_cast<CResSource*>(m_8->ResolvePath("STATEZ_MENU"));
    if (m_2c == 0) {
        return 0;
    }

    if (!m_c->m_imageRegistry->HasKeyEqual_155550("MENU")) {
        void* set = SymTab2c()->ResolvePath("IMAGEZ");
        if (set == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_c->m_imageRegistry->InstallTree(set, "MENU", "_");
        g_resourceInstallActive = 0;
    }

    if (!m_c->m_soundRegistry->HasKeyEqual_1583c0("MENU")) {
        void* set = SymTab2c()->ResolvePath("SOUNDZ");
        if (set == 0) {
            return 0;
        }
        m_c->m_soundRegistry->ScanTree_157ee0(static_cast<CSymTab*>(set), "MENU", "_");
    }

    if (!m_c->m_drawTarget->Method_158d20()) {
        if (!m_c->m_drawTarget->Method_158cb0(0, 0x30000)) {
            return 0;
        }
    }

    RECT rc;
    rc.left = 0;
    rc.top = 8;
    rc.right = 0x27f;
    rc.bottom = 0x1df;
    m_1b4 = new CChatBox;
    m_1b4->Init();

    // 0x182ab0 is __thiscall on the freshly-built CChatBox (retail: `mov [esi+0x1b4],ecx`
    // then `call 0x182ab0` with ecx still the new object; `ret 0x18` = callee-cleaned
    // 6 stack args).  It seeds the box from the resource holder + the game window's HWND.
    if (!m_1b4->InitRegion(m_c, reinterpret_cast<i32>(m_4->m_gameWnd->m_hwnd), &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    if (m_1b4->AdvanceRow0(const_cast<char*>("MENU_CURSOR"), 0x64, 0x20)) {
        m_1b4->AdvanceRow1(const_cast<char*>("MENU_CURSOR"), 0x64, 0x20);
    }
    m_1b4->m_row0Key = "MENU_SELECT";
    m_1b4->m_row1Key = "MENU_ACTIVATE";

    LeafCue* e;
    m_c->m_soundRegistry->m_10.Lookup("MENU_ACTIVATE", reinterpret_cast<void*&>(e));
    if (e != 0) {
        m_c->m_soundRegistry->m_10.Lookup("MENU_ACTIVATE", reinterpret_cast<void*&>(e));
        m_1b8 = e->m_10->m_durationMs;
    } else {
        m_1b8 = 0;
    }

    if (!MenuCommit(m_1b4, -1)) {
        return 0;
    }

    LeafCue* fm;
    (static_cast<CDDrawSubMgrLeafScan*>(g_gameReg->m_world->m_soundRegistry))
        ->m_10.Lookup("MENU_MENU", reinterpret_cast<void*&>(fm));
    m_1bc = fm;
    return 1;
}

// ---------------------------------------------------------------------------
// CChatBox::Init (0xa0280) - homed here, not in ChatBox.cpp. 0xa0280 sits INSIDE
// this TU's own 0x9fe50..0xa0d80 .text run, bracketed on both sides by CMenuState
// methods (LoadGameAssetNamespaces 0x9fe50+0x343=0xa0193, then 0xa0280, then
// ReleaseResources 0xa02c0), while every other CChatBox method lives in the chat
// box's own block 0xe2000 away at 0x182ab0..0x183246. A compiland's .text run is
// contiguous, so the byte at 0xa0280 is contributed by THIS obj - retail's menu TU
// defines the chat box's field-zeroing init.
// ---------------------------------------------------------------------------
// re-zero both rows (no list teardown; Reset minus the Clear()).
RVA(0x000a0280, 0x2b)
void CChatBox::Init() {
    m_page = 0;
    m_4 = 0;
    m_activeNode = 0;
    m_row0Anim = 0;
    m_row1Anim = 0;
    m_row0Frame = 0;
    m_row1Frame = 0;
    m_row0Key.Empty();
    m_row1Key.Empty();
}

// CMenuState::ReleaseResources() (slot 2 / +0x8): release the MENU resource set
// (name registry + leaf registry), dispose the worker list, free the menu UI
// object, then chain CState::ReleaseResources. Also reached directly from ~CMenuState.
RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {
    // m_c re-read for each access (retail does not cache it); the null-guarded
    // block tests m_c once and reuses it for both the Free and DisposeWorkers.
    m_c->m_imageRegistry->RemoveKeysEqual_155360("MENU", "_");
    m_c->m_soundRegistry->RemoveKeysEqual_157c70("MENU", "_");
    if (m_c) {
        // The test value of m_c is reused for the leaf-registry access; the
        // worker-list dispose re-reads m_c fresh (retail does not cache it).
        SoundStream* r = m_c->m_soundRegistry->m_2c;
        if (r) {
            (static_cast<SoundStream*>(r))->Stop();
        }
        m_c->m_workerList->ClearWorkers();
    }
    // m_1b4 IS cached (retail holds it in edi across the pre-delete + delete).
    CChatBox* ui = m_1b4;
    if (ui) {
        delete ui; // ~CChatBox non-virtual -> direct dtor + ??3
        m_1b4 = 0;
    }
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

// CMenuState::StartMusic() (0xa05a0): if the menu music + the registry gate are
// live, push the configured item into the player on the draw-clock window, under
// a save/restored reentrancy gate.
RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_1bc == 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    i32 saved = g_sndEnabled;
    i32 flag = saved;
    if (!saved) {
        flag = 1;
        g_sndEnabled = 1;
    }
    i32 item = g_gameReg->m_soundVolume;
    LeafCue* mus = m_1bc;
    if (flag) {
        u32 clk = g_killCueClock;
        if (clk - mus->m_14 >= static_cast<u32>(mus->m_18)) {
            mus->m_14 = clk;
            mus->m_10->ConfigureItem(item, 0, 0, 1);
        }
    }
    if (!saved) {
        g_sndEnabled = saved;
    }
}

// CMenuState::StopMusicChain() (0xa0640): if the menu music is playing, request
// a fade-out stop, then spin the cursor/anim tick until playback ends.
RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_1bc == 0) {
        return;
    }
    LeafCue* mus = m_1bc;
    if (!mus->m_10->IsPlaying()) {
        return;
    }
    m_1bc->m_10->CloneAndPlay(0, 0x1f4, 1);
    if (!m_1bc->m_10->IsPlaying()) {
        return;
    }
    do {
        SoundStream* r = m_c->m_soundRegistry->m_2c;
        if (r) {
            (static_cast<SoundDevice*>(r))->PurgeVoiceList(-1);
        }
    } while (m_1bc->m_10->IsPlaying());
}

// CMenuState::FrameSlot28(int) (slot 10 / +0x28, 0xa06d0): flush + flip the menu
// view, stamp the start clock, run the music-stop chain, then busy-wait m_1b8 ms.
RVA(0x000a06d0, 0x5f)
i32 CMenuState::FrameSlot28(i32) {
    m_c->m_drawTarget->Method_158ee0();
    m_c->m_drawTarget->m_frontPair->m_surface->Flip(0);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_1b8)
        ;
    return 1;
}

// CMenuState::Render(): the front-end per-frame menu draw.
//   1. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   2. SIX entity-flag scans (masks 0x80000000/0x40000000/0x20000000/0x10000000/
//      0x3 (byte)/0x100): the FIRST scan that finds a flagged entity fires the
//      matching no-arg method on the UI object m_1b4 and short-circuits to the
//      tail; the 0x100 scan, if its handler returns 0, also posts a WM_COMMAND
//      0x8036 before the tail.
//   3. TAIL: m_1b4->Step(g_frameDelta); m_1b4->Pre(); DrawVersion({g_versionRect..d4});
//      m_1b4->Post();   return 1;
RVA(0x000a0750, 0x1d0)
i32 CMenuState::Render() {
    CGMEntityList* L = g_actorList;

    // per-entity Update pass (re-reads count each iter, like the target)
    for (i32 i = 0; i < L->m_count; i++) {
        L->m_data[i]->Update();
    }

    // six prioritized entity-flag scans, each firing a distinct UI handler
    i32 c;
    L = g_actorList;
    i32 n = L->m_count;
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_data[c]->m_2ac) & 0x80000000) {
            m_1b4->OnFlag80000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_data[c]->m_2ac) & 0x40000000) {
            m_1b4->OnFlag40000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_data[c]->m_2ac) & 0x20000000) {
            m_1b4->OnFlag20000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (static_cast<u32>(L->m_data[c]->m_2ac) & 0x10000000) {
            m_1b4->OnFlag10000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_data[c]->m_2ac & 0x3) {
            m_1b4->OnFlag00000003();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_data[c]->m_2ac & 0x100) {
            if (!m_1b4->OnFlag00000100()) {
                PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8036, 0);
            }
            goto tail;
        }
    }
tail:
    m_1b4->Step(g_frameDelta);
    m_1b4->Pre();
    BuildVersionString(g_versionRect); // 0xa0d80 (the fake DrawVersion alias folded away)
    m_1b4->Post();
    return 1;
}

// CMenuState::Vslot0c @0xa0b90 (slot 12) - the front-end keydown dispatcher: the
// arrow keys drive the menu UI object's HitTest nav, RETURN/SPACE its Activate, and
// ESCAPE its page Switch(1); when ESCAPE's Switch returns 0 (top page) it clears the
// fade duration and posts WM_COMMAND(0x8027) to the app window. Always returns 1.
RVA(0x000a0b90, 0xc7)
i32 CMenuState::Vslot0c(i32 key, i32 arg2) {
    if (key == 0x28) {
        m_1b4->HitTest2();
    } else if (key == 0x26) {
        m_1b4->HitTest1();
    } else if (key == 0x27) {
        m_1b4->HitTest4();
    } else if (key == 0x25) {
        m_1b4->HitTest3();
    } else if (key == 0xd || key == 0x20) {
        m_1b4->OnFlag00000003();
    } else if (key == 0x1b) {
        if (m_1b4->OnFlag00000100() == 0) {
            m_1b8 = 0;
            PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8027, 0);
        }
    }
    return 1;
}

// CMenuState slot 14 (+0x38) / slot 16 (+0x40): the two mouse hit-test forwarders -
// each passes (arg2, arg3) to the menu UI object's HitTest0 (== CChatBox::HitTest0),
// discards its result, and returns 1. arg1 (the message/event id) is unused.
RVA(0x000a0ca0, 0x21)
i32 CMenuState::Vslot0e(i32 arg1, i32 arg2, i32 arg3) {
    if (m_1b4) {
        m_1b4->HitTest0(arg2, arg3);
    }
    return 1;
}
RVA(0x000a0ce0, 0x21)
i32 CMenuState::Vslot10(i32 arg1, i32 arg2, i32 arg3) {
    if (m_1b4) {
        m_1b4->HitTest0(arg2, arg3);
    }
    return 1;
}

// CMenuState::ReadyGate @0x0a0d40 - the &&-chained ready/transition probe: poll
// the active/ready gate (IsActive, slot 3); if ready, attempt the pending state
// commit (CommitState, the 0x1136 thunk); if that succeeds, run the activation
// poll (Vslot06, slot 6). A short-circuit chain - each early bail returns the
// failing call's (zero) result.
RVA(0x000a0d40, 0x24)
i32 CMenuState::ReadyGate() {
    i32 r = IsActive();
    if (r == 0) {
        return r;
    }
    // The middle probe is the base CState::Vslot07 slot, invoked statically (direct
    // rel32 to 0xfac70, via the ILT thunk) - was the fake CMenuState::CommitState.
    r = CState::Vslot07();
    if (r == 0) {
        return r;
    }
    return Vslot06();
}

// CMenuState::BuildVersionString (0xa0d80): format the on-screen version banner
// into a transient CString, append " (SPAWN MODE)" when the CD prompt latched the
// spawn install, then hand it to the shared HUD-message sprite helper into the
// caller-supplied RECT (the 4 args form the RECT by value). The build/patch field
// g_versionMid selects the two- vs three-number version format.
DATA(0x00251608)
i32 g_versionMajor = 0; // decl in <Gruntz/GameMode.h>
DATA(0x0025160c)
i32 g_versionMid = 0; // decl in <Gruntz/GameMode.h>
DATA(0x00251610)
i32 g_versionMinor = 0; // decl in <Gruntz/GameMode.h>
// The shared HUD message-sprite helper (0x1154b0, glyphstr): push a transient text
// sprite carrying a CString into a RECT. Canonical signature is
// ?ShowHudMessage@@YAXPAUHudMsgSink@@HHHHHHHH@Z (1 sink ptr + 8 int words) - the
// text/rect ptrs ride the trailing int args (cast at the call, as in BootyStateActivate).
struct HudMsgSink;
void ShowHudMessage(
    HudMsgSink* sink,
    i32 text,
    i32 rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0
RVA(0x000a0d80, 0xd7)
void CMenuState::BuildVersionString(CGMVerRect r) {
    CString str;
    if (g_versionMid == 0) {
        str.Format("Gruntz v%d.%d", g_versionMajor, g_versionMinor);
    } else {
        str.Format("Gruntz v%d.%d%d", g_versionMajor, g_versionMid, g_versionMinor);
    }
    if (g_cdPromptResult) {
        str += " (SPAWN MODE)";
    }
    ShowHudMessage(reinterpret_cast<HudMsgSink*>(m_c), reinterpret_cast<i32>(&str), reinterpret_cast<i32>(&r), 0x64, 1, 0xff, 0xff, 0, 0);
}

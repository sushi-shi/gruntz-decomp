#include <Gruntz/MenuState.h> // C-linkage decls for the ex-wrapped defs
#include <Gruntz/GameMode.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
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
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawWorkerRegistry (m_c->m_imageRegistry)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <DDrawMgr/DDrawWorkerList.h>  // renderer B - the real CDDrawWorkerList (ClearWorkers)
#include <Win32.h>                     // IsDlgButtonChecked + HWND (real USER32 header)
#include <Gruntz/SoundState.h> // ex Globals.h transitive

DATA(0x00245574)
AttractActorList* g_actorList = 0;
DATA(0x00245cc8)
CGMVerRect g_versionRect; // .bss - zero at load

static inline CGruntzMgr* Owner(CState* s) {
    return s->m_mgr;
}

void operator delete(void*);

RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

i32 MenuCommit(CChatBox* obj, i32 idx); // 0x402fcc

i32 MenuCommit(CChatBox* obj, i32 idx); // 0x402fcc

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
    m_mgr->RestoreVideoMode(0);
    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_MENU"));
    if (m_2c == 0) {
        return 0;
    }

    if (!m_world->m_imageRegistry->HasKeyEqual("MENU")) {
        void* set = SymTab2c()->ResolvePath("IMAGEZ");
        if (set == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_world->m_imageRegistry->InstallTree(set, "MENU", "_");
        g_resourceInstallActive = 0;
    }

    if (!m_world->m_soundRegistry->HasKeyEqual("MENU")) {
        void* set = SymTab2c()->ResolvePath("SOUNDZ");
        if (set == 0) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(set), "MENU", "_");
    }

    if (!m_world->m_drawTarget->HasOverlay()) {
        if (!m_world->m_drawTarget->CreateOverlay(0, 0x30000)) {
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
    if (!m_1b4->InitRegion(m_world, m_mgr->m_gameWnd->m_hwnd, &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    if (m_1b4->AdvanceRow0(const_cast<char*>("MENU_CURSOR"), 0x64, 0x20)) {
        m_1b4->AdvanceRow1(const_cast<char*>("MENU_CURSOR"), 0x64, 0x20);
    }
    m_1b4->m_row0Key = "MENU_SELECT";
    m_1b4->m_row1Key = "MENU_ACTIVATE";

    LeafCue* e;
    m_world->m_soundRegistry->m_10.Lookup("MENU_ACTIVATE", reinterpret_cast<void*&>(e));
    if (e != 0) {
        m_world->m_soundRegistry->m_10.Lookup("MENU_ACTIVATE", reinterpret_cast<void*&>(e));
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

RVA(0x000a0280, 0x2b)
void CChatBox::Init() {
    m_page = 0;
    m_wnd = 0;
    m_activeNode = 0;
    m_row0Anim = 0;
    m_row1Anim = 0;
    m_row0Frame = 0;
    m_row1Frame = 0;
    m_row0Key.Empty();
    m_row1Key.Empty();
}

RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {
    // m_c re-read for each access (retail does not cache it); the null-guarded
    // block tests m_c once and reuses it for both the Free and DisposeWorkers.
    m_world->m_imageRegistry->RemoveKeysEqual("MENU", "_");
    m_world->m_soundRegistry->RemoveKeysEqual("MENU", "_");
    if (m_world) {
        // The test value of m_c is reused for the leaf-registry access; the
        // worker-list dispose re-reads m_c fresh (retail does not cache it).
        SoundStream* r = m_world->m_soundRegistry->m_2c;
        if (r) {
            (static_cast<SoundStream*>(r))->Stop();
        }
        m_world->m_workerList->ClearWorkers();
    }
    // m_1b4 IS cached (retail holds it in edi across the pre-delete + delete).
    CChatBox* ui = m_1b4;
    if (ui) {
        delete ui; // ~CChatBox non-virtual -> direct dtor + ??3
        m_1b4 = 0;
    }
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

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
        SoundStream* r = m_world->m_soundRegistry->m_2c;
        if (r) {
            (static_cast<SoundDevice*>(r))->PurgeVoiceList(-1);
        }
    } while (m_1bc->m_10->IsPlaying());
}

RVA(0x000a06d0, 0x5f)
i32 CMenuState::FrameSlot28(i32) {
    m_world->m_drawTarget->TransExit();
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_1b8)
        ;
    return 1;
}

RVA(0x000a0750, 0x1d0)
i32 CMenuState::Render() {
    AttractActorList* L = g_actorList;

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
                PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, 0x8036, 0);
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
            PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
        }
    }
    return 1;
}

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

RVA(0x000a0d40, 0x24)
i32 CMenuState::Vslot07() {
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

DATA(0x00251608)
i32 g_versionMajor = 0; // decl in <Gruntz/GameMode.h>
DATA(0x0025160c)
i32 g_versionMid = 0; // decl in <Gruntz/GameMode.h>
DATA(0x00251610)
i32 g_versionMinor = 0; // decl in <Gruntz/GameMode.h>
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
    ShowHudMessage(reinterpret_cast<HudMsgSink*>(m_world), reinterpret_cast<i32>(&str), reinterpret_cast<i32>(&r), 0x64, 1, 0xff, 0xff, 0, 0);
}

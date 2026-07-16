// CreditsState.cpp - CCreditsState, the credits/attract game-state (C:\Proj\Gruntz).
// Split out of the former GameMode.cpp god-TU (per-class TU cut): CCreditsState owns
// its full method set here (incl. SetupTitle @0x39a60, formerly hosted on the fake
// `CCreditzOwner` this-view). The CState base implementation stays in
// GameMode.cpp; the sibling states live in MenuState.cpp / BootyStateActivate.cpp.
// The ~CCreditsState `??1` (with the CState ctor) is the class's vtable +
// inline-virtual (Update) emission anchor - it stays in this TU.
//
// DE-VIEW PASS (2026-07-13): the 23 .cpp-local view structs this TU carried are GONE.
// Every one of them was a per-hop shadow of a class the tree already models, and the
// SAME FILE proved it: CCreditsState::Render already walked the real chain
// (m_c->m_drawTarget->m_frontPair->m_surface->m_8, CDDrawSurfaceMgr -> CDDrawSubMgrPages ->
// SurfaceA/B -> CDDSurface -> IDirectDrawSurface) while its neighbours re-modelled the
// identical hops as CreditsScrollView/CreditsView4/CreditsView4M14/CreditsHdcProv,
// CMenuRootA/CMenuPageA/CMenuBrightHolder and CCreditsDrawRoot/CCreditsDrawView/
// CCreditsDrawHolder/CCreditsSurface. Likewise CCreditzOwner + CreditsScrollSelf were
// two `this`-views of CCreditsState itself (their fields ARE the class's +0x1b4..+0x20c
// block), CCreditzImageRoot was CState::m_4 (the CGruntzMgr owner; +0x48 == m_sound, the
// spelling CreditzAssets.cpp already uses), CCreditzSoundMgr was CState::m_c, CCreditzRegSet
// was CState::m_8, CCreditzRegObj/CCreditzMusicSet were the CSymTab the code already cast
// them to, CCreditzSubEntry was CParseSource (its m_c IS m_length @+0x0c), CButeCfg was
// the canonical CButeMgr singleton and CCreditzAttractReg was the CGruntzMgr singleton
// (+0x80 == m_numRuns). Names are placeholders; only offsets + code bytes are load-bearing.
#include <Bute/SymParser.h>     // CSymParser::ResolvePath (LoadCreditz / InitAttractTitle)
#include <Bute/SymTab.h>        // CSymTab Insert/FindSub/ResolvePath (LoadCreditz / SetupTitle)
#include <Bute/ButeMgr.h>       // CButeMgr GetIntDef (InitAttractTitle brightness)
#include <Io/MoviePlayer.h>     // CMoviePlayer (~/CloseSmacker - ReleaseResources / StepVideo)
#include <Gruntz/BankMgr.h>     // CResSource (InitAttractTitle m_2c state store)
#include <Gruntz/ParseSource.h> // CParseSource (the resolved MIDIZ sub-entry: BeginParse/m_length)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ReleaseResources / LoadCreditz keys)
#include <DDrawMgr/DDrawSubMgrLeaf.h>  // CDDrawSubMgrLeaf (m_animRegistry RemoveKeysEqual_1527d0)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (the m_c->m_drawTarget page pump)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (was GameMode.cpp local view)
#include <DDrawMgr/DDSurface.h>           // CDDSurface (Render Draw / InitAttractTitle ShadeRect)
#include <Gruntz/GameMode.h>              // CCreditsState : CState + CGMOwner/CGMSoundEntry
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (CState::m_4 owner; m_sound @+0x48, m_numRuns @+0x80)
#include <Rez/RezMgr.h>       // RezFree (ReleaseResources video-handle teardown)
#include <Dsndmgr/GruntzSoundZ.h> // CGruntzSoundZ::CreateBank (0x138670) - credits sound-bank loader
#include <Win32.h>                // windows.h base types (ddraw.h needs them first)
#include <ddraw.h> // real IDirectDrawSurface (credits-scroll DC + Render input surface)
#include <rva.h>
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <stdio.h>                     // sprintf (InitAttractTitle STATEZ_ATTRACT/TITLE%d keys)
// Real MFC CRgn/CGdiObject for the credits clip region (CCreditsState::m_1e8).
// GameMode.h pulled <Mfc.h>->afx.h (defines _AFX_ENABLE_INLINES); skip afxwin*.inl for
// the clang label step only (implicit-int CMenu::op==); wine cl keeps the inlines.
// See docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

// The owning game-manager (CState::m_4, a real CGruntzMgr*) reached through the
// gamemode-local CGMOwner reduced view (same helper the sibling state TUs use). The
// sound-bank path does NOT go through it: m_4 is already typed CGruntzMgr*, so
// m_4->m_sound is cast-free (the spelling CreditzAssets.cpp uses).
static inline CGMOwner* Owner(CState* s) {
    return (CGMOwner*)s->m_4;
}

// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// The CButeMgr text-config singleton (?g_buteMgr@@3VCButeMgr@@A @ VA 0x6453d8 ->
// RVA 0x2453d8) - the canonical class g_buteMgr from <Bute/ButeMgr.h>; was the CButeCfg shell.

// The game-manager singleton (0x64556c). Its +0x80 launch counter (m_numRuns,
// "Num_Runs") rotates the attract TITLE index; same object as CState::m_4. Spelled
// CGruntzMgr* here exactly as the sibling CreditzAssets.cpp does (was CCreditzAttractReg).
extern "C" CGruntzMgr* g_gameReg;

// The attract-state count divisor (DEFINED in src/Rez/RezSync.cpp) is declared in
// <Gruntz/Attract.h> (included below).
#include <Gruntz/Attract.h>

// StepVideo: the Smacker frame-step wrapper (FUN_0057c8e0): __stdcall(handle, frame);
// ret nonzero while more frames remain (PTR__SmackGoto@8). Reloc-masked.
extern "C" i32 __stdcall Eng_SmackStep(void* handle, i32 frame);

DATA(0x0022bf74)
extern "C" i32 g_clipRegionEnabled; // 0x62bf74 - gates the credits CRgn clip

// The credits-scroll reseed constants (0x5e96f8 / 0x5e96f0 / 0x5e9708 in retail's
// .rdata). They were `extern double g_5e96f8;`-style DECLARATIONS with no definition
// anywhere - fabricated symbols, guaranteed unresolved externals. They are ordinary
// file-scope constants: MSVC5 keeps a `const double` in memory (it does NOT fold FP
// constant expressions the way it folds integer ones), so `kScreenH / kScrollRate`
// still lowers to retail's `fld QWORD PTR [c1]; fdiv QWORD PTR [c2]; call __ftol`.
static const double kScreenH = 480.0;    // 0x5e96f8  screen height
static const double kScrollRate = 0.025; // 0x5e96f0  scroll rate
static const double kStepScale = 1000.0; // 0x5e9708  scroll-step scale (m_scrollStep reseed)

// ===========================================================================
// CCreditsState methods, ascending retail-RVA order.
// ===========================================================================

// @confidence: high
// @source: decomp-xref
// CCreditsState::LoadGameAssetNamespaces (0x38d20; the slot-1 override, ex
// "LoadCreditzStateAssets"). Byte-exact (100%). int (BOOL) return
// like its loader siblings; the literal `return 0;` keeps the opening/Init guards as
// `test eax,eax`. The MONOLITH block is a SIBLING `if(midiz)` so the second
// `cmp edi,ebp; je` survives (docs/patterns/redundant-sibling-guard-retest.md). The
// 'IMX' music tag (0x584d49) is a non-relocated immediate. The "STATEZ_CREDITZ" Register
// is the CHelpState slot-1 source (0x13c030 == CSymParser::ResolvePath).
RVA(0x00038d20, 0x176)
i32 CCreditsState::LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) {
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (!CState::LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;

    m_1b8 = 0;
    m_1bc = 0;
    m_1c0 = 0;
    m_1c4 = 0;
    m_2c = (CResSource*)m_8->ResolvePath("STATEZ_CREDITZ");
    if (!m_2c) {
        return 0;
    }

    void* sounds = SymTab2c()->FindSub("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    m_c->m_soundRegistry->ScanTree_157ee0((CSymTab*)sounds, "CREDITZ", "_");

    CSymTab* midiz = (CSymTab*)SymTab2c()->ResolvePath("MIDIZ");
    if (midiz) {
        CParseSource* e = (CParseSource*)midiz->Insert("PLAY", (void*)0x584d49);
        if (e) {
            i32 val = e->BeginParse();
            if (val) {
                m_4->m_sound->CreateBank((void*)val, e->m_length, "CREDITZ"); // 0x138670
            }
        }
    }
    // Sibling re-test (not nested): retail re-emits `cmp edi,ebp; je` for the
    // MONOLITH block, pinning midiz in edi across the PLAY calls
    // (docs/patterns/redundant-sibling-guard-retest.md).
    if (midiz) {
        CParseSource* e2 = (CParseSource*)midiz->Insert("MONOLITH", (void*)0x584d49);
        if (e2) {
            i32 val = e2->BeginParse();
            if (val) {
                m_4->m_sound->CreateBank((void*)val, e2->m_length, "MONOLITH"); // 0x138670
            }
        }
    }

    if (!m_c->m_drawTarget->Method_158d20()) {
        if (!m_c->m_drawTarget->Method_158cb0(0, 0x30000)) {
            return 0;
        }
    }

    SetupTitle();
    m_20c = 2;
    i32 r = FinishState();
    m_1b4 = 0;
    return r;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::ReleaseResources() (0x38f00): if (m_c) free the pooled resource then
// release the three named registries ("CREDITZ"); then tear down + RezFree the video
// handle (m_videoHandle) and chain CState::ReleaseResources. m_c is re-read for each access.
RVA(0x00038f00, 0x87)
void CCreditsState::ReleaseResources() {
    if (m_c) {
        SoundStream* r = m_c->m_soundRegistry->m_2c;
        if (r) {
            ((SoundStream*)r)->Stop();
        }
        m_c->m_soundRegistry->RemoveKeysEqual_157c70("CREDITZ", "_");
        m_c->m_imageRegistry->RemoveKeysEqual_155360("CREDITZ", "_");
        // retail: `mov ecx,[edx+0x2c]; call 0x1527d0` - the Leaf's own remove (the old
        // (CDDrawWorkerRegistry*) cast MISBOUND this call to the +0x10 twin's 0x155360).
        m_c->m_animRegistry->RemoveKeysEqual_1527d0("CREDITZ", "_");
    }
    // Cache the video handle in a local so it stays pinned in edi across the
    // Teardown call (retail reuses the same register for the RezFree push).
    CMoviePlayer* vh = m_videoHandle;
    if (vh) {
        vh->~CMoviePlayer();
        RezFree(vh);
        m_videoHandle = 0;
    }
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

// CCreditsState::Vslot09 (slot 9 / +0x24, 0x39120): force the OS cursor hidden,
// then (re)prime the attract title; returns whether InitAttractTitle succeeded.
RVA(0x00039120, 0x2c)
i32 CCreditsState::Vslot09(i32 /*unused*/) {
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    return InitAttractTitle() != 0;
}

// CCreditsState::Render(): the canonical Render spine (input poll -> input-virtual bail
// -> cursor anim -> per-entity Update loop -> message scan -> two sub-steps -> draw ->
// two latched one-shot FX).
RVA(0x000391d0, 0x17c)
i32 CCreditsState::Render() {
    IDirectDrawSurface* in = m_c->m_drawTarget->m_frontPair->m_surface->m_8;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            Owner(this)->Post(0x8006, 0xfa0);
            return 0;
        }
    }

    if (m_c->m_soundRegistry->m_2c) {
        GM_SimpleAnim(-1);
    }

    // per-entity Update pass
    {
        CGMEntityList* L = g_actorList;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_data[i]->Update();
        }
    }

    // message scan: first flagged entity posts a WM_COMMAND
    {
        CGMEntityList* L = g_actorList;
        i32 n = L->m_count;
        for (i32 j = 0; j < n; j++) {
            if (L->m_data[j]->m_2ac & 0xffffff) {
                // wParam = (m_24==5) ? 0x8023 : 0x8027. The init+conditional-override
                // keeps the cmp+jne branch (docs body comment in the credits Render).
                u32 wp = 0x8027;
                if (m_24 == 5) {
                    wp = 0x8023;
                }
                PostMessageA(Owner(this)->m_4->m_4, 0x111, wp, 0);
                Owner(this)->m_8->m_244 = 0;
                break;
            }
        }
    }

    Sub1();
    Sub2();

    // draw: cache m_c->m_drawTarget (the target keeps it in esi for the three derefs).
    CDDrawSubMgrPages* v4 = m_c->m_drawTarget;
    v4->m_frontPair->m_surface->Draw(0);
    v4->m_backPair->BltSelf(
        v4->m_overlayPair
    ); // SurfaceB::Blit WAS CDDrawSurfacePair::BltSelf @0x3a1d0 (thunk 0x1564)

    if (!m_1b4 && Owner(this)->m_14) {
        Owner(this)->m_48->Play("CREDITZ", 1);
        m_1b4 = 1;
    }

    if (m_1c4) {
        i32 s = Owner(this)->m_48->Find("MONOLITH");
        if (s && !((CGMSoundEntry*)s)->Query()) {
            Sub3();
        }
    }
    return 1;
}

// CCreditsState::InputVirtual (slot 8 / +0x20, @0x393b0, formerly ShowAttractTitle) -
// the per-frame input poll: gate on the page pump (m_c->m_drawTarget, dispatched as
// CDDrawSubMgrPages); if loaded, force the cursor hidden then prime the attract title.
// Returns 1 (0 when not loaded).
RVA(0x000393b0, 0x3a)
i32 CCreditsState::InputVirtual() {
    // the page pump at m_c->m_drawTarget is CDDrawSubMgrPages; the ready gate is
    // Method_158bc0 (0x158bc0) - NOT CParseSource::BeginParse (0x139960); same page gate
    // the sibling states (CHelpState/CSplashState) poll.
    if (m_c->m_drawTarget->Method_158bc0() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    InitAttractTitle();
    return 1;
}

// CCreditsState::Vslot06 (slot 6 / +0x18, 0x39400): the Vfunc3-gated title roll -
// bail unless the state's ready gate (Vfunc3) is set, hide the cursor, then (re)prime
// the attract title and return its result.
RVA(0x00039400, 0x2f)
i32 CCreditsState::Vslot06() {
    if (Vfunc3() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    return InitAttractTitle();
}

// CCreditsState::Vslot0c (slot 12 / +0x30, 0x39440): keydown handler - on ESC/SPACE/
// ENTER post a WM_COMMAND to the top-level window (0x8023 in active-selection mode
// m_24==5, else 0x8027).
RVA(0x00039440, 0x46)
i32 CCreditsState::Vslot0c(i32 code, i32 unused) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        if (m_24 == 5) {
            PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8023, 0);
        } else {
            PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8027, 0);
        }
    }
    return 1;
}

// CCreditsState::Vslot0e (slot 14 / +0x38, 0x394b0): the credits-title click hit-test.
// XREF-recovered identity (was the fake ClickHost_0394b0 view): the retail vtable
// ??_7CCreditsState@@6B@+0x38 (slot 14) references this RVA. If the click (x,y) lands in
// the 0..0x64 title box, run the music-bank swap (LoadCreditzAssets, via ILT thunk
// 0x3d41); otherwise post a WM_COMMAND (0x8023 in selection mode m_24==5, else 0x8027).
// Always returns 1. arg2 is unused (the mouse-message middle word).
// @early-stop
// RECT/POINT stack-frame codegen plateau (~50%): logic (PtInRect hit-test -> Activate
// else WM_COMMAND) is byte-faithful; the residual is the RECT/POINT locals' stack-slot
// layout + the branchless wParam select form, not source-steerable.
RVA(0x000394b0, 0x86)
i32 CCreditsState::Vslot0e(i32 x, i32 unused, i32 y) {
    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = 0x64;
    rc.bottom = 0x64;
    POINT pt;
    pt.x = x;
    pt.y = y;
    if (PtInRect(&rc, pt)) {
        LoadCreditzAssets();
        return 1;
    }
    i32 cmd;
    if (m_24 == 5) {
        cmd = 0x8023;
    } else {
        cmd = 0x8027;
    }
    PostMessageA(Owner(this)->m_4->m_4, 0x111, cmd, 0);
    return 1;
}

// InitAttractTitle (0x39570): the credits/attract title (re)init - the twin of
// CAttract::LoadTitleConfig. If the title view is already live (m_videoPlaying), just
// run the menu-page frame sub-steps and bail; otherwise pick a rotating TITLE index off
// the mgr launch counter (g_gameReg->m_numRuns), format the "STATEZ_ATTRACT"/"TITLE%d"
// keys, resolve the attract state into m_2c, fade the title in, apply the configured
// brightness, transition the page, and build the menu page.
// @early-stop
// 81.2%: logic byte-faithful (the twin of CAttract::LoadTitleConfig). Residual is the
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md)
// on the FadeInTitle fail return-0 + the sprintf stack-buffer slot layout. Not steerable.
RVA(0x00039570, 0x122)
i32 CCreditsState::InitAttractTitle() {
    CDDrawSurfaceMgr* root = m_c;
    if (m_videoPlaying != 0) {
        ((CDDrawSubMgrPages*)root->m_drawTarget)->Method_158dc0();
        ((CDDrawSubMgrPages*)root->m_drawTarget)->Method_158e90();
        ((CDDrawSubMgrPages*)root->m_drawTarget)->Method_158d50(0);
        root->m_drawTarget->m_overlayPair->m_surface->Fill(0);
        return 1;
    }
    char stateName[0x20];
    char titleName[0x20];
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);
    void* saved = (void*)m_2c;
    void* state = m_8->ResolvePath(stateName);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }
    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (CResSource*)saved;
    if (faded == 0) {
        return 0;
    }
    CDDSurface* tgt = root->m_drawTarget->m_backPair->m_surface;
    tgt->ShadeRect(g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32), 0);
    ((CDDrawSubMgrPages*)root->m_drawTarget)->Method_158e90();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited; ex "BuildMenuPage")
    return 1;
}

// DrawScrollingCredits (0x396f0): the credits scroll-text renderer. Ticks the three
// overlay timers down by the frame delta, advances the scrolling caption RECT by
// `delta * speed * 0.001` (wrapping when it scrolls off, reseeding m_1f4), then - if the
// IDirectDrawSurface hands back an HDC - paints the caption transparently and (when the
// gates are live) the static credit. GDI via the IAT; GetDC/ReleaseDC are the DDraw COM
// slots (+0x44/+0x68). CString temp -> /GX frame.
// @early-stop
// ~75%: complete + correct (matched jb-branch polarity, fadd accumulator with the extern
// reseed constants blocking the /O2 constant-fold, the DDraw GetDC/ReleaseDC COM slots,
// the CGdiObject::operator-HRGN null-check clip, both DrawTextA paths + the static credit
// CString). Residual walls: (1) the /GX EH-frame representation (docs/seh-eh.md); (2)
// float-consistency (/Op) st0 reuse vs store/reload; (3) FP/prov-chain scheduling. All
// logic + externs/strings named; the 3 FP-constant relocs stay differently-named.
RVA(0x000396f0, 0x2b8)
i32 CCreditsState::DrawScrollingCredits() {
    if (m_c == 0) {
        return 0;
    }
    // The credits scroll paints through the draw-surface page's real CDDSurface
    // (m_c->m_drawTarget->m_backPair->m_surface) and its held IDirectDrawSurface (m_8): GetDC is
    // COM slot 17 (+0x44), ReleaseDC slot 26 (+0x68).
    CDDSurface* prov = m_c->m_drawTarget->m_backPair->m_surface;

    if (g_frameDelta >= m_1f4) {
        m_1f4 = 0;
    } else {
        m_1f4 -= g_frameDelta;
    }
    if (m_1c4 != 0) {
        if (g_frameDelta >= m_1bc) {
            m_1bc = 0;
        } else {
            m_1bc -= g_frameDelta;
        }
        if (g_frameDelta >= m_1c0) {
            m_1c0 = 0;
        } else {
            m_1c0 -= g_frameDelta;
        }
    }

    m_drawRect = m_scrollRect;
    double contrib = static_cast<double>(g_frameDelta) * m_scrollStep * 0.001;
    m_scrollAccum = m_scrollAccum + contrib;
    i32 scrolled = static_cast<i32>(m_scrollAccum);
    m_drawRect.top -= scrolled;
    m_drawRect.bottom -= scrolled;
    if (m_drawRect.bottom < 0) {
        m_scrollAccum = 0.0;
        m_drawRect = m_scrollRect;
        m_1f4 = static_cast<i32>((kScreenH / kScrollRate));
    }

    HDC hdc = 0;
    prov->m_8->GetDC(&hdc);
    if (hdc != 0) {
        i32 oldBk = SetBkMode(hdc, TRANSPARENT);
        if (g_clipRegionEnabled != 0) {
            SelectClipRgn(hdc, m_1e8); // CRgn -> HRGN via CGdiObject::operator HRGN
        }
        i32 oldColor = SetTextColor(hdc, FlashColor());
        DrawTextA(hdc, m_caption, -1, &m_drawRect, 0x50);
        SetTextColor(hdc, oldColor);
        if (m_1c4 != 0 && m_1c0 != 0) {
            CString s("Now is the time at Monolith when we dance");
            RECT r = {0, 0, 0x280, 0x1e0};
            i32 oldColor2 = SetTextColor(hdc, 0xffffff);
            DrawTextA(hdc, s, -1, &r, 0x75); // CString -> LPCTSTR (implicit)
            SetTextColor(hdc, oldColor2);
        }
        if (g_clipRegionEnabled != 0) {
            SelectClipRgn(hdc, 0);
        }
        SetBkMode(hdc, oldBk);
        prov->m_8->ReleaseDC(hdc);
    }
    return 1;
}

// CCreditsState::SetupTitle (0x39a60, __thiscall, ret BOOL) - build the credits scroll:
// pull the "CREDITZ" TXT section from the STATEZ_CREDITZ store (m_2c, dispatched as
// CSymTab) into the m_caption CString, build the clip region, measure the text against
// the offscreen DDraw surface's HDC to set the scroll rect, then seed the scroll
// accumulator. (Was hosted on the fake CCreditzOwner this-view.)
// @early-stop
// scheduling tail (~99.7%): for the final (double)(unsigned)m_1f4 conversion the int64
// temp's two halves are emitted in the opposite order relative to the fld/fmul. All other
// bytes identical (llvm-objdump -dr). GetDC/ReleaseDC region is byte-exact via the real
// IDirectDrawSurface COM slots.
RVA(0x00039a60, 0x179)
i32 CCreditsState::SetupTitle() {
    // CSymTab::Insert resolves the "CREDITZ" section of FOURCC type 'TXT'
    // (== 0x545854, a tag value not an address); returns the section CParseSource as H.
    CParseSource* sect = (CParseSource*)SymTab2c()->Insert("CREDITZ", (void*)'TXT');
    if (sect) {
        char* src = (char*)sect->BeginParse();
        if (!src) {
            return 0;
        }
        i32 len = sect->m_length;
        char* buf = (char*)operator new(len + 1);
        if (!buf) {
            return 0;
        }
        memcpy(buf, src, len);
        buf[len] = 0;
        m_caption = buf;
        sect->EndParse();
        operator delete(buf);
    }
    m_1e8.Attach(CreateRectRgn(0x32, 0, 0x24e, 0x1e0));
    CDDSurface* prov = m_c->m_drawTarget->m_backPair->m_surface;
    HDC hdc = 0;
    prov->m_8->GetDC(&hdc);
    if (hdc) {
        i32 h = DrawTextA(hdc, m_caption, -1, &m_drawRect, 0x450);
        SetRect(&m_scrollRect, 0x32, 0x1e0, 0x24e, h + 0x1e0);
        prov->m_8->ReleaseDC(hdc);
    }
    m_scrollAccum = 0.0;
    m_1f4 = static_cast<i32>((kScreenH / kScrollRate));
    m_scrollStep = (kScreenH * kStepScale) / static_cast<double>(static_cast<unsigned>(m_1f4));
    return 1;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::FinishState() (0x39c40): clear the playing gate, return 1.
RVA(0x00039c40, 0x10)
i32 CCreditsState::FinishState() {
    m_videoPlaying = 0;
    return 1;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::StepVideo() (0x39c60): if the credits aren't playing return 1. Else
// advance the Smacker movie one frame; when the last frame is reached, Close() the handle
// and FinishState(). Either way, if both surfaces are live, blit the current frame.
// The draw chain is the real one: m_c->m_drawTarget (CDDrawSubMgrPages) -> the +0x14 / +0x18
// surface pages -> their CDDSurface (m_2c) and the page's own BltFast source RECT (m_1c).
// @early-stop
// scheduling coin-flip wall (~95%): 49/51 instructions byte-identical; the sole residual
// is the BltFast `this` (src->m_2c) load scheduled one push earlier in retail + scratch-
// reg rotation. Complete + correct body; not source-steerable (zero-register-pinning.md).
RVA(0x00039c60, 0x7a)
i32 CCreditsState::StepVideo() {
    if (!m_videoPlaying) {
        return 1;
    }
    i32 ret = 0;
    if (m_videoHandle) {
        CDDrawSubMgrPages* v = m_c->m_drawTarget;
        CDDrawSurfacePair* dst = v->m_overlayPair;
        CDDrawSurfacePair* src = v->m_backPair;
        if (!Eng_SmackStep(dst->m_surface->m_8, -1)) {
            m_videoHandle->CloseSmacker();
            ret = FinishState();
        }
        if (dst && src) {
            src->m_surface->BltFast(0, 0, dst->m_surface, &dst->m_srcRect, 0x10);
        }
    }
    return ret;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::FlashColor() (0x39d00): if the flash gate (m_1c4) is set and the
// re-roll timer (m_1bc) has expired, roll a fresh random RGB color, reset the timer to
// 0x12c, latch it at m_1b8 and return it. Otherwise return the held color.
// @early-stop
// byte-insert RGB pack wall (~70%): the value-correct (b<<16)|(g<<8)|r packs via
// `mov ch,al; mov cl,bl; shl 8; or esi` in retail (g/b land in byte regs) vs two
// `shl 8; or` here, compounded by a shrink-wrapped callee-save push. See docs/patterns/
// rgb-pack-byte-insert.md + shrink-wrapped-callee-save-push.md; not steerable.
RVA(0x00039d00, 0x8c)
i32 CCreditsState::FlashColor() {
    i32 color = 0xffffff;
    if (m_1c4) {
        if (m_1bc) {
            return m_1b8;
        }
        i32 r = rand() % 256;
        i32 g = rand() % 256;
        i32 b = rand() % 256;
        m_1bc = 0x12c;
        color = (b << 16) | ((g & 0xff) << 8) | (r & 0xff);
        m_1b8 = color;
    }
    return color;
}

// CCreditsState::~CCreditsState (`??1`, 0x8d5e0): stamp the CCreditsState vptr, run
// ReleaseResources, then cl auto-destroys the m_caption CString + the m_1e8 CRgn in
// reverse-declaration order before chaining ~CState. /GX EH frame for the member unwind.
// This EH-framed `??1` (with the CState ctor) anchors the CCreditsState vtable +
// inline-virtual (Update) emission in this TU.
//
// The inlined ~CRgn teardown (CRgn own-stamp dead-store-elided: stamp ??_7CGdiObject,
// call CGdiObject::DeleteObject, restamp ??_7CObject) makes this TU emit the
// out-of-line ??1CRgn COMDAT; the retail-kept copy is the 0x8c400 body the fake
// `CHolder8c400` used to claim (RTTI at its vtable 0x1ea2a4 = .?AVCRgn@@).
// (??0CRgn @0x8c3b0 / ??_GCRgn @0x8c3d0 stay unbound: they were kept from the retail
// gruntzmgr obj, whose TransitionState calls ??0CRgn out-of-line - our TransitionState
// still reaches it through the declared-only CTsSub45 ctor, reloc-masked to the same
// rva, and no TU of ours emits those two COMDATs yet.)
// @rva-symbol: ??1CRgn@@UAE@XZ 0x0008c400 0x46
RVA(0x0008d5e0, 0x8b)
CCreditsState::~CCreditsState() {
    ReleaseResources();
}

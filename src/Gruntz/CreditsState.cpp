#include <Bute/SymParser.h> // CSymParser::ResolvePath (LoadCreditz / InitAttractTitle)
#include <Gruntz/GameRegMfcPtr.h>
#include <Bute/SymTab.h>        // CSymTab Insert/FindSub/ResolvePath (LoadCreditz / SetupTitle)
#include <Bute/ButeMgr.h>       // CButeMgr GetIntDef (InitAttractTitle brightness)
#include <Io/MoviePlayer.h>     // CMoviePlayer (~/CloseSmacker - ReleaseResources / StepVideo)
#include <Gruntz/BankMgr.h>     // CSymTab (InitAttractTitle m_2c state store)
#include <Gruntz/ParseSource.h> // CParseSource (the resolved MIDIZ sub-entry: BeginParse/m_length)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ReleaseResources / LoadCreditz keys)
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // CDDrawSubMgrLeaf (m_animRegistry RemoveKeysEqual)
#include <DDrawMgr/DDrawSubMgrPages.h>    // CDDrawSubMgrPages (the m_c->m_drawTarget page pump)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (was GameMode.cpp local view)
#include <DDrawMgr/DDSurface.h>           // CDDSurface (Render Draw / InitAttractTitle ShadeRect)
#include <DinMgr2/DirectInputMgr2.h>      // CInputDevBase (Poll/m_currentKeys press-edge flags)
#include <Gruntz/GameMode.h> // CCreditsState : CState (ex CGMOwner/CGMSound views dissolved to CGruntzMgr/CGruntzSoundZ)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (CState::m_4 owner; m_sound @+0x48, m_numRuns @+0x80)
#include <Rez/RezMgr.h>       // RezFree (ReleaseResources video-handle teardown)
#include <Dsndmgr/GruntzSoundZ.h> // CGruntzSoundZ::CreateBank (0x138670) - credits sound-bank loader
#include <Win32.h>                // windows.h base types (ddraw.h needs them first)
#include <ddraw.h> // real IDirectDrawSurface (credits-scroll DC + Render input surface)
#include <rva.h>
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <stdio.h>                     // sprintf (InitAttractTitle STATEZ_ATTRACT/TITLE%d keys)
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

static inline CGruntzMgr* Owner(CState* s) {
    return s->m_mgr;
}

void operator delete(void*);

#include <Gruntz/Attract.h>
#include <Gruntz/CreditsState.h> // g_clipRegionEnabled decl

DATA(0x0022bf74)
i32 g_clipRegionEnabled; // owner def (zero-init .bss)

VTBL(CCreditsState, 0x001e9c64);

static const double kScreenH = 480.0;    // 0x5e96f8  screen height
static const double kScrollRate = 0.025; // 0x5e96f0  scroll rate
static const double kStepScale = 1000.0; // 0x5e9708  scroll-step scale (m_scrollStep reseed)

RVA(0x00038d20, 0x176)
i32 CCreditsState::LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) {
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (!CState::LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;

    m_flashColor = 0;
    m_flashTimer = 0;
    m_fadeCountdown = 0;
    m_fxEnabled = 0;
    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_CREDITZ"));
    if (!m_2c) {
        return 0;
    }

    void* sounds = SymTab2c()->FindSub("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(sounds), "CREDITZ", "_");

    CSymTab* midiz = static_cast<CSymTab*>(SymTab2c()->ResolvePath("MIDIZ"));
    if (midiz) {
        CParseSource* e = midiz->Insert("PLAY", reinterpret_cast<void*>(0x584d49));
        if (e) {
            i32 val = e->BeginParse();
            if (val) {
                m_mgr->m_sound
                    ->CreateBank(reinterpret_cast<void*>(val), e->m_length, "CREDITZ"); // 0x138670
            }
        }
    }
    // Sibling re-test (not nested): retail re-emits `cmp edi,ebp; je` for the
    // MONOLITH block, pinning midiz in edi across the PLAY calls
    // (docs/patterns/redundant-sibling-guard-retest.md).
    if (midiz) {
        CParseSource* e2 = midiz->Insert("MONOLITH", reinterpret_cast<void*>(0x584d49));
        if (e2) {
            i32 val = e2->BeginParse();
            if (val) {
                m_mgr->m_sound->CreateBank(
                    reinterpret_cast<void*>(val),
                    e2->m_length,
                    "MONOLITH"
                ); // 0x138670
            }
        }
    }

    if (!m_world->m_drawTarget->HasOverlay()) {
        if (!m_world->m_drawTarget->CreateOverlay(0, 0x30000)) {
            return 0;
        }
    }

    SetupTitle();
    m_20c = 2;
    i32 r = FinishState();
    m_musicStarted = 0;
    return r;
}

RVA(0x00038f00, 0x87)
void CCreditsState::ReleaseResources() {
    if (m_world) {
        SoundStream* r = m_world->m_soundRegistry->m_2c;
        if (r) {
            (static_cast<SoundStream*>(r))->Stop();
        }
        m_world->m_soundRegistry->RemoveKeysEqual("CREDITZ", "_");
        m_world->m_imageRegistry->RemoveKeysEqual("CREDITZ", "_");
        // retail: `mov ecx,[edx+0x2c]; call 0x1527d0` - the Leaf's own remove (the old
        // (CDDrawWorkerRegistry*) cast MISBOUND this call to the +0x10 twin's 0x155360).
        m_world->m_animRegistry->RemoveKeysEqual("CREDITZ", "_");
    }
    // Cache the video handle in a local so it stays pinned in edi across the
    // Teardown call (retail reuses the same register for the RezFree push).
    CMoviePlayer* vh = m_videoHandle;
    if (vh) {
        // RezFree IS ::operator delete (both 0x1b9b82), so this pair IS `delete vh`.
        delete vh; // ~CMoviePlayer non-virtual (0x038fc0) + ??3
        m_videoHandle = 0;
    }
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

RVA(0x00039120, 0x2c)
i32 CCreditsState::Vslot09(i32 /*unused*/) {
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    return InitAttractTitle() != 0;
}

RVA(0x000391d0, 0x17c)
i32 CCreditsState::Render() {
    IDirectDrawSurface* in = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            Owner(this)->ReportError(0x8006, 0xfa0);
            return 0;
        }
    }

    if (m_world->m_soundRegistry->m_2c) {
        GM_SimpleAnim(-1);
    }

    // per-entity Update pass
    {
        CFixedPtrArray32* L = g_actorList;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_items[i]->Poll();
        }
    }

    // message scan: first flagged entity posts a WM_COMMAND
    {
        CFixedPtrArray32* L = g_actorList;
        i32 n = L->m_count;
        for (i32 j = 0; j < n; j++) {
            if (L->m_items[j]->m_currentKeys & 0xffffff) {
                // wParam = (m_24==5) ? 0x8023 : 0x8027. The init+conditional-override
                // keeps the cmp+jne branch (docs body comment in the credits Render).
                u32 wp = 0x8027;
                if (m_24 == 5) {
                    wp = 0x8023;
                }
                PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, wp, 0);
                Owner(this)->m_owner->m_running = 0;
                break;
            }
        }
    }

    StepVideo();            // 0x39c60 (ex Sub1)
    DrawScrollingCredits(); // 0x396f0 (ex Sub2)

    // draw: cache m_c->m_drawTarget (the target keeps it in esi for the three derefs).
    CDDrawSubMgrPages* v4 = m_world->m_drawTarget;
    v4->m_frontPair->m_surface->Flip(0);
    v4->m_backPair->BltSelf(
        v4->m_overlayPair
    ); // SurfaceB::Blit WAS CDDrawSurfacePair::BltSelf @0x3a1d0 (thunk 0x1564)

    if (!m_musicStarted && Owner(this)->m_musicEnabled) {
        Owner(this)->m_sound->PlayByName("CREDITZ", 1);
        m_musicStarted = 1;
    }

    if (m_fxEnabled) {
        CGruntzSoundInnerZ* s = Owner(this)->m_sound->FindBank("MONOLITH");
        if (s && !s->IsStarted()) {
            Sub3();
        }
    }
    return 1;
}

RVA(0x000393b0, 0x3a)
i32 CCreditsState::InputVirtual() {
    // the page pump at m_c->m_drawTarget is CDDrawSubMgrPages; the ready gate is
    // PagesReady (0x158bc0) - NOT CParseSource::BeginParse (0x139960); same page gate
    // the sibling states (CHelpState/CSplashState) poll.
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    InitAttractTitle();
    return 1;
}

RVA(0x00039400, 0x2f)
i32 CCreditsState::Vslot06() {
    if (IsActive() == 0) {
        return 0;
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    return InitAttractTitle();
}

RVA(0x00039440, 0x46)
i32 CCreditsState::Vslot0c(i32 code, i32 unused) {
    if (code == 0x1b || code == 0x20 || code == 0xd) {
        if (m_24 == 5) {
            PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
        } else {
            PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
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
    PostMessageA(Owner(this)->m_gameWnd->m_hwnd, 0x111, cmd, 0);
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
    CDDrawSurfaceMgr* root = m_world;
    if (m_videoPlaying != 0) {
        (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->PresentBackPage();
        (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->TransTitle();
        (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->ClearAllPages(0);
        root->m_drawTarget->m_overlayPair->m_surface->Fill(0);
        return 1;
    }
    char stateName[0x20];
    char titleName[0x20];
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);
    void* saved = static_cast<void*>(m_2c);
    void* state = m_symParser->ResolvePath(stateName);
    m_2c = static_cast<CSymTab*>(state);
    if (state == 0) {
        return 0;
    }
    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = static_cast<CSymTab*>(saved);
    if (faded == 0) {
        return 0;
    }
    CDDSurface* tgt = root->m_drawTarget->m_backPair->m_surface;
    tgt->ShadeRect(g_buteMgr.GetIntDef("Menu", "BrightnessPercent", 0x32), 0);
    (static_cast<CDDrawSubMgrPages*>(root->m_drawTarget))->TransTitle();
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
    if (m_world == 0) {
        return 0;
    }
    // The credits scroll paints through the draw-surface page's real CDDSurface
    // (m_c->m_drawTarget->m_backPair->m_surface) and its held IDirectDrawSurface (m_8): GetDC is
    // COM slot 17 (+0x44), ReleaseDC slot 26 (+0x68).
    CDDSurface* prov = m_world->m_drawTarget->m_backPair->m_surface;

    if (g_frameDelta >= m_scrollReseedTimer) {
        m_scrollReseedTimer = 0;
    } else {
        m_scrollReseedTimer -= g_frameDelta;
    }
    if (m_fxEnabled != 0) {
        if (g_frameDelta >= m_flashTimer) {
            m_flashTimer = 0;
        } else {
            m_flashTimer -= g_frameDelta;
        }
        if (g_frameDelta >= m_fadeCountdown) {
            m_fadeCountdown = 0;
        } else {
            m_fadeCountdown -= g_frameDelta;
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
        m_scrollReseedTimer = static_cast<i32>((kScreenH / kScrollRate));
    }

    HDC hdc = 0;
    prov->m_ddSurface->GetDC(&hdc);
    if (hdc != 0) {
        i32 oldBk = SetBkMode(hdc, TRANSPARENT);
        if (g_clipRegionEnabled != 0) {
            SelectClipRgn(hdc, m_1e8); // CRgn -> HRGN via CGdiObject::operator HRGN
        }
        i32 oldColor = SetTextColor(hdc, FlashColor());
        DrawTextA(hdc, m_caption, -1, &m_drawRect, 0x50);
        SetTextColor(hdc, oldColor);
        if (m_fxEnabled != 0 && m_fadeCountdown != 0) {
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
        prov->m_ddSurface->ReleaseDC(hdc);
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
    CParseSource* sect = SymTab2c()->Insert("CREDITZ", reinterpret_cast<void*>('TXT'));
    if (sect) {
        char* src = reinterpret_cast<char*>(sect->BeginParse());
        if (!src) {
            return 0;
        }
        i32 len = sect->m_length;
        char* buf = static_cast<char*>(operator new(len + 1));
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
    CDDSurface* prov = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = 0;
    prov->m_ddSurface->GetDC(&hdc);
    if (hdc) {
        i32 h = DrawTextA(hdc, m_caption, -1, &m_drawRect, 0x450);
        SetRect(&m_scrollRect, 0x32, 0x1e0, 0x24e, h + 0x1e0);
        prov->m_ddSurface->ReleaseDC(hdc);
    }
    m_scrollAccum = 0.0;
    m_scrollReseedTimer = static_cast<i32>((kScreenH / kScrollRate));
    m_scrollStep =
        (kScreenH * kStepScale) / static_cast<double>(static_cast<unsigned>(m_scrollReseedTimer));
    return 1;
}

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
        CDDrawSubMgrPages* v = m_world->m_drawTarget;
        CDDrawSurfacePair* dst = v->m_overlayPair;
        CDDrawSurfacePair* src = v->m_backPair;
        if (!Eng_SmackStep(dst->m_surface->m_ddSurface, -1)) {
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
    if (m_fxEnabled) {
        if (m_flashTimer) {
            return m_flashColor;
        }
        i32 r = rand() % 256;
        i32 g = rand() % 256;
        i32 b = rand() % 256;
        m_flashTimer = 0x12c;
        color = (b << 16) | ((g & 0xff) << 8) | (r & 0xff);
        m_flashColor = color;
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
RVA_COMPGEN(0x0008c400, 0x46, ??1CRgn@@UAE@XZ)
RVA(0x0008d5e0, 0x8b)
CCreditsState::~CCreditsState() {
    ReleaseResources();
}

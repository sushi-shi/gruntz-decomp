// CreditsState.cpp - CCreditsState, the credits/attract game-state (C:\Proj\Gruntz).
// Split out of the former GameMode.cpp god-TU (per-class TU cut): CCreditsState +
// CCreditzOwner (the credits-scroll owner facet, SetupTitle @0x39a60) now own their
// full method set here. The CState base + the CGameModeBase cleanup pair stay in
// GameMode.cpp; the sibling states live in MenuState.cpp / BootyStateActivate.cpp.
// The ~CCreditsState `??1` (with the CState ctor) is the class's vtable +
// inline-virtual (Update) emission anchor - it stays in this TU. Names are
// placeholders; only offsets + code bytes are load-bearing.
#include <Bute/SymParser.h>     // CSymParser::ResolvePath (LoadCreditz / InitAttractTitle)
#include <Bute/SymTab.h>        // CSymTab Insert/FindSub/ResolvePath (LoadCreditz / SetupTitle)
#include <Bute/ButeMgr.h>       // CButeMgr GetIntDef (InitAttractTitle brightness)
#include <Io/MoviePlayer.h>     // CMoviePlayer (~/CloseSmacker - ReleaseResources / StepVideo)
#include <Gruntz/BankMgr.h>     // CResSource (InitAttractTitle m_2c state store)
#include <Gruntz/ParseSource.h> // CParseSource::BeginParse (Creditz IsLoaded probe)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ReleaseResources / LoadCreditz keys)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (InitAttractTitle / LoadCreditz gates)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (was GameMode.cpp local view)
#include <DDrawMgr/DDSurface.h>           // CDDSurface (Render Draw / InitAttractTitle ShadeRect)
#include <Gruntz/GameMode.h> // CCreditsState : CState + CGMEntityList/CGMOwner/CGMSoundEntry
#include <Rez/RezMgr.h>      // RezFree (ReleaseResources video-handle teardown)
#include <Dsndmgr/GruntzSoundZ.h> // CGruntzSoundZ::CreateBank (0x138670) - credits sound-bank loader
#include <Win32.h>           // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>           // real IDirectDrawSurface (credits-scroll DC + Render input surface)
#include <rva.h>
#include <stdio.h> // sprintf (InitAttractTitle STATEZ_ATTRACT/TITLE%d keys)
// Real MFC CRgn/CGdiObject for the credits clip region (CreditsScrollSelf::m_1e8).
// GameMode.h pulled <Mfc.h>->afx.h (defines _AFX_ENABLE_INLINES); skip afxwin*.inl for
// the clang label step only (implicit-int CMenu::op==); wine cl keeps the inlines.
// See docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>

// The owning game-manager (CState::m_4, a real CGruntzMgr*) reached through the
// gamemode-local CGMOwner reduced view (same helper the sibling state TUs use).
static inline CGMOwner* Owner(CState* s) {
    return (CGMOwner*)s->m_4;
}

// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// ---------------------------------------------------------------------------
// Credits-scroll sub-object views (walked by DrawScrollingCredits / SetupTitle /
// LoadCreditz). Only the offsets they read are modeled; field names placeholder.
// ---------------------------------------------------------------------------

// The credits scroll's DirectDraw surface (prov->m_8): the game's real
// IDirectDrawSurface (<ddraw.h>). Only GetDC (slot 17, +0x44) and ReleaseDC
// (slot 26, +0x68) are used; both __stdcall with the surface as the hidden `this`.
SIZE_UNKNOWN(CreditsHdcProv);
struct CreditsHdcProv { // m_c->m_4->m_14->m_2c
    char p0[0x8];
    IDirectDrawSurface* m_8; // +0x08 the DDraw surface
};
SIZE_UNKNOWN(CreditsView4M14);
struct CreditsView4M14 {
    char p0[0x2c];
    CreditsHdcProv* m_2c; // +0x2c
};
SIZE_UNKNOWN(CreditsView4);
struct CreditsView4 {
    char p0[0x14];
    CreditsView4M14* m_14; // +0x14
};
SIZE_UNKNOWN(CreditsScrollView);
struct CreditsScrollView {
    char p0[0x4];
    CreditsView4* m_4; // +0x04
};
// The credits clip region at CreditsScrollSelf+0x1e8 is the real MFC CRgn/CGdiObject
// (vptr @+0x0, m_hObject @+0x4): the `(&m_1e8 != 0) ? m_hObject : 0` ternary at the
// SelectClipRgn site is CGdiObject::operator HRGN's null-this check verbatim.
SIZE_UNKNOWN(CreditsScrollSelf);
struct CreditsScrollSelf {
    char m_pad00[0xc];
    CreditsScrollView* m_c; // +0x0c
    char m_pad10[0x1bc - 0x10];
    u32 m_1bc;    // +0x1bc overlay timer B (unsigned countdown)
    u32 m_1c0;    // +0x1c0 overlay timer C (unsigned countdown)
    i32 m_1c4;    // +0x1c4 second-caption gate
    RECT m_src;   // +0x1c8 source caption RECT
    RECT m_dst;   // +0x1d8 scrolled caption RECT (top +0x1dc / bottom +0x1e4 scroll)
    CRgn m_1e8;   // +0x1e8 clip region (real MFC CRgn: vptr + m_hObject)
    char* m_1f0;  // +0x1f0 caption CString buffer
    u32 m_1f4;    // +0x1f4 reseed timer A (unsigned countdown)
    double m_1f8; // +0x1f8 scroll accumulator
    double m_200; // +0x200 scroll speed
};

// The "STATEZ_CREDITZ" registered object (m_2c): same Register source as CHelpState
// (FUN_0053c030). FindSet/FindSubset/Resolve/IsLoaded below are the reloc-masked
// __thiscall helpers off it / its sub-entries.
SIZE_UNKNOWN(CCreditzSubEntry);
struct CCreditzSubEntry { // a music sub-entry ("PLAY"/"MONOLITH")
    // IsLoaded @0x139960 IS CParseSource::BeginParse; cast at each call.
    char m_pad00[0xc];
    void* m_c; // +0x0c
};
SIZE_UNKNOWN(CCreditzMusicSet);
struct CCreditzMusicSet { // the looked-up "MIDIZ" set (m_2c->FindSet)
    // Resolve @0x13a000 IS CSymTab::Insert; cast at each call.
};
SIZE_UNKNOWN(CCreditzRegObj);
struct CCreditzRegObj { // the registered STATEZ_CREDITZ object (m_2c)
    // FindSoundSet @0x13a230 IS CSymTab::FindSub; cast at the call.
    // FindMusicSet @0x13bae0 IS CSymTab::ResolvePath; cast at the call.
};
SIZE_UNKNOWN(CCreditzSoundRegistry);
struct CCreditzSoundRegistry { // this->m_c->+0x28 (the LoadLevelSounds registry)
    void Install(void* set, char* szName, char* szKey); // FUN_00557ee0 __thiscall
};
SIZE_UNKNOWN(CCreditzStateCore);
struct CCreditzStateCore { // this->m_c->m_4 (the ready/init pump)
    // IsReady @0x158d20 IS CDDrawSubMgrPages::Method_158d20; cast at the call.
    // Init @0x158cb0 IS CDDrawSubMgrPages::Method_158cb0; cast at the call.
    i32 IsLoaded(); // FUN_00558bc0 __thiscall, ret BOOL (ready-3 predicate)

    char m_pad00[0x14];
    CreditsView4M14* m_14; // +0x14  DC-chain link (== CreditsView4::m_14; SetupTitle's HDC prov)
};
SIZE_UNKNOWN(CCreditzImageRoot);
struct CCreditzImageRoot { // this->m_4 points here; +0x48 is the registry
    char m_pad00[0x48];
    CGruntzSoundZ* m_48; // +0x48  credits sound-bank store (CreateBank @0x138670)
};
SIZE_UNKNOWN(CCreditzSoundMgr);
struct CCreditzSoundMgr { // this->m_c points here
    char m_pad00[0x4];
    CCreditzStateCore* m_4; // +0x04
    char m_pad08[0x28 - 0x8];
    CCreditzSoundRegistry* m_28; // +0x28
};
SIZE_UNKNOWN(CCreditzRegSet);
struct CCreditzRegSet { // this->m_8 points here
    // Register @0x13c030 IS CSymParser::ResolvePath; cast at the call.
};
// Typed view of `this`: m_4 the image-registry root, m_8 the namespace registry,
// m_c the sound/state manager, m_2c the registered STATEZ_CREDITZ object.
SIZE_UNKNOWN(CCreditzOwner);
struct CCreditzOwner {
    char m_pad00[0x4];
    CCreditzImageRoot* m_4; // +0x04
    CCreditzRegSet* m_8;    // +0x08
    CCreditzSoundMgr* m_c;  // +0x0c
    char m_pad10[0x2c - 0x10];
    CCreditzRegObj*
        m_2c; // +0x2c  STATEZ_CREDITZ object (dispatched as CSymTab: Insert/FindSub/ResolvePath)
    char m_pad30[0x1b4 - 0x30];
    i32 m_1b4; // +0x1b4
    i32 m_1b8; // +0x1b8
    i32 m_1bc; // +0x1bc
    i32 m_1c0; // +0x1c0
    i32 m_1c4; // +0x1c4
    // SetupTitle's credits-scroll fields (== the CreditsScrollSelf view's m_src/m_dst/
    // m_1e8/m_1f0/m_1f4/m_1f8/m_200).
    RECT m_scrollRect; // +0x1c8  scroll caption rect (SetRect target)
    RECT m_textRect;   // +0x1d8  DrawTextA measure rect
    CRgn m_clipRgn;    // +0x1e8  clip region (real MFC CRgn; Attach)
    CString m_text;    // +0x1f0  credits caption CString (m_pszData read as LPCTSTR)
    i32 m_1f4;         // +0x1f4  reseed timer A
    double m_1f8;      // +0x1f8  scroll accumulator (reset to 0.0)
    double m_200;      // +0x200  scroll step (reseeded)
    char m_pad208[0x20c - 0x208];
    i32 m_20c;         // +0x20c
    i32 SetupTitle();  // RVA 0x39a60 __thiscall (credits title/scroll setup)
    i32 FinishState(); // RVA 0x439c40 __thiscall
    // LoadGameAssetNamespaces (0xf9ea0) is inherited from CState; call it on `this`
    // (CCreditsState), not on this CCreditzOwner facet view.
};

// InitAttractTitle sub-object views: the menu page brightness holders + surface.
SIZE_UNKNOWN(CMenuBrightHolder);
struct CMenuBrightHolder {
    char m_pad00[0x2c];
    CDDSurface* m_2c; // +0x2c
};
SIZE_UNKNOWN(CMenuPageA);
struct CMenuPageA {
    char m_pad00[0x14];
    CMenuBrightHolder* m_14; // +0x14 title brightness holder
    CMenuBrightHolder* m_18; // +0x18 menu brightness holder
};
SIZE_UNKNOWN(CMenuRootA);
struct CMenuRootA {
    char m_pad00[0x4];
    CMenuPageA* m_04; // +0x04
};
// The CButeMgr text-config singleton (same 0x6453d8 datum as g_buteMgr) + the
// attract-state count divisor. TU-local views; both reloc-mask.
SIZE_UNKNOWN(CButeCfg);
struct CButeCfg {};
DATA(0x002453d8)
extern CButeCfg g_buteCfg;
extern "C" i32 g_attractStateCount;
// g_gameReg (*0x24556c) viewed for its +0x80 attract frame counter (InitAttractTitle
// rotates the TITLE index off it). Reloc-masked; the 0x24556c canonical-type unification
// is a separate worklist item.
SIZE_UNKNOWN(CCreditzAttractReg);
struct CCreditzAttractReg {
    char m_pad00[0x80];
    i32 m_80; // +0x80  attract frame counter (title rotation source)
};
extern "C" CCreditzAttractReg* g_gameReg;

// StepVideo draw views: the credits draw chain (m_c->m_4) + the Smacker frame step.
// The Smacker frame-step wrapper (FUN_0057c8e0): __stdcall(handle, frame); ret nonzero
// while more frames remain (PTR__SmackGoto@8). Reloc-masked.
extern "C" i32 __stdcall Eng_SmackStep(void* handle, i32 frame);
SIZE_UNKNOWN(CCreditsSurface);
struct CCreditsSurface {
    char m_pad00[0x8];
    void* m_8; // +0x08  Smacker frame buffer (SmackStep arg)
};
SIZE_UNKNOWN(CCreditsDrawHolder);
struct CCreditsDrawHolder {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  clip RECT (address taken)
    char m_pad20[0x2c - 0x20];
    CDDSurface* m_2c; // +0x2c  the DD surface
};
SIZE_UNKNOWN(CCreditsDrawView);
struct CCreditsDrawView {
    char m_pad00[0x14];
    CCreditsDrawHolder* m_14; // +0x14  source surface holder
    CCreditsDrawHolder* m_18; // +0x18  dest surface holder
};
SIZE_UNKNOWN(CCreditsDrawRoot);
struct CCreditsDrawRoot {
    char m_pad00[0x4];
    CCreditsDrawView* m_4; // +0x04
};

extern "C" i32 g_62bf74; // clip-region enable gate

// The credits-scroll reseed constants (0x5e96f8 / 0x5e96f0 / 0x5e9708 in retail's
// .rdata). They were `extern double g_5e96f8;`-style DECLARATIONS with no definition
// anywhere - fabricated symbols, guaranteed unresolved externals. They are ordinary
// file-scope constants: MSVC5 keeps a `const double` in memory (it does NOT fold FP
// constant expressions the way it folds integer ones), so `kScreenH / kScrollRate`
// still lowers to retail's `fld QWORD PTR [c1]; fdiv QWORD PTR [c2]; call __ftol`.
static const double kScreenH = 480.0;     // 0x5e96f8  screen height
static const double kScrollRate = 0.025;  // 0x5e96f0  scroll rate
static const double kStepScale = 1000.0;  // 0x5e9708  scroll-step scale (m_200 reseed)

// ===========================================================================
// CCreditsState methods, ascending retail-RVA order.
// ===========================================================================

// @confidence: high
// @source: decomp-xref
// CCreditsState::LoadCreditzStateAssets (0x38d20). Byte-exact (100%). int (BOOL) return
// like its loader siblings; the literal `return 0;` keeps the opening/Init guards as
// `test eax,eax`. The MONOLITH block is a SIBLING `if(midiz)` so the second
// `cmp edi,ebp; je` survives (docs/patterns/redundant-sibling-guard-retest.md). The
// 'IMX' music tag (0x584d49) is a non-relocated immediate. The "STATEZ_CREDITZ" Register
// is the CHelpState::LoadAssets source (FUN_0053c030).
RVA(0x00038d20, 0x176)
i32 CCreditsState::LoadCreditzStateAssets(i32 a1, i32 a2, i32 a3) {
    CCreditzOwner* self = (CCreditzOwner*)this;

    if (!LoadGameAssetNamespaces(a1, a2, a3)) { // inherited CState method on `this`
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;

    self->m_1b8 = 0;
    self->m_1bc = 0;
    self->m_1c0 = 0;
    self->m_1c4 = 0;
    self->m_2c = (CCreditzRegObj*)((CSymParser*)self->m_8)->ResolvePath("STATEZ_CREDITZ");
    if (!self->m_2c) {
        return 0;
    }

    void* sounds = ((CSymTab*)self->m_2c)->FindSub("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->ScanTree_157ee0((DirNode*)sounds, "CREDITZ", "_");

    CCreditzMusicSet* midiz = (CCreditzMusicSet*)((CSymTab*)self->m_2c)->ResolvePath("MIDIZ");
    if (midiz) {
        CCreditzSubEntry* e = (CCreditzSubEntry*)((CSymTab*)midiz)->Insert("PLAY", (void*)0x584d49);
        if (e) {
            i32 val = ((CParseSource*)e)->BeginParse();
            if (val) {
                self->m_4->m_48->CreateBank((void*)val, (u32)e->m_c, "CREDITZ"); // 0x138670
            }
        }
    }
    // Sibling re-test (not nested): retail re-emits `cmp edi,ebp; je` for the
    // MONOLITH block, pinning midiz in edi across the PLAY calls
    // (docs/patterns/redundant-sibling-guard-retest.md).
    if (midiz) {
        CCreditzSubEntry* e2 =
            (CCreditzSubEntry*)((CSymTab*)midiz)->Insert("MONOLITH", (void*)0x584d49);
        if (e2) {
            i32 val = ((CParseSource*)e2)->BeginParse();
            if (val) {
                self->m_4->m_48->CreateBank((void*)val, (u32)e2->m_c, "MONOLITH"); // 0x138670
            }
        }
    }

    if (!((CDDrawSubMgrPages*)self->m_c->m_4)->Method_158d20()) {
        if (!((CDDrawSubMgrPages*)self->m_c->m_4)->Method_158cb0(0, 0x30000)) {
            return 0;
        }
    }

    self->SetupTitle();
    self->m_20c = 2;
    // FinishState is CCreditsState's own method (0x39c40), not CCreditzOwner's - call
    // it on `this` (same pointer as self) so the rel32 ties to the real body.
    i32 r = FinishState();
    self->m_1b4 = 0;
    return r;
}

// @confidence: high
// @source: decomp-xref
// CCreditsState::ReleaseResources() (0x38f00): if (m_c) free the pooled resource then
// release the three named registries ("CREDITZ"); then tear down + RezFree the video
// handle (m_videoHandle) and chain BaseCleanup. m_c is re-read for each access.
RVA(0x00038f00, 0x87)
void CCreditsState::ReleaseResources() {
    if (m_c) {
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            ((SoundStream*)r)->Stop();
        }
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("CREDITZ", "_");
        ((CDDrawWorkerRegistry*)m_c->m_10)->RemoveKeysEqual_155360("CREDITZ", "_");
        ((CDDrawWorkerRegistry*)m_c->m_animRegistry)->RemoveKeysEqual_155360("CREDITZ", "_");
    }
    // Cache the video handle in a local so it stays pinned in edi across the
    // Teardown call (retail reuses the same register for the RezFree push).
    CMoviePlayer* vh = m_videoHandle;
    if (vh) {
        vh->~CMoviePlayer();
        RezFree(vh);
        m_videoHandle = 0;
    }
    ((CGameModeBase*)this)->BaseCleanup();
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
    IDirectDrawSurface* in = m_c->m_drawTarget->m_10->m_2c->m_8;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            Owner(this)->Post(0x8006, 0xfa0);
            return 0;
        }
    }

    if (((CSoundRegistry*)m_c->m_28)->m_2c) {
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
    CDrawTarget* v4 = m_c->m_drawTarget;
    v4->m_10->m_2c->Draw(0);
    v4->m_14->Blit((i32)v4->m_18);

    if (!m_1b4 && Owner(this)->m_14) {
        Owner(this)->m_48->Play(g_60ce90, 1);
        m_1b4 = 1;
    }

    if (m_1c4) {
        i32 s = Owner(this)->m_48->Find(g_60ce74);
        if (s && !((CGMSoundEntry*)s)->Query()) {
            Sub3();
        }
    }
    return 1;
}

// CCreditsState::InputVirtual (slot 8 / +0x20, @0x393b0, formerly ShowAttractTitle) -
// the per-frame input poll: gate on the state core (m_c->m_4->IsLoaded); if loaded,
// force the cursor hidden then prime the attract title. Returns 1 (0 when not loaded).
RVA(0x000393b0, 0x3a)
i32 CCreditsState::InputVirtual() {
    CCreditzOwner* self = (CCreditzOwner*)this;
    // the page pump at m_c->m_4 is CDDrawSubMgrPages; the ready gate is Method_158bc0
    // (0x158bc0) - NOT CParseSource::BeginParse (0x139960); same m_c->m_4 page-gate the
    // sibling states (CHelpState/CSplashState) poll.
    if (((CDDrawSubMgrPages*)self->m_c->m_4)->Method_158bc0() == 0) {
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
// the mgr frame counter, format the "STATEZ_ATTRACT"/"TITLE%d" keys, resolve the attract
// state into m_2c, fade the title in, apply the configured brightness, transition the
// page, and build the menu page.
// @early-stop
// 81.2%: logic byte-faithful (the twin of CAttract::LoadTitleConfig). Residual is the
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md)
// on the FadeInTitle fail return-0 + the sprintf stack-buffer slot layout. Not steerable.
RVA(0x00039570, 0x122)
i32 CCreditsState::InitAttractTitle() {
    CMenuRootA* root = (CMenuRootA*)m_c;
    if (m_videoPlaying != 0) {
        ((CDDrawSubMgrPages*)root->m_04)->Method_158dc0();
        ((CDDrawSubMgrPages*)root->m_04)->Method_158e90();
        ((CDDrawSubMgrPages*)root->m_04)->Method_158d50(0);
        root->m_04->m_18->m_2c->Fill(0);
        return 1;
    }
    char stateName[0x20];
    char titleName[0x20];
    i32 idx = g_gameReg->m_80 % g_attractStateCount + 1;
    sprintf(stateName, "STATEZ_ATTRACT");
    sprintf(titleName, "TITLE%d", idx);
    void* saved = (void*)m_2c;
    void* state = ((CSymParser*)m_8)->ResolvePath(stateName);
    m_2c = (CResSource*)state;
    if (state == 0) {
        return 0;
    }
    i32 faded = FadeInTitle(titleName, 0, 0, 1, 0, 0);
    m_2c = (CResSource*)saved;
    if (faded == 0) {
        return 0;
    }
    CDDSurface* tgt = root->m_04->m_14->m_2c;
    tgt->ShadeRect(((CButeMgr*)&g_buteCfg)->GetIntDef("Menu", "BrightnessPercent", 0x32), 0);
    ((CDDrawSubMgrPages*)root->m_04)->Method_158e90();
    BuildMenuPage(0x50, 0x3e8, 0, 1);
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
    CreditsScrollSelf* self = (CreditsScrollSelf*)this;
    if (self->m_c == 0) {
        return 0;
    }
    CreditsHdcProv* prov = self->m_c->m_4->m_14->m_2c;

    if (g_645584 >= self->m_1f4) {
        self->m_1f4 = 0;
    } else {
        self->m_1f4 -= g_645584;
    }
    if (self->m_1c4 != 0) {
        if (g_645584 >= self->m_1bc) {
            self->m_1bc = 0;
        } else {
            self->m_1bc -= g_645584;
        }
        if (g_645584 >= self->m_1c0) {
            self->m_1c0 = 0;
        } else {
            self->m_1c0 -= g_645584;
        }
    }

    self->m_dst = self->m_src;
    double contrib = (double)g_645584 * self->m_200 * 0.001;
    self->m_1f8 = self->m_1f8 + contrib;
    i32 scrolled = (i32)self->m_1f8;
    self->m_dst.top -= scrolled;
    self->m_dst.bottom -= scrolled;
    if (self->m_dst.bottom < 0) {
        self->m_1f8 = 0.0;
        self->m_dst = self->m_src;
        self->m_1f4 = (i32)(kScreenH / kScrollRate);
    }

    HDC hdc = 0;
    prov->m_8->GetDC(&hdc);
    if (hdc != 0) {
        i32 oldBk = SetBkMode(hdc, TRANSPARENT);
        if (g_62bf74 != 0) {
            SelectClipRgn(
                hdc,
                self->m_1e8
            ); // CRgn -> HRGN via CGdiObject::operator HRGN (null-this)
        }
        i32 oldColor = SetTextColor(hdc, ((CCreditsState*)self)->FlashColor());
        DrawTextA(hdc, self->m_1f0, -1, &self->m_dst, 0x50);
        SetTextColor(hdc, oldColor);
        if (self->m_1c4 != 0 && self->m_1c0 != 0) {
            CString s("Now is the time at Monolith when we dance");
            RECT r = {0, 0, 0x280, 0x1e0};
            i32 oldColor2 = SetTextColor(hdc, 0xffffff);
            DrawTextA(hdc, s, -1, &r, 0x75); // CString -> LPCTSTR (implicit)
            SetTextColor(hdc, oldColor2);
        }
        if (g_62bf74 != 0) {
            SelectClipRgn(hdc, 0);
        }
        SetBkMode(hdc, oldBk);
        prov->m_8->ReleaseDC(hdc);
    }
    return 1;
}

// CCreditzOwner::SetupTitle (0x39a60, __thiscall, ret BOOL) - build the credits scroll:
// pull the "CREDITZ" TXT section from the STATEZ_CREDITZ store (m_2c, dispatched as
// CSymTab) into the m_text CString, build the clip region, measure the text against the
// offscreen DDraw surface's HDC to set the scroll rect, then seed the scroll accumulator.
// @early-stop
// scheduling tail (~99.7%): for the final (double)(unsigned)m_1f4 conversion the int64
// temp's two halves are emitted in the opposite order relative to the fld/fmul. All other
// bytes identical (llvm-objdump -dr). GetDC/ReleaseDC region is byte-exact via the real
// IDirectDrawSurface COM slots.
RVA(0x00039a60, 0x179)
i32 CCreditzOwner::SetupTitle() {
    // CSymTab::Insert resolves the "CREDITZ" section of FOURCC type 'TXT'
    // (== 0x545854, a tag value not an address); returns the section CParseSource as H.
    CParseSource* sect = (CParseSource*)((CSymTab*)m_2c)->Insert("CREDITZ", (void*)'TXT');
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
        m_text = buf;
        sect->EndParse();
        operator delete(buf);
    }
    m_clipRgn.Attach(CreateRectRgn(0x32, 0, 0x24e, 0x1e0));
    CreditsHdcProv* prov = m_c->m_4->m_14->m_2c;
    HDC hdc = 0;
    prov->m_8->GetDC(&hdc);
    if (hdc) {
        i32 h = DrawTextA(hdc, m_text, -1, &m_textRect, 0x450);
        SetRect(&m_scrollRect, 0x32, 0x1e0, 0x24e, h + 0x1e0);
        prov->m_8->ReleaseDC(hdc);
    }
    m_1f8 = 0.0;
    m_1f4 = (i32)(kScreenH / kScrollRate);
    m_200 = (kScreenH * kStepScale) / (double)(unsigned)m_1f4;
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
        CCreditsDrawView* v = ((CCreditsDrawRoot*)m_c)->m_4;
        CCreditsDrawHolder* dst = v->m_18;
        CCreditsDrawHolder* src = v->m_14;
        if (!Eng_SmackStep(dst->m_2c->m_8, -1)) {
            m_videoHandle->CloseSmacker();
            ret = FinishState();
        }
        if (dst && src) {
            src->m_2c->BltFast(0, 0, dst->m_2c, &dst->m_1c, 0x10);
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

// GameMode.cpp - the game-state ("mode") hierarchy the per-frame tick drives.
// See GameMode.h for the hierarchy + the headline finding. Names are placeholders;
// only offsets + code bytes are load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CState::CState()          99.86% - ctor (scalar zero/seed)
//   CState::~CState() (??_G)  98.75% - slot-0 scalar-deleting dtor
//   CState::Update()         100.00% - slot 4  return 1;
//   CState::Render()         100.00% - slot 5  return 1;
//   CMenuState::Update()     100.00% - return 5;
//   CCreditsState::Update()  100.00% - return 8;
//   CBootyState::Update()    100.00% - return 0xa;
//   CMenuState::Render()     BYTE-EXACT 99.43% (all
//       residuals reloc-masked) - the front-end per-frame menu draw.
//   CCreditsState::Render()  97.17% plateau - the
//       per-frame credits draw (residuals: reloc-masked operands + one register
//       coin-flip in the cursor-anim chain + the wParam select's mov-vs-push
//       codegen form; see the body comments).
// The <100% leaves are reloc-masked operands only: the ctor's lone diff is the
// vtable DIR32 store; the dtor's three are vtable DIR32 + base-cleanup REL32 +
// op-delete REL32 (instruction sequences are byte-identical vs dump_target.py).
//
// TWO LEVERS: (1) the base cleanup is __thiscall - modeled as a method
// on a helper struct so the dtor tail-call emits NO `add esp,4`. (2) define the
// dtor body INLINE in the header so MSVC folds the vtable-restore + base cleanup
// directly into the synthesized `??_G` (the target inlines, not `call ??1`).
//
// THE STATE-ID FINDING: the per-frame tick (RezMgr::PerFrameTick) calls m_mode's
// slot +0x10 (Update) and gates timing on `!= 0x11`; each concrete state's Update
// returns a fixed small enum tag (1/3/5/8/0xa = base/play/menu/credits/booty),
// i.e. slot +0x10 is the "which state am I" query, NOT the simulation step. The
// real per-frame step+draw is slot +0x14 (Render), overridden by each concrete
// state (carcassed in the long comment at the bottom of this file).
#include <Gruntz/GameMode.h>
#include <rva.h>

// ===========================================================================
// CState - the base game-state class.
// ===========================================================================

// CState::CState(): store the vftable, then zero a flat list of
// scalar members in source-declaration order, seeding four time/budget fields to
// 0x40. NO embedded sub-object ctors and NO EH frame (plain /O2 - the ctor uses
// eax=this, edx=0x40, ecx=0 held registers).
RVA(0x0008c750, 0xa9)
CState::CState() {
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_28 = 0;
    m_2c = 0;
    m_14 = 0;
    m_18 = 0;
    m_38 = 0;
    m_3c = 0;
    m_4c = 0;
    m_24 = 0;
    m_160 = 0;
    m_164 = 0;
    m_168 = 0;
    m_170 = 0x40;
    m_16c = 0;
    m_174 = 0x40;
    m_178 = 0;
    m_180 = 0x40;
    m_17c = 0;
    m_184 = 0x40;
    m_188 = 0;
    m_190 = 0;
    m_18c = 0;
    m_194 = 0;
    m_198 = 0;
    m_1a0 = 0;
    m_19c = 0;
    m_1a4 = 0;
    m_150 = 0;
    m_154 = 0;
}

// CState::~CState()  - the slot-0 scalar-deleting dtor `??_G`. Restore the
// vftable, chain the WAP32 base cleanup, then (if the low
// bit of the hidden flags arg is set) `operator delete(this)`. The dtor body is
// defined INLINE in the header so MSVC folds it into the synth `??_G` thunk (the
// target inlines it; see GameMode.h). This thunk has no source body, so it cannot
// carry an RVA() attribute - pin the deleting-dtor symbol by mangled name here.
// @rva-symbol: ??_GCState@@UAEPAXI@Z 0x0008c710 0x24

// CState::Update()  (slot 4 / +0x10): the base default = return 1.
RVA(0x0008c4b0, 0x6)
int CState::Update() {
    return 1;
}

// CState::Render()  (slot 5 / +0x14): the base default = return 1.
RVA(0x0008c4d0, 0x6)
int CState::Render() {
    return 1;
}

// The intervening vtable slots (1..3) - out-of-line stubs that anchor the vftable
// order so Update lands at slot 4 (+0x10) and Render at slot 5 (+0x14).
void CState::Vfunc1() {}
void CState::Vfunc2() {}
void CState::Vfunc3() {}

// ===========================================================================
// The concrete states - each overrides Update() to return its state-ID tag.
// ===========================================================================

// (CPlay::Update lives with the rest of CPlay in src/Gruntz/CPlay.cpp.)

// CMenuState::Update(): the MENU state's ID = 5.
RVA(0x0008ce10, 0x6)
int CMenuState::Update() {
    return 5;
}

// CCreditsState::Update(): the CREDITS state's ID = 8.
RVA(0x0008d590, 0x6)
int CCreditsState::Update() {
    return 8;
}

// CBootyState::Update(): the BOOTY state's ID = 0xa.
RVA(0x0008d3f0, 0x6)
int CBootyState::Update() {
    return 0xa;
}

// ===========================================================================
// The concrete Render overrides (vtable slot +0x14) - the real per-frame
// step+draw. Plain /O2 /MT: neither carries a stack C++ object / EH frame.
// ===========================================================================

// CCreditsState::Render(): the canonical Render spine.
//   1. INPUT POLL: i = m_c->m_4->m_10->m_2c->m_8; if (i && i->vtbl[+0x60](i))
//      skip the input-virtual; else
//   2. INPUT VIRTUAL: if (this->vtbl[+0x20]()) { m_4->Post(0x8006,0xfa0); return 0; }
//   3. CURSOR ANIM: if (m_c->m_28->m_2c) GM_SimpleAnim(-1);
//   4. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   5. MESSAGE SCAN: first e with (e->m_2ac & 0xffffff) -> PostMessageA(hwnd,
//      WM_COMMAND, m_24==5 ? 0x8023 : 0x8027, 0); m_4->m_8->m_244 = 0;
//   6. SUB-STEPS: Sub1(); Sub2();
//   7. DRAW: m_c->m_4->m_10->m_2c->Draw(0); m_c->m_4->m_14->Blit(m_c->m_4->m_18);
//   8. ONE-SHOT FX (+0x1b4 latch): if (!m_1b4 && m_4->m_14) {
//        m_4->m_48->Play("CREDITZ",1); m_1b4 = 1; }
//   9. CONDITIONAL FX (+0x1c4 gate): if (m_1c4) { s = m_4->m_48->Find("MONOLITH");
//        if (s && !s->Query()) Sub3(); }   return 1;
RVA(0x000391d0, 0x17c)
int CCreditsState::Render() {
    CGMInputObj* in = ((CGMView*)m_c)->m_4->m_10->m_2c->m_8;
    if (!in || in->vtbl->Poll(in)) {
        if (!InputVirtual()) {
            ((CGMOwner*)m_4)->Post(0x8006, 0xfa0);
            return 0;
        }
    }

    if (((CGMView*)m_c)->m_28->m_2c) {
        GM_SimpleAnim(-1);
    }

    // per-entity Update pass
    {
        CGMEntityList* L = g_645574;
        for (int i = 0; i < L->m_count; i++) {
            L->m_elems[i]->Update();
        }
    }

    // message scan: first flagged entity posts a WM_COMMAND
    {
        CGMEntityList* L = g_645574;
        int n = L->m_count;
        for (int j = 0; j < n; j++) {
            if (L->m_elems[j]->m_2ac & 0xffffff) {
                // wParam = (m_24==5) ? 0x8023 : 0x8027. MSVC 5.0 /O2 branchless-
                // collapses an inline `?:` of these (sub/neg/sbb/and 4/add); the
                // init+conditional-override below keeps the cmp+jne branch (the
                // target's push-per-branch is the lazy `?:` form MSVC won't emit
                // when both arms fold to a 4-apart constant - irreducible).
                unsigned wp = 0x8027;
                if (m_24 == 5) {
                    wp = 0x8023;
                }
                PostMessageA(((CGMOwner*)m_4)->m_4->m_4, 0x111, wp, 0);
                ((CGMOwner*)m_4)->m_8->m_244 = 0;
                break;
            }
        }
    }

    Sub1();
    Sub2();

    // draw: cache m_c->m_4 (the target keeps it in esi for the three derefs).
    CGMView::M4* v4 = ((CGMView*)m_c)->m_4;
    v4->m_10->m_2c->Draw(0);
    v4->m_14->Blit((int)v4->m_18);

    if (!m_1b4 && ((CGMOwner*)m_4)->m_14) {
        ((CGMOwner*)m_4)->m_48->Play(g_60ce90, 1);
        m_1b4 = 1;
    }

    if (m_1c4) {
        int s = ((CGMOwner*)m_4)->m_48->Find(g_60ce74);
        if (s && !((CGMSoundEntry*)s)->Query()) {
            Sub3();
        }
    }
    return 1;
}

// CMenuState::Render(): the front-end per-frame menu draw.
//   1. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   2. SIX entity-flag scans (masks 0x80000000/0x40000000/0x20000000/0x10000000/
//      0x3 (byte)/0x100): the FIRST scan that finds a flagged entity fires the
//      matching no-arg method on the UI object m_1b4 and short-circuits to the
//      tail; the 0x100 scan, if its handler returns 0, also posts a WM_COMMAND
//      0x8036 before the tail.
//   3. TAIL: m_1b4->Step(g_645584); m_1b4->Pre(); DrawVersion({g_645cc8..d4});
//      m_1b4->Post();   return 1;
RVA(0x000a0750, 0x1d0)
int CMenuState::Render() {
    CGMEntityList* L = g_645574;

    // per-entity Update pass (re-reads count each iter, like the target)
    for (int i = 0; i < L->m_count; i++) {
        L->m_elems[i]->Update();
    }

    // six prioritized entity-flag scans, each firing a distinct UI handler
    int c;
    L = g_645574;
    int n = L->m_count;
    for (c = 0; c < n; c++) {
        if ((unsigned)L->m_elems[c]->m_2ac & 0x80000000) {
            m_1b4->OnFlag80000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((unsigned)L->m_elems[c]->m_2ac & 0x40000000) {
            m_1b4->OnFlag40000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((unsigned)L->m_elems[c]->m_2ac & 0x20000000) {
            m_1b4->OnFlag20000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((unsigned)L->m_elems[c]->m_2ac & 0x10000000) {
            m_1b4->OnFlag10000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_elems[c]->m_2ac & 0x3) {
            m_1b4->OnFlag00000003();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_elems[c]->m_2ac & 0x100) {
            if (!m_1b4->OnFlag00000100()) {
                PostMessageA(((CGMOwner*)m_4)->m_4->m_4, 0x111, 0x8036, 0);
            }
            goto tail;
        }
    }
tail:
    m_1b4->Step(g_645584);
    m_1b4->Pre();
    DrawVersion(g_645cc8);
    m_1b4->Post();
    return 1;
}

// InputVirtual (slot 8 / +0x20) is the per-frame input poll the Render does an
// indirect `this->vtbl[+0x20]()` to (its body is irrelevant to the Render match -
// only the indirect call site is). Out-of-line so the CCreditsState vtable
// resolves; NOT a byte-matched target. (Slots 6,7 are inherited from CState.)
int CCreditsState::InputVirtual() {
    return 0;
}

// ===========================================================================
// REMAINING Render overrides (slot +0x14) NOT matched here (deferred targets):
//   CBootyState::Render - the bonus-state per-frame draw.
//   CPlay::Render       - the in-game per-frame heart (the
//       carcass is reconstructed in the `cplay` WIP unit, src/Gruntz/CPlay.cpp).
// Both follow the same spine the two matched Renders above show (per-entity
// Update loop over g_645574 -> entity-flag message scans -> draw/UI step),
// scaled up with the level/grunt simulation.
//
// CState member offsets the Render path pins (beyond the ctor's): +0x4 (the
// owner back-ptr -> +0x4->+0x4 = HWND), +0xc (the view/input sub-object holder),
// +0x24 (a state-discriminator: ==5 selects WM 0x8023 vs 0x8027), and the
// subclass FX state at +0x1b4 (CCreditsState one-shot-FX latch / CMenuState UI
// object) + +0x1c4 (CCreditsState conditional-FX gate). The global is a
// POINTER to the per-frame entity list (count@+4, elem-ptr array@+8) that every
// state Render iterates (e->vtbl[+0x10] = per-entity Update; e->m_2ac = flags).

// -------------------------------------------------------------------------
// Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
// -------------------------------------------------------------------------
// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001d440, 0xd7d)
void CBootyState::vfunc_1() {}

// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x0008d5e0, 0x8b)
void CCreditsState::Stub_08d5e0() {}

// @confidence: low
// @source: winapi:SelectClipRgn;SetBkMode
// @stub
RVA(0x000396f0, 0x2b8)
int CCreditsState::winapi_0396f0_SelectClipRgn_SetBkMode() {
    return 0;
}

// -------------------------------------------------------------------------
// Re-homed __thiscall behavioral methods (relocated from src/Stub/).
// -------------------------------------------------------------------------

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00019540, 0x12a)
void CState::BuildWarpStoneGlitterAnimation() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001a040, 0x55e)
void CState::LoadGruntEffectSprites() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001b450, 0x1ac)
void CState::BuildBootyWalkingGruntz() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001c210, 0x4b5)
void CBootyState::CheckWarpLetterBonus() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000a0d80, 0xd7)
void CMenuState::BuildVersionString(int, int, int, int) {}

// The "STATEZ_CREDITZ" registered object (m_2c): same Register source as
// CHelpState (FUN_0053c030). FindSet/FindSubset/Resolve/IsLoaded below are the
// reloc-masked __thiscall helpers off it / its sub-entries.
struct CCreditzSubEntry { // a music sub-entry ("PLAY"/"MONOLITH")
    int IsLoaded();       // FUN_00539960 __thiscall, ret BOOL/value
    char m_pad00[0xc];
    void* m_c; // +0x0c
};
struct CCreditzMusicSet { // the looked-up "MIDIZ" set (m_2c->FindSet)
    // FUN_0053a000 __thiscall: resolve a named sub-entry under a packed tag.
    CCreditzSubEntry* Resolve(char* szName, int tag);
};
struct CCreditzRegObj {               // the registered STATEZ_CREDITZ object (m_2c)
    void* FindSoundSet(char* szName); // FUN_0053a230 __thiscall, ret set ptr
    void* FindMusicSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};
struct CCreditzSoundRegistry { // this->m_c->+0x28 (the LoadLevelSounds registry)
    void Install(void* set, char* szName, char* szKey); // FUN_00557ee0 __thiscall
};
struct CCreditzImageRegistry { // this->m_4->+0x48
    // FUN_00538670 __thiscall: install a resolved sub-entry under a name.
    void Install3(void* res, void* host, char* szName);
};
struct CCreditzStateCore {      // this->m_c->m_4 (the ready/init pump)
    int IsReady();              // FUN_00558d20 __thiscall, ret BOOL
    int Init(int a, int flags); // FUN_00558cb0 __thiscall, ret BOOL
};
struct CCreditzImageRoot { // this->m_4 points here; +0x48 is the registry
    char m_pad00[0x48];
    CCreditzImageRegistry* m_48; // +0x48
};
struct CCreditzSoundMgr { // this->m_c points here
    char m_pad00[0x4];
    CCreditzStateCore* m_4; // +0x04
    char m_pad08[0x28 - 0x8];
    CCreditzSoundRegistry* m_28; // +0x28
};
struct CCreditzRegSet {                   // this->m_8 points here
    CCreditzRegObj* Register(char* name); // FUN_0053c030 __thiscall (CHelpState idiom)
};
// Two owner methods reached at the tail, both __thiscall(this) no args:
// the title/cursor setup (RVA 0x39a60) and the state-finish (0x439c40).

// Typed view of `this`: m_4 the image-registry root, m_8 the namespace registry,
// m_c the sound/state manager, m_2c the registered STATEZ_CREDITZ object.
struct CCreditzOwner {
    char m_pad00[0x4];
    CCreditzImageRoot* m_4; // +0x04
    CCreditzRegSet* m_8;    // +0x08
    CCreditzSoundMgr* m_c;  // +0x0c
    char m_pad10[0x2c - 0x10];
    CCreditzRegObj* m_2c; // +0x2c
    char m_pad30[0x1b4 - 0x30];
    int m_1b4; // +0x1b4
    int m_1b8; // +0x1b8
    int m_1bc; // +0x1bc
    int m_1c0; // +0x1c0
    int m_1c4; // +0x1c4
    char m_pad1c8[0x20c - 0x1c8];
    int m_20c;                                  // +0x20c
    void SetupTitle();                          // RVA 0x39a60 __thiscall
    int FinishState();                          // RVA 0x439c40 __thiscall
    int LoadGameAssetNamespaces(int, int, int); // base loader; reloc-masked near call
};

// @confidence: high
// @source: decomp-xref
// Byte-exact (100%). int (BOOL) return like its loader siblings: the early
// guards `return 0` (each reusing the just-loaded/zeroed eax via `test eax,eax`),
// and the success tail returns FinishState()'s result unmodified - a `void`
// return would tail-merge the bare epilogues. The literal `return 0;` (not
// `return loaded;`) is load-bearing: it keeps the opening/Init guards as
// `test eax,eax` and lets cl defer `xor ebp,ebp` to where retail materializes it.
// The MONOLITH block is a SIBLING (not nested) `if(midiz)` so the second
// `cmp edi,ebp; je` survives (docs/patterns/redundant-sibling-guard-retest.md).
// The 'IMX' music tag (0x584d49) is a non-relocated immediate. The
// "STATEZ_CREDITZ" Register is the CHelpState::LoadAssets source (FUN_0053c030).
RVA(0x00038d20, 0x176)
int CCreditsState::LoadCreditzStateAssets(int a1, int a2, int a3) {
    CCreditzOwner* self = (CCreditzOwner*)this;

    if (!self->LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;

    self->m_1b8 = 0;
    self->m_1bc = 0;
    self->m_1c0 = 0;
    self->m_1c4 = 0;
    self->m_2c = self->m_8->Register("STATEZ_CREDITZ");
    if (!self->m_2c) {
        return 0;
    }

    void* sounds = self->m_2c->FindSoundSet("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    self->m_c->m_28->Install(sounds, "CREDITZ", "_");

    CCreditzMusicSet* midiz = (CCreditzMusicSet*)self->m_2c->FindMusicSet("MIDIZ");
    if (midiz) {
        CCreditzSubEntry* e = midiz->Resolve("PLAY", 0x584d49);
        if (e) {
            int val = e->IsLoaded();
            if (val) {
                self->m_4->m_48->Install3((void*)val, e->m_c, "CREDITZ");
            }
        }
    }
    // Sibling re-test (not nested): retail re-emits `cmp edi,ebp; je` for the
    // MONOLITH block, pinning midiz in edi across the PLAY calls
    // (docs/patterns/redundant-sibling-guard-retest.md).
    if (midiz) {
        CCreditzSubEntry* e2 = midiz->Resolve("MONOLITH", 0x584d49);
        if (e2) {
            int val = e2->IsLoaded();
            if (val) {
                self->m_4->m_48->Install3((void*)val, e2->m_c, "MONOLITH");
            }
        }
    }

    if (!self->m_c->m_4->IsReady()) {
        if (!self->m_c->m_4->Init(0, 0x30000)) {
            return 0;
        }
    }

    self->SetupTitle();
    self->m_20c = 2;
    int r = self->FinishState();
    self->m_1b4 = 0;
    return r;
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00039570, 0x122)
void CCreditsState::InitAttractTitle() {}

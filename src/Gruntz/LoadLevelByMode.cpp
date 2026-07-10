// LoadLevelByMode.cpp - the PLAY game-state per-mode level loader
// (C:\Proj\Gruntz). A /GX EH-framed megafunction (3636 B) on the PLAY-state
// object (`this`, a CPlay/CLevelValidator-faceted level host): given a level id
// it tears down the previous level, resolves the level's area page and display
// name, then runs the long linear init chain that brings up a level (the
// BATTLEZ / MULTI / CUSTOMLEVEL / TRAINING / Level%i variants), wiring the map,
// sound, palette, cursor-snap sprite, status bars and the loading screen.
//
// Reconstructed with the REAL retail ABI (was a cdecl/void* carcass at 34.8%):
//   * `this` is the PLAY-state object; its init-chain steps are __thiscall CPlay
//     methods (BuildHelpReveal 0x1019, RegisterInputBindings 0x3a71,
//     LoadActionTileSprites 0x2dce, ScanBuildTiles 0x3553, AddLevelGruntz 0x17ee,
//     ValidateLevelTiles 0x345e (CLevelValidator facet), ...) reached via
//     `mov ecx,esi; call`. The four `[edx+0x84/0xa4/0xa8/0x10]` slots stay manual
//     vtable dispatch (the emitted `mov edx,[esi]; call [edx+X]`).
//   * m_4 is a CGruntzMgr; GetWorldFileName (0x2531) returns a CString BY VALUE
//     (hidden ret-slot ptr -> the /GX EH frame's [esp+0x14] temp). Insert
//     (CSymTab 0x13a000), BeginParse/EndParse (CParseSource 0x139960/0x1399d0) and
//     ResolvePath (CSymParser 0x13c030) are __thiscall on the resolved set.
//   * genuine __cdecl helpers: Cmd_ResetScroll (0x2bd0), EngStr_DrawText (0x1c5d),
//     QueryToken (0x39a4), sprintf/atoi/srand/new/delete.
//   * the WarpStone scan / Level%i loop / loading screen each build a real
//     block-scoped CString, so cl emits the ctor(inline)/dtor(0x1b9cde)/state
//     stores that give the exception frame.
//
// IDENTITY (xref-recovered) + DEFERRED FOLD (blocked by cross-TU ownership):
//   * `this` is the canonical CPlay (<Gruntz/Play.h>, `class CPlay : public CState`,
//     matcher-4's `play` unit) - proven by the __thiscall init-chain leaves all
//     being CPlay methods (BuildHelpReveal 0x1019, LoadWarlordSprites 0x2b80,
//     ScanBuildTiles 0x3553, AddLevelGruntz 0x17ee, BuildWarlordNameTable 0x38dc,
//     ScanShuffleQuads 0x28dd, LoadActionTileSprites 0x2dce) PLUS
//     CLevelValidator::ValidateLevelTiles (0x345e) on the SAME `this` at offset 0
//     (the inheritance the class shape encodes). `CPlayLevelLoad` here is a FAKE
//     VIEW of CPlay: homing LoadByMode/LoadPyramidBridge onto the real CPlay needs
//     their method decls added to Play.h + the +0xa4/+0xa8/+0x84/+0x10 vtable-slot
//     signatures there corrected (slot41 BuildMusicCategoryTable / slot42
//     BuildWorldLevelPath are modeled no-arg but retail passes 1 arg). That edits
//     matcher-4's Play.h -> DEFERRED cross-TU fold, reported (not a wall).
//   * `Eng` is a declared-only carcass that CONFLATES the xref-recovered sub-object
//     classes (m_4 = CGruntzMgr <Gruntz/GruntzMgr.h>: GetWorldFileName/SyncOptionsState/
//     RecomputeViewScale/CheckSavedMode; set = CSymParser/CSymTab/CParseSource;
//     ctx = CLightFxRender; m_3f4 = CTimer; m_2dc = CSBI_RectOnly; CImageSet3;
//     CButeMgr; CBattlezData; CAreaMgr; CSaveGame; CFontConfig; CBrickz; CDDSurface).
//     The calls are reloc-masked so the view name is byte-irrelevant; the fold to
//     the real per-class canonical headers is DEFERRED (retype + header method
//     decls, cross-TU). Only offsets / code bytes are load-bearing. Returns int;
//     the retail `ret 8` implies a second (unused) param.
//
// @early-stop  (34.8% -> ~78.8%: rebuilt the real retail ABI - the __thiscall
// engine-helper convention (was cdecl push/add-esp), the /GX EH frame from real
// CString temps + the CString-by-value GetWorldFileName return (was a void*
// carcass with no frame), the polymorphic +0x10/+0x84/+0xa4/+0xa8 vtable slots
// (was cdecl fn-ptr calls), the ((level-1)%0x24)/4+1 area math (was +q), the
// self->m_c grid teardown (was m_4), the 0xb4 nameBuf frame slot, and the
// warp-cache reload flag -> the +0xa4 arg, so the frame/prologue/calls align.)
// Residual is the documented /GX regalloc + EH-state wall: MSVC5 pins a fresh
// `xor eax,eax` zero where our cl reuses the edi=0 register, reloads `this`
// (g_gameReg) into edx vs ecx, and numbers the EH scope-table states across the
// CString temps differently - a systematic register/schedule rename across the
// 3636-byte frame, not steerable from C source (docs/patterns/
// eh-state-numbering-base.md; big-seh-fuzzy-desync.md; zero-register-pinning.md;
// o2-optimizer-bailout-framed.md).

#include <Mfc.h> // CString (the /GX EH temps) + PtInRect/UpdateWindow/ShowCursor

#include <Bute/SymParser.h>          // CSymParser (ResolvePath)
#include <Bute/SymTab.h>             // CSymTab (Insert)
#include <Gruntz/AreaMgr.h>          // CAreaMgr (g_61139c)
#include <Gruntz/FontConfig.h>       // CFontConfig (m_4->m_5c)
#include <Gruntz/WorldSoundSet.h>    // CWorldSoundSet (m_4->m_54)
#include <Io/SaveGame.h>             // CSaveGame (m_4->m_58)
#include <Gruntz/GruntSpawnConfig.h> // CGruntSpawnConfig (m_4->m_60)
#include <Gruntz/GruntzMgr.h>        // CGruntzMgr (m_4)
#include <Gruntz/Play.h>             // the real CPlay (`this` derives from it)
#include <Gruntz/SpriteFactory.h>    // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <rva.h>
#include <stdio.h>  // sprintf (0x11f890)
#include <stdlib.h> // atoi (0x11ffb0) + srand (0x11fed0)
#include <Globals.h>

// ---------------------------------------------------------------------------
// Shared singletons + the per-mode/per-area globals (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
extern "C" i32 g_644c54;            // DAT_00644c54 (area index)
extern "C" i32 g_645270;            // DAT_00645270 (area page size)
extern void* g_645570;              // DAT_00645570
extern "C" i32 g_64558c;            // DAT_0064558c
extern "C" i32 g_64e35c;            // DAT_0064e35c
extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA @0x6bf37c
extern void* g_612618;              // DAT_00612618 (last-level cache)
extern void* g_61139c;              // PTR_DAT_0061139c
extern "C" char g_emptyString[];    // _g_emptyString @0x6293f4

// ---------------------------------------------------------------------------
// RESIDUAL carcass (9 classes already folded onto their real canonical headers:
// CGruntzMgr/CGruntSpawnConfig/CButeMgr/CAreaMgr/CSymTab/CSymParser/CSaveGame/
// CFontConfig/CWorldSoundSet). The methods below are the DEFERRED remainder,
// grouped by why they aren't folded yet (all reloc-masked, so byte-neutral):
//   (a) cross-module canonical header + retype (SoundStream Dsndmgr[Teardown/grid];
//       CGruntzSoundZ Dsndmgr[Reset0]; CGruntzCmdMgr Net[ClearList]; DirectInputMgr2
//       StateMgrBZ[HideMenu]; CDDSurface ResMgr[ShadeRect]; CSBI_RectOnly SBI_Image
//       [ClearTiles/PostMap]);
//   (b) needs a byte-neutral declared-only method added to its Gruntz header first
//       (CParseSource CImage.h[BeginParse/EndParse]; CBrickz/CBrickzGrid[LoadAttributes/
//       UpdateDiagonals]; CLightFxRender[InstallLightFx/BuildLightShape]; CTimer
//       [TimerEqSet/TimerReset]; CBattlezData[BattlezInit=Init]);
//   (c) Teardown-on-savedThis is CMulti::AckJoinFailure (a cross-cast of the CPlay
//       `this`); WorkerReset is the GruntzPlayer(int) ctor (placement-new);
//   (d) @identity-TODO - genuinely unrecovered (no xref/header): ObListInit 0x1b48a6,
//       ButeStore 0x1b5485, GetBool 0x1bedde, CImageSet3::GetSize 0x1633e0 (no header).
// ---------------------------------------------------------------------------
struct Eng {
    void Teardown();                           // 0x137a80  (grid)
    void Reset0();                             // 0x138530  (m_4->m_48)
    void WorkerReset(i32 a);                   // 0x40a7    (team)
    void BattlezInit();                        // 0x2e3c    CBattlezData::Init (g_gameReg->m_7c)
    void ObListInit();                         // 0x1b48a6  (g_gameReg->m_6c+0x1c)
    void ClearList();                          // 0x2c11    (g_gameReg->m_6c)
    i32 BeginParse();                          // 0x139960  CParseSource (set)
    i32 EndParse();                            // 0x1399d0  CParseSource (set)
    i32 LoadAttributes(i32 a, i32 b);          // 0x3d19    CBrickz (m_4->m_70)
    i32 UpdateDiagonals(void* a);              // 0x3562    CBrickzGrid (m_4->m_70)
    i32 InstallLightFx(void* mgr, i32 id);     // 0x3bf2    CLightFxRender (ctx)
    i32 BuildLightShape(i32 a);                // 0x3bca    CLightFxRender (ctx)
    i32 GetSize();                             // 0x1633e0/0x163300 CImageSet3
    void ClearTiles(i32 n);                    // 0x130c    CSBI_RectOnly (m_2dc)
    void PostMap();                            // 0x26a3    CSBI_RectOnly (m_2dc)
    void HideMenu();                           // 0x133110  (g_645570)
    void ShadeRect(i32 a, void* rect);         // 0x13f460  CDDSurface (map surface)
    i32 TimerEqSet(i32 a, i32 b);              // 0x2eaa    CTimer::LoadTimerSprite (m_3f4)
    void TimerReset();                         // 0x14ce    CTimer::Reset (m_3f4)
    i32 GetBool(i32 key, void* out, i32 dflt); // 0x1bedde  GetBool (bute)
    void ButeStore(void* key, void* val);      // 0x1b5485  (g_gameReg->m_68+0x260)
};

// ---------------------------------------------------------------------------
// Genuine __cdecl engine helpers (reloc-masked rel32).
// ---------------------------------------------------------------------------
void Cmd_ResetScroll();            // 0x2bd0  YAXXZ
i32 QueryToken(i32 a);             // 0x39a4  QueryToken(int)
i32 ValidateMainBlock(void* cstr); // 0x2c8e  static WwdFile (CString byval)
void ActiveWait(i32 ms);           // 0x13dfe0 busy-wait
void* RezAlloc(i32 sz);            // 0x1b9b46 operator new
void RezFree(void* p);             // 0x1b9b82 operator delete
void EngStr_DrawText(                                         // 0x1c5d  YAX...
    void* host, void* rect, void* buf, i32 a, i32 b, i32 c, i32 d, i32 e, i32 f
);

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))
#define E(p) ((Eng*)(p))

// ---------------------------------------------------------------------------
// The PLAY-state level loader (`this`). Its own init-chain steps are __thiscall
// methods; raw-offset field access via the macros.
// ---------------------------------------------------------------------------
// `this` IS the canonical CPlay (proven by xref - the init-chain leaves are all
// CPlay methods; the +0x10/+0x84/+0xa4/+0xa8 dispatches map to CPlay's real vtable
// slots Update/Vslot21/BuildMusicCategoryTable/BuildWorldLevelPath). LoadByMode is
// modeled on this thin CPlay-derived facet (its two big methods aren't yet in the
// canonical Play.h owned by another unit); the init-chain steps are declared here
// as reloc-masked leaves (real CPlay/CLevelValidator targets in comments).
class CPlayLevelLoad : public CPlay {
public:
    i32 LoadByMode(i32 level, i32 unused); // ?LoadLevelByMode@@ placeholder

    i32 BuildHelpReveal(i32 a);  // 0x1019
    i32 RegisterInputBindings(); // 0x3a71
    void AckJoinFailure();       // 0x35e4  (CMulti, on a saved-flag object)
    i32 FadeInTitle(const char* b, i32 c, i32 d, i32 e, i32 f, i32 g); // 0x1e60
    i32 SoundStep(i32 a, i32 b, i32 c, i32 d);                         // 0x1843
    i32 InfoTextStep();                                                // 0x14b5
    void ZeroBlock();                                                  // 0x32d3
    i32 LoadActionTileSprites(i32 a);                                  // 0x2dce
    void BuildWarlordNameTable(i32 a);                                 // 0x38dc
    i32 StepB(i32 a);                                                  // 0x3021
    i32 StepC(i32 a);                                                  // 0x3346
    i32 StepD(i32 a);                                                  // 0x23b5
    i32 StepE(i32 a);                                                  // 0x1c2b
    i32 StepF(i32 a);                                                  // 0x2964
    i32 StepG(i32 a);                                                  // 0x2e9b
    void StepH();                                                      // 0x4458
    i32 StepI(i32 a);                                                  // 0x2c07
    i32 StepJ(i32 a);                                                  // 0x247d
    i32 StepK(i32 a);                                                  // 0x3b61
    void FinalizeWorld();                                              // 0x1db6  (via m_4)
    void ScanWarpStone();                          // 0x28dd  CPlay::ScanShuffleQuads
    i32 AfterTitle();                              // 0x2e14  (via m_4)
    void SetTitle(void* save58, i32 n, void* out); // 0x175d (via m_4->m_58)
    i32 LoadMap(void* a, void* b);                 // 0x2b80  CPlay::LoadWarlordSprites
    i32 LoadStep1();                               // 0x3553  CPlay::ScanBuildTiles
    i32 LoadStep2();                               // 0x345e  CLevelValidator::ValidateLevelTiles
    i32 LoadStep3();                               // 0x17ee  CPlay::AddLevelGruntz
    void StartGame();                              // 0x3d55
};

// ===========================================================================
RVA(0x000ca200, 0xe34)
i32 CPlayLevelLoad::LoadByMode(i32 level, i32) {
    void* self = this;
    void* gameReg;
    void* set;
    i32 reload = 0; // [esp+0x20] warp-cache reload flag (-> the vtable +0xa4 arg)
    // one contiguous stack buffer: the AREA%i/Level%i name at +0x00, a 148-byte
    // zeroed sub-block at +0x20 (retail's [esp+0x58] rep-stos region / LoadMap arg).
    char nameBuf[0xb4]; // [esp+0x38]

    // ---- 1) reset prior-level scroll/area globals ----
    I32(self, 0x484) = 1;
    g_645584 = 0;
    g_645580 = 0;
    g_645588 = 0;
    g_6455f0 = 0;
    if (level > 0x64) {
        level -= 0x64;
        g_6455f0 = 1;
    }

    // clear the m_3f4 worker's slots
    void* worker = PTR(self, 0x3f4);
    if (worker != 0) {
        I32(worker, 0x40) = 0;
        I32(worker, 0x44) = 0;
        I32(worker, 0x30) = 0;
        I32(worker, 0x34) = 0;
        I32(worker, 0x48) = 0;
        I32(worker, 0x4c) = 0;
    }

    // tear down the old grid (self->m_c->m_28->m_2c) / map / sound sub-objects
    void* grid = PTR(PTR(PTR(self, 0xc), 0x28), 0x2c);
    if (grid != 0) {
        E(grid)->Teardown();
    }
    E(PTR(PTR(self, 0x4), 0x48))->Reset0();
    ((CWorldSoundSet*)PTR(PTR(self, 0x4), 0x54))->Teardown();
    ((CGruntSpawnConfig*)PTR(PTR(self, 0x4), 0x60))->DtorBody();
    ((CGruntSpawnConfig*)PTR(PTR(self, 0x4), 0x60))->ClearSprites();
    ((CGruntzMgr*)PTR(self, 0x4))->RestoreVideoMode(0);

    gameReg = g_64556c;
    if (I32(gameReg, 0x134) != 2) {
        g_644c54 = 0;
        if (I32(gameReg, 0xc) != 0) {
            i32 v = I32(gameReg, 0xc) ^ 1;
            I32(gameReg, 0xc) = v;
            ((CGruntzMgr*)g_64556c)->FinishLevel(v, 1);
        }
    }

    // clear the 4 score/team slots at +0x384 (-4 .. +28, two dwords each)
    {
        i32* p = (i32*)((char*)self + 0x388);
        i32 n = 4;
        do {
            p[-1] = -1;
            p[0] = -1;
            p += 2;
        } while (--n);
    }

    g_6bf3c0 = g_645580;
    g_6bf3bc = g_645584;

    // reset the 4 team blocks at host->m_150 (stride 0x238): single-mode primes
    // team 0 ready, multi-mode zeroes the round counters.
    for (i32 t = 0; t < 4; ++t) {
        void* hostBase = PTR(self, 0x4);
        gameReg = g_64556c;
        void* team = (char*)hostBase + t * 0x48 * 8 - t * 8 + 0x150; // [edx+ecx*8+0x150]
        if (I32(gameReg, 0x134) == 1) {
            E(team)->WorkerReset(0);
            if (t == 0) {
                I32(team, 0x20) = 1;
                I32(team, 0x28) = 1;
            }
        } else {
            I32(team, 0x2c) = 0;
            I32(team, 0x28) = I32(team, 0x20);
            I32(team, 0x24) = 0;
        }
    }

    // ---- 2) mode/level-number resolve ----
    i32 modeFlag = ((i32)Update() == 0x11) ? 1 : 0;
    void* savedThis = modeFlag ? self : 0; // [esp+0x10] = (-modeFlag) & self
    I32(self, 0x1c4) = 1;
    I32(self, 0x1c) = level;
    {
        i32 r = (level - 1) % 0x24;
        I32(self, 0x20) = r / 4 + 1; // ((level-1)%0x24)/4 + 1 (signed div-by-4)
    }

    gameReg = g_64556c;
    g_645588 = 0;
    if (I32(gameReg, 0x134) == 3) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    Cmd_ResetScroll();
    E(PTR(g_64556c, 0x7c))->BattlezInit();
    E((char*)PTR(g_64556c, 0x6c) + 0x1c)->ObListInit();
    E(PTR(g_64556c, 0x6c))->ClearList();
    g_64558c = 0;
    I32(self, 0x1bc) = 0;
    I32(PTR(self, 0x4), 0x130) = 0;

    // already-loaded guard: when host->m_c8[-2] == 0 skip the name-resolve.
    void* host = PTR(self, 0x4);
    if (I32(PTR(host, 0xc8), -8) != 0) {
        if (I32(host, 0x128) != 0) {
            // BATTLEZ: resolve the level number from the level name's digit run.
            set = ((CSymParser*)PTR(host, 0x34))->ResolvePath("GAME_BATTLEZ");
            if (set == 0) {
                goto fail0;
            }
            i32 ins = ((CSymTab*)set)
                          ->Insert(
                              (const char*)((CGruntzMgr*)PTR(self, 0x4))->GetWorldFileName(),
                              g_emptyString
                          );
            if (ins == 0) {
                return 0;
            }
            void* desc = (void*)E(set)->BeginParse();
            if (desc == 0) {
                goto fail0;
            }
            char* p = (char*)desc + 0x10;
            char c = *p;
            while (c != 0) {
                if (c < '0' || c > '9') {
                    ++p;
                    c = *p;
                    continue;
                }
                break;
            }
            i32 num = atoi(p);
            E(set)->EndParse();
            level = num;
        } else if (I32(host, 0x12c) != 0) {
            // MULTI: same digit resolve off "GAME_MULTI".
            set = ((CSymParser*)PTR(host, 0x34))->ResolvePath("GAME_MULTI");
            if (set == 0) {
                goto fail0;
            }
            i32 ins = ((CSymTab*)set)
                          ->Insert(
                              (const char*)((CGruntzMgr*)PTR(self, 0x4))->GetWorldFileName(),
                              g_emptyString
                          );
            if (ins == 0) {
                return 0;
            }
            void* desc = (void*)E(set)->BeginParse();
            if (desc == 0) {
                goto fail0;
            }
            char* p = (char*)desc + 0x10;
            char c = *p;
            while (c != 0) {
                if (c < '0' || c > '9') {
                    ++p;
                    c = *p;
                    continue;
                }
                break;
            }
            i32 num = atoi(p);
            E(set)->EndParse();
            level = num;
        } else {
            // default: bute-driven level number (ValidateMainBlock(CString)).
            level = ValidateMainBlock(
                (void*)(const char*)((CGruntzMgr*)PTR(self, 0x4))->GetWorldFileName()
            );
            I32(self, 0x1bc) = 0;
            I32(PTR(self, 0x4), 0x130) = 0;
        }

        // recompute area page from the resolved level number
        i32 r = (level - 1) % 0x24;
        I32(self, 0x1c) = level;
        I32(self, 0x20) = r / 4 + 1;
    }

    // ---- 3) build the level name + look it up ----
    sprintf(nameBuf, "AREA%i", I32(self, 0x20));
    set = ((CSymParser*)PTR(self, 0x8))->ResolvePath(nameBuf);
    I32(self, 0x28) = (i32)set;
    if (set == 0) {
        goto fail0;
    }

    // ---- 4) area-page jump table over (m_20 - 1) in 0..7 ----
    {
        i32 page = I32(self, 0x20) - 1;
        switch ((u32)page) {
            case 0:
                g_645270 = 4;
                break;
            case 1:
                g_645270 = 0;
                g_64553c = 0;
                break;
            case 2:
                g_645270 = 8;
                g_64553c = 0;
                break;
            case 3:
                g_645270 = 8;
                g_64553c = 0xf;
                break;
            case 4:
                g_645270 = 5;
                g_64553c = 9;
                break;
            case 5:
                g_645270 = 4;
                g_64553c = 0xb;
                break;
            case 6:
                g_645270 = 0xe;
                g_64553c = 0xb;
                break;
            case 7:
                g_645270 = 4;
                g_64553c = 0;
                break;
            default:
                g_645270 = 4;
                g_64553c = 0;
                break;
        }
    }

    // m_2c = m_28 (the resolved area descriptor); refresh the host window
    {
        i32 prevTiles = I32(self, 0x2c);
        I32(self, 0x2c) = I32(self, 0x28);
        UpdateWindow((HWND)PTR(PTR(PTR(self, 0x4), 0x4), 0x4));

        host = PTR(self, 0x4);
        if (I32(PTR(host, 0xc8), -8) != 0) {
            if (I32(host, 0x128) == 0 && I32(host, 0x12c) == 0) {
                sprintf(nameBuf, "CUSTOMLEVEL", 0);
            }
        } else if (level > 0x24) {
            sprintf(nameBuf, "TRAINING", 0);
        }
        (void)prevTiles;
    }

    // ---- 5) the long linear init chain ----
    if (!FadeInTitle(nameBuf, 0, 0, 0, 0, 1)) { // [esp+0x48] out, push 0/0/0/0/1
        goto fail0;
    }
    SoundStep(0x50, 0x3e8, 0, 1);
    InfoTextStep();
    I32(self, 0x2c) = 0;
    {
        i32* z = (i32*)((char*)nameBuf + 0x20);
        i32 n = 0x25;
        while (n--) {
            *z++ = 0;
        }
    }
    ZeroBlock();
    BuildHelpReveal(0);
    Vslot21(); // vtable +0x84 (CPlay slot 33)
    if (savedThis != 0) {
        E(savedThis)->Teardown(); // AckJoinFailure placeholder (0x35e4 on saved obj)
    }
    RegisterInputBindings();

    if (!StepK(0)) {
        goto fail0;
    }

    // the warp-stone / last-level cache comparison (612618 / 61139c)
    {
        void* cached = g_612618;
        i32 eq = ((CAreaMgr*)g_61139c)->SameGroup((i32)cached); // 0x2f2c
        reload = (eq == 0) ? 1 : 0;
        i32 diff = (level != (i32)g_612618) ? 1 : 0;
        if (g_61139c == 0) {
            return 0;
        }
        g_612618 = (void*)level;

        BuildHelpReveal(0);
        if (savedThis != 0) {
            E(savedThis)->Teardown();
        }
        RegisterInputBindings();

        BuildHelpReveal(0);
        if (savedThis != 0) {
            E(savedThis)->Teardown();
        }
        RegisterInputBindings();

        if (!LoadActionTileSprites(diff)) {
            goto fail0;
        }
    }

    // a tail of paired BeginStep(0)/EndStep brackets around the real init steps.
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (modeFlag != 0 && I32(g_64556c, 0x134) == 1) {
        BuildWarlordNameTable((i32)savedThis);
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!StepB(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!StepC(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!StepD((i32)savedThis)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!StepE(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!StepF(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!StepG(0)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    StepH();
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!StepI(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!StepJ(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    if (!BuildWorldLevelPath(1)) { // vtable +0xa8 (CPlay slot 42)
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();

    // finalize the world planes
    ((CGruntzMgr*)PTR(self, 0x4))->RecomputeViewScale();
    if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
        E(PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize();
    }
    if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
        E(PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize();
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();

    // view setup off host->m_70
    {
        void* g5c = PTR(PTR(PTR(self, 0xc), 0x24), 0x5c);
        void* host70 = PTR(PTR(self, 0x4), 0x70);
        if (!E(host70)->LoadAttributes(I32(g5c, 0x28), I32(g5c, 0x2c))) {
            goto fail0;
        }
    }
    if (!E(PTR(PTR(self, 0x4), 0x70))->UpdateDiagonals(PTR(self, 0x4))) {
        goto fail0;
    }

    // lazily allocate the level context at +0x320
    if (I32(self, 0x320) == 0) {
        void* ctx = RezAlloc(0x43c);
        if (ctx != 0) {
            I32(ctx, 0) = 0;
            I32(ctx, 0x4) = 0;
            I32(ctx, 0x8) = 0;
            I32(ctx, 0xc) = 0;
            I32(ctx, 0x10) = 0;
            I32(ctx, 0x48) = 0;
            I32(ctx, 0x434) = 0;
            I32(ctx, 0x438) = 0;
        } else {
            ctx = 0;
        }
        I32(self, 0x320) = (i32)ctx;
        if (!E(ctx)->InstallLightFx(PTR(self, 0x4), 0xfa)) {
            goto fail0;
        }
    }
    if (!E(PTR(self, 0x320))->BuildLightShape(I32(self, 0x20))) {
        goto fail0;
    }

    // ---- the WarpStone bute scan (single-mode only) ----
    gameReg = g_64556c;
    if (I32(gameReg, 0x134) != 1) {
        CString warp; // [esp+0x14]
        i32 same = 0;
        if (E(&warp)->GetBool(0x81ab, &warp, 2)) {
            char* a = (char*)(const char*)((CGruntzMgr*)g_64556c)->GetWorldFileName();
            char* b = (char*)(const char*)warp;
            i32 eq = 1;
            while (*b == *a) {
                if (*b == 0) {
                    break;
                }
                a += 1;
                b += 1;
            }
            if (*b != *a) {
                eq = 0;
            }
            same = eq;
        }
        if (same) {
            ScanWarpStone();
        }
    }

    // ---- area-name -> level-list build ----
    if (I32(PTR(self, 0x4), 0x134) == 3) {
        AfterTitle();
    }
    ((CSaveGame*)PTR(PTR(self, 0x4), 0x58))
        ->FillSlot2((SaveSlot*)((char*)self + 0x1d0), I32(self, 0x1c), 0);
    {
        CString key; // [esp+0x18]
        gameReg = g_64556c;
        I32(PTR(gameReg, 0x68), 0x2a0) = 0;
        i32 count = I32(self, 0x1c);
        i32 i = count - ((count - 1) % 4); // round-down-to-4 idiom
        for (; i < count; ++i) {
            // key.Format("Level%i", i) -> wsprintf-into-CString (0x1b2cf5)
            key.Format("Level%i", i);
            void* bm = PTR(g_64556c, 0x68);
            i32 v = g_buteMgr.GetInt((const char*)key, "WarpStone");
            E((char*)bm + 0x260)->ButeStore(PTR(bm, 0x268), (void*)v);
        }
    }
    E(PTR(self, 0x2dc))->ClearTiles(I32(self, 0x1c));

    // ---- CursorSnapSprite registration (factory at [self+0xc]->m_8) ----
    set = ((CSpriteFactory*)PTR(PTR(self, 0xc), 0x8))
              ->CreateSprite(0, 0, 0, 0x13880, "CursorSnapSprite", 0x40001);
    I32(self, 0x4e4) = (i32)set;
    if (set != 0) {
        void* host8 = PTR(PTR(self, 0xc), 0x8);
        (*(void (**)(void*, i32))((char*)*(void**)host8 + 0x24))(host8, 0); // host8 vtable +0x24
        if (savedThis == 0) {
            // empty cursor-snap set -> reset the resource-install flag
            void* tiles = PTR(self, 0x2dc);
            i32 id = (I32(tiles, 0) == 0) ? 0x1a9 : 0x249;
            if (!E(PTR(self, 0x3f4))->TimerEqSet(id, 0x1ca)) {
                void* spr = PTR(self, 0x3f4);
                if (spr != 0) {
                    E(spr)->TimerReset();
                    RezFree(spr);
                    I32(self, 0x3f4) = 0;
                }
            }
        } else {
            // load the level map + the four map sub-steps
            if (LoadMap(savedThis, (char*)nameBuf + 0x20) && LoadStep1() && LoadStep2()
                && LoadStep3()) {
                void* host8b = PTR(PTR(self, 0xc), 0x8);
                (*(void (**)(void*, i32))((char*)*(void**)host8b + 0x24))(host8b, 0);
                E(PTR(self, 0x2dc))->PostMap();
                E(g_645570)->HideMenu();
                while (ShowCursor(0) >= 0)
                    ;
                ((CGruntzMgr*)PTR(self, 0x4))->CGruntzMgr::PerFrameTick();
                if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
                    E(PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize();
                }
                if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
                    E(PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize();
                }
                BuildHelpReveal(0);
                if (savedThis != 0) {
                    E(savedThis)->Teardown();
                }
                RegisterInputBindings();
                if (BuildMusicCategoryTable(reload)) { // vtable +0xa4 (CPlay slot 41)
                    goto okContinue;
                }
            }
            goto fail1;
        }
    }

okContinue:
    BuildHelpReveal(0);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }
    RegisterInputBindings();
    BuildHelpReveal(1);
    ActiveWait(0x64);
    if (savedThis != 0) {
        E(savedThis)->Teardown();
    }

    gameReg = g_64556c;
    if (I32(gameReg, 0x114) == 0) {
        void* mapHost = PTR(PTR(PTR(PTR(self, 0xc), 0x4), 0x10), 0x2c);
        E(mapHost)->ShadeRect(0x32, 0);
        gameReg = g_64556c;
    }

    // ---- loading-screen blit (mode != 2 && m_114 == 0) ----
    if (I32(gameReg, 0x134) != 2 && I32(gameReg, 0x114) == 0) {
        CString scr; // [esp+0x14]
        I32(self, 0x4f8) = 1;
        I32(self, 0x484) = 0;
        i32 rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = 0x280;
        rect[3] = 0x1e0;
        if (E(&scr)->GetBool(0x8128, &scr, 4)) {
            EngStr_DrawText(PTR(self, 0xc), rect, (char*)nameBuf + 0x4, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    } else {
        I32(self, 0x484) = 1;
    }

    // ---- final state stamp ----
    I32(self, 0x4b8) = 0;
    I32(self, 0x4fc) = 0;
    I32(self, 0x500) = 0;
    I32(self, 0x4f0) = 0;
    I32(self, 0x4f4) = 0;
    I32(self, 0x400) = 0x1f4;
    I32(self, 0x404) = 0;
    I32(self, 0x3f8) = g_645588;
    I32(self, 0x3fc) = 0;
    I32(self, 0x408) = 1;
    ((CString*)((char*)self + 0x410))->operator=(g_emptyString);
    I32(self, 0x40c) = 0;
    I32(self, 0x470) = 0;
    I32(self, 0x474) = 0;
    I32(self, 0x478) = 0;
    I32(self, 0x47c) = 0;
    I32(self, 0x4b0) = 0;
    I32(self, 0x514) = 3;
    I32(self, 0x4ec) = 1;
    g_64e35c = 0;
    StartGame();
    if (I32(g_64556c, 0x134) == 2) {
        g_64e35c = 1;
        I32(self, 0x4ec) = 0;
        ((CGruntzMgr*)PTR(self, 0x4))->CheckSavedMode();
        ((CFontConfig*)PTR(PTR(self, 0x4), 0x5c))->FreeNodes();
    }
    return 1;

fail1:
    return 0;
fail0:
    return 0;
}

#undef I32
#undef PTR
#undef E

SIZE_UNKNOWN(Eng);

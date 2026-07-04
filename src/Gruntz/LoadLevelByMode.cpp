// LoadLevelByMode.cpp - the PLAY game-state per-mode level loader
// (C:\Proj\Gruntz). A backlog megafunction (3636 B): given a level/mode id
// (arg1) it tears down the previous level, resolves the level's area page and
// display name, then runs the long linear init chain that brings up a level
// (the BATTLEZ / MULTI / CUSTOMLEVEL / TRAINING / Level%i variants), wiring the
// map, sound, palette, cursor-snap sprite, status bars and the loading screen.
// It is a /GX EH-framed routine (CString temps at [esp+0x14]/[esp+0x18] for the
// area-name and warp-stone formatting give it the exception frame) with one
// area-page jump-table switch over the area count.
//
// Identity: the stub had it as EngineLabelBacklog::LoadLevelByMode; the $SG
// string set ("GAME_BATTLEZ", "GAME_MULTI", "AREA%i", "CUSTOMLEVEL",
// "TRAINING", "Level%i", "WarpStone", "CursorSnapSprite") and the
// g_gameReg->m_134 mode dispatch (1=single, 2=multi, 3=battle) identify it as
// the per-mode level bring-up on the >0x514-byte PLAY-state object (`this`=esi,
// sibling of CGamePlayInput::DispatchKey in GameKeyHandler.cpp). Reconstructed
// as an EH unit (the CString temps).
//
// Structure:
//   1) Reset prior-level state: zero the four scroll/area globals, clear the
//      m_3f4 worker's slots, tear down the old map/sound/grid sub-objects.
//   2) Mode-split on g_gameReg->m_134 (battle/multi/single) + the m_4->m_c8
//      already-loaded guard: resolve the level/area number `level` either from
//      the "GAME_BATTLEZ"/"GAME_MULTI" registry key (skipping a trailing digit
//      run) or from the bute-driven default; compute the area page index m_20 =
//      ((level-1)/0x24)*... .
//   3) Build the level name: "AREA%i"/"CUSTOMLEVEL"/"TRAINING"/"Level%i" then
//      Lookup the level descriptor in the game's level set (m_8).
//   4) The area-page jump table (m_20-1 in 0..7 -> 0x4cb034) seeds the two
//      per-page globals g_645270 (page size) and g_64553c (sub-index).
//   5) The long linear init chain: a sequence of `BeginStep(0)/EndStep()` paired
//      sub-object steps, each gated `if (!Step()) goto fail`, that brings the
//      level online (map, fog, sounds, the WarpStone bute scan, the
//      CursorSnapSprite registration, the status-bar build, the loading-screen
//      blit). Returns 1 on success, 0 (edi) on any failing step.
//
// CARCASS doctrine: `this` and the g_gameReg singleton sub-objects are unmatched
// engine classes accessed by raw this+offset (a naming-independent choice for
// this externally-coupled loader). Every callee body is external (no-body,
// reloc-masked rel32 / IAT FF15); the data globals + GAME_*/Level%i strings are
// named so their DIR32 operands reloc-mask. PtInRect-family imports (timeGetTime,
// UpdateWindow, ShowCursor) come via <Win32.h>. Only offsets / code bytes are
// load-bearing.
//
// Returns int (1 ok / 0 fail) - a void model would tail-merge the many
// `mov eax,edi; ... ret` failure epilogues (see
// docs/patterns/void-vs-bool-return-epilogue-split.md).
//
// @early-stop
// /GX megafunction wall. The state reset, the battle/multi/single mode split
// with the inline trailing-digit skip + the area-page math, the "AREA%i"/
// "Level%i" name build, the area jump table and the full linear init chain are
// reconstructed here and match retail's logic. The residual is the documented
// wall shared by the sibling /GX megafunctions (MainMenuBuilder ~78%,
// ValidateLevelTiles ~17%, TerrainTileLoader): MSVC5's EH-state numbering across
// the two CString temps, the idiv area-page schedule, and the regalloc pinning
// of 0/1/level in ebx/ebp/edi/edx across the 3636-byte frame are not steerable
// from C source. Deferred to the final sweep (docs/patterns/
// eh-state-numbering-base.md; big-seh-fuzzy-desync.md; zero-register-pinning.md;
// o2-optimizer-bailout-framed.md).

#include <Win32.h> // timeGetTime / UpdateWindow / ShowCursor (reloc-masked IAT)
#include <rva.h>
#include <stdio.h>  // sprintf (0x11f890)
#include <stdlib.h> // atoi (0x11ffb0) + srand (0x11fed0)
#include <Globals.h>

// ---------------------------------------------------------------------------
// Shared singletons + the per-mode/per-area globals (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
extern void* g_64556c;              // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
extern "C" i32 g_645580;            // DAT_00645580
extern "C" i32 g_645584;            // DAT_00645584
extern "C" i32 g_645588;            // DAT_00645588 (running game clock; cleared here)
extern "C" i32 g_644c54;            // DAT_00644c54 (area index)
extern "C" i32 g_645270;            // DAT_00645270 (area page size)
extern void* g_645570;              // DAT_00645570
extern "C" i32 g_64558c;            // DAT_0064558c
extern "C" i32 g_64e35c;            // DAT_0064e35c
extern "C" i32 g_6bf3c0;            // _g_6bf3c0 (draw-clock mirror)
extern "C" i32 g_6bf3bc;            // _g_6bf3bc (default geo)
extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA @0x6bf37c
extern void* g_612618;              // DAT_00612618 (last-level cache)
extern void* g_61139c;              // PTR_DAT_0061139c
extern void* g_buteMgr;             // ?g_buteMgr@@3VCButeMgr@@A @0x6453d8
extern "C" char g_emptyString[];    // _g_emptyString @0x6293f4

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked __thiscall ILT thunks (no body).
// The two recurring no-arg "step" calls (0x1019 BeginStep(arg), 0x3a71 EndStep)
// bracket each init sub-step; 0x35e4 destroys the CString temp.
// ---------------------------------------------------------------------------
void LlBeginStep(void* self, i32 a);   // 0x1019  __thiscall(this, push a)
void LlEndStep(void* self);            // 0x3a71  __thiscall(this)
void LlFreeTemp(void* tmp);            // 0x35e4  free the CString diagnostic temp
i32 LlTeardownGrid(void* a);           // 0x137a80 (cdecl-ish, reloc-masked)
void LlReset0(void* self);             // 0x138530 __thiscall
void LlReset1(void* self);             // 0x28ab
void LlReset2(void* self);             // 0x20a4
void LlReset3(void* self);             // 0x244b
void LlReset4(void* self, i32 a);      // 0x34ef (push edi)
void LlScrub(void* self, i32 a);       // 0x409d
void LlWorkerReset(void* self, i32 a); // 0x40a7

void* LlRegistryFind(void* reg, const char* key);            // 0x13c030 find set
void* LlSetField(void* self);                                // 0x2531 -> &m_xx
void* LlPathJoin(void* set, i32 nameField, const char* sep); // 0x13a000
void* LlSetResolve(void* set);                               // 0x139960
void LlSetRelease(void* set);                                // 0x1399d0
void* LlButeLevelNum(void* src);                             // 0x2c8e
void LlStrCtor(void* str);                                   // 0x1b9b93 CString::CString
void LlStrDtor(void* str);                                   // 0x1b9cde CString::~CString
void LlAssignStr(void* str, const char* s);                  // 0x1b9e74 CString::operator=

void LlSetMode(void* self, i32 mode); // 0x2bd0
void LlClearTimers(void* self);       // 0x2e3c
void LlObListInit(void* p);           // 0x1b48a6
void LlClearList(void* self);         // 0x2c11

// The long linear init chain steps (each __thiscall on `this`, ret BOOL).
i32 LlBuildGrid(void* self, void* outRect, i32, i32, i32, i32); // 0x1e60
void LlPrime(void* self, i32 a, i32 b, i32 c, i32 d);           // 0x1843
void LlPrime2(void* self);                                      // 0x14b5
void LlZeroBlock(void* self);                                   // 0x32d3
i32 LlStepA(void* self, i32 a);                                 // 0x2dce
void LlPostBuild(void* self, i32 a);                            // 0x38dc
i32 LlStepB(void* self, i32 a);                                 // 0x3021
i32 LlStepC(void* self, i32 a);                                 // 0x3346
i32 LlStepD(void* self, i32 a);                                 // 0x23b5
i32 LlStepE(void* self, i32 a);                                 // 0x1c2b
i32 LlStepF(void* self, i32 a);                                 // 0x2964
i32 LlStepG(void* self, i32 a);                                 // 0x2e9b
void LlStepH(void* self);                                       // 0x4458
i32 LlStepI(void* self, i32 a);                                 // 0x2c07
i32 LlStepJ(void* self, i32 a);                                 // 0x247d
i32 LlStepK(void* self, i32 a);                                 // 0x3b61
void LlFinalizeWorld(void* self);                               // 0x1db6
i32 LlClearPlaneA(void* a);                                     // 0x1633e0 (cdecl)
i32 LlClearPlaneB(void* a);                                     // 0x163300 (cdecl)
i32 LlSetupView(void* host70);                                  // 0x3d19 (host->m_70)
i32 LlSetupView2(void* host70, void* a);                        // 0x3562
void* RezAlloc(i32 sz);                                         // 0x1b9b46 operator new
i32 LlInstallCtx(void* host70, i32 id, void* ctx);              // 0x3bf2
i32 LlSizeView(void* ctx, i32 a);                               // 0x3bca
i32 LlScanWarpStone(void* self);                                // 0x28dd
void LlSetTitle(void* level58, i32 nameLen, void* out);         // 0x175d
i32 LlAfterTitle(void* self);                                   // 0x2e14
void LlWsFormat(void* out, const char* fmt, i32 n);             // 0x1b2cf5 wsprintf-into-CString
i32 LlButeLookup(void* bute, const char* sect, const char* key, void* out);              // 0x171af0
void LlButeStore(void* dst, void* key, void* val);                                       // 0x1b5485
void LlClearTiles(void* self, i32 n);                                                    // 0x130c
void* LlRegisterNamespace(i32 a, const char* ns, i32 tag, i32 cap, i32 b, i32 c, i32 d); // 0x1597b0
void LlSpriteHook(void* host8, i32 a);       // FUN via host->m_8 vtable +0x24
i32 LlEqSet(void* self, i32 a);              // 0x2eaa
void LlSpriteRelease(void* spr);             // 0x14ce
void RezFree(void* p);                       // 0x1b9b82 operator delete
i32 LlLoadMap(void* self, void* a, void* b); // 0x2b80
i32 LlLoadStep1(void* self);                 // 0x3553
i32 LlLoadStep2(void* self);                 // 0x345e
i32 LlLoadStep3(void* self);                 // 0x17ee
void LlPostMap(void* self);                  // 0x26a3
void LlHideMenu(void* a);                    // 0x133110
i32 LlFinalizeLevel(void* host4);            // 0x3d23
i32 LlBuildBars(void* self, i32 a);          // virtual host4 +0xa4 / 0xa8
void LlFinishMap(void* self);                // 0x12ee
void LlFinishMap2(void* self);               // 0x128a
void LlStartGame(void* self);                // 0x3d55
void ActiveWait(i32 ms); // 0x13dfe0 busy-wait
void LlSetCue(void* mapHost, i32 a, i32 b);  // 0x13f460
void LlSpriteResize(
    void* host,
    void* a,
    void* b,
    i32 c,
    i32 d,
    i32 e,
    i32 f,
    i32 g,
    i32 h
);                                                           // 0x1c5d
i32 LlButeBoolGet(void* bute, i32 key, void* out, i32 dflt); // 0x1bedde GetBool
i32 LlStrCmp(const char* a, const char* b);                  // inlined strcmp (modeled)

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// ---------------------------------------------------------------------------
// The PLAY-state level loader (`this`). Raw-offset access throughout.
// ---------------------------------------------------------------------------
class CPlayLevelLoad {
public:
    i32 LoadByMode(i32 level); // ?LoadLevelByMode@@ (the backlog placeholder)
    char m_pad[4];
};

// ===========================================================================
RVA(0x000ca200, 0xe34)
i32 CPlayLevelLoad::LoadByMode(i32 level) {
    void* self = this;
    void* gameReg;
    void* set;
    char nameBuf[0x20]; // [esp+0x38] AREA%i / Level%i / TRAINING name
    void* warpTemp;     // [esp+0x14] WarpStone CString temp
    void* areaTemp;     // [esp+0x18] area-name CString temp

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

    // tear down the old grid / map / sound sub-objects
    void* host = PTR(self, 0x4);
    void* grid = PTR(PTR(host, 0x28), 0x2c);
    if (grid != 0) {
        LlTeardownGrid(grid);
    }
    LlReset0(PTR(PTR(self, 0x4), 0x48));
    LlReset1(PTR(PTR(self, 0x4), 0x54));
    LlReset2(PTR(PTR(self, 0x4), 0x60));
    LlReset3(PTR(PTR(self, 0x4), 0x60));
    LlReset4(PTR(self, 0x4), 0);

    gameReg = g_64556c;
    if (I32(gameReg, 0x134) != 2 && I32(gameReg, 0xc) != 0) {
        g_644c54 = 0;
        i32 v = I32(gameReg, 0xc) ^ 1;
        I32(gameReg, 0xc) = v;
        LlScrub(g_64556c, v);
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

    // reset the 4 team blocks at host->m_150 (0x48-byte stride): single-mode
    // primes team 0 ready, multi-mode zeroes the round counters.
    for (i32 t = 0; t < 4; ++t) {
        void* hostBase = PTR(self, 0x4);
        gameReg = g_64556c;
        void* team = (char*)hostBase + t * 0x48 * 8 - t * 8 + 0x150; // [edx+ecx*8+0x150]
        if (I32(gameReg, 0x134) == 1) {
            LlWorkerReset(team, 0);
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
    // m_20 = ((level-1)/0x24) area page, sentinel m_1c4=1, m_1c=level
    i32 modeFlag = ((*(i32(**)(void*))(*(void**)self))(self) == 0x11) ? 1 : 0;
    i32 battleFlag = modeFlag;
    I32(self, 0x1c4) = 1;
    I32(self, 0x1c) = level;
    {
        // ebp = battleFlag, then masked into a slot for later restore
        i32 mask = -modeFlag;
        // [esp+0x10] = mask & self  (a saved-this idiom)
        (void)mask;
    }
    {
        i32 q = (level - 1) / 0x24;
        i32 r = (level - 1) % 0x24;
        I32(self, 0x20) = (r + (r & 3)) / 4 + 1 + q; // the /GX area-page math
    }

    gameReg = g_64556c;
    g_645588 = 0;
    if (I32(gameReg, 0x134) == 3) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    LlSetMode(self, 0);
    LlClearTimers(PTR(g_64556c, 0x7c));
    LlObListInit((char*)PTR(g_64556c, 0x6c) + 0x1c);
    LlClearList(PTR(g_64556c, 0x6c));
    g_64558c = 0;
    I32(self, 0x1bc) = 0;
    I32(PTR(self, 0x4), 0x130) = 0;

    // already-loaded guard: when host->m_c8[-2] == 0 skip the name-resolve and
    // jump straight to the "AREA%i" build at label nameBuild.
    host = PTR(self, 0x4);
    if (I32(PTR(host, 0xc8), -8) != 0) {
        if (I32(host, 0x128) != 0) {
            // BATTLEZ mode: resolve the level number from the level name's
            // trailing digit run.
            set = LlRegistryFind(PTR(host, 0x34), "GAME_BATTLEZ");
            if (set == 0) {
                goto fail0;
            }
            LlStrCtor(&areaTemp);
            char* name = (char*)*(void**)LlSetField(PTR(self, 0x4));
            set = LlPathJoin(set, (i32)name, g_emptyString);
            LlStrDtor(&areaTemp);
            if (set == 0) {
                return 0;
            }
            void* desc = LlSetResolve(set);
            if (desc == 0) {
                goto fail0;
            }
            // skip a trailing digit run, then atoi the tail
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
            LlSetRelease(set);
            level = num;
        } else if (I32(host, 0x12c) != 0) {
            // MULTI mode: same trailing-digit resolve off "GAME_MULTI".
            set = LlRegistryFind(PTR(host, 0x34), "GAME_MULTI");
            if (set == 0) {
                goto fail0;
            }
            LlStrCtor(&areaTemp);
            char* name = (char*)*(void**)LlSetField(PTR(self, 0x4));
            set = LlPathJoin(set, (i32)name, g_emptyString);
            LlStrDtor(&areaTemp);
            if (set == 0) {
                return 0;
            }
            void* desc = LlSetResolve(set);
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
            LlSetRelease(set);
            level = num;
        } else {
            // default: bute-driven level number.
            void* fld = LlSetField(PTR(self, 0x4));
            level = (i32)LlButeLevelNum(fld);
            I32(self, 0x1bc) = (i32)PTR(host, 0); // edi (the found set)
            I32(PTR(self, 0x4), 0x130) = 0;       // edi
        }

        // recompute area page from the resolved level number
        i32 q = (level - 1) / 0x24;
        i32 r = (level - 1) % 0x24;
        I32(self, 0x1c) = level;
        I32(self, 0x20) = (r + (r & 3)) / 4 + 1 + q;
    }

    // ---- 3) build the level name + look it up ----
    sprintf(nameBuf, "AREA%i", I32(self, 0x20));
    set = LlRegistryFind(PTR(self, 0x8), nameBuf);
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
                g_645270 = 0; // ecx is 0 here in retail
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
    if (!LlBuildGrid(self, nameBuf, 0, 0, 0, 1)) { // [esp+0x48] out, push 0/0/0/1
        goto fail0;
    }
    LlPrime(self, 0x50, 0x3e8, 0, 1);
    LlPrime2(self);
    I32(self, 0x2c) = 0; // edi
    {
        i32* z = (i32*)((char*)nameBuf + 0x20);
        i32 n = 0x25;
        while (n--) {
            *z++ = 0;
        }
    }
    LlZeroBlock(self);
    LlBeginStep(self, 0);
    (*(void (**)(void*))((char*)*(void**)self + 0x84))(self); // vtable +0x84
    if (I32(&warpTemp, 0) != 0) {                             // [esp+0x1c] temp guard
        LlFreeTemp(&areaTemp);
    }
    LlEndStep(self);

    if (!LlStepK(self, 0)) {
        goto fail0;
    }

    // the warp-stone / last-level cache comparison (612618 / 61139c)
    {
        void* cached = g_612618;
        i32 eq = LlEqSet(g_61139c, (i32)cached); // 0x2f2c
        i32 ebpFlag = (eq == 0) ? 1 : 0;
        i32 diff = (level != (i32)g_612618) ? 1 : 0;
        if (g_61139c == 0) {
            return 0;
        }
        g_612618 = (void*)level;

        LlBeginStep(self, 0);
        if (I32(&warpTemp, 0) != 0) {
            LlFreeTemp(&areaTemp);
        }
        LlEndStep(self);

        LlBeginStep(self, 0);
        i32 reload = diff;
        if (reload != 0) {
            LlFreeTemp(&areaTemp);
        }
        LlEndStep(self);

        if (!LlStepA(self, diff)) {
            goto fail0;
        }
        (void)ebpFlag;
    }

    // a tail of paired BeginStep(0)/EndStep brackets around the real init steps.
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (modeFlag != 0 && I32(g_64556c, 0x134) == 1) {
        LlPostBuild(self, (i32)warpTemp);
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepB(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepC(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepD(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepE(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepF(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepG(self, 0)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    LlStepH(self);
    LlEndStep(self);
    if (!LlStepI(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!LlStepJ(self, (i32)warpTemp)) {
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);
    if (!(*(i32(**)(void*, i32))((char*)*(void**)self + 0xa8))(
            self,
            (i32)warpTemp
        )) { // vtable +0xa8
        goto fail0;
    }
    LlBeginStep(self, 0);
    LlEndStep(self);

    // finalize the world planes
    LlFinalizeWorld(PTR(self, 0x4));
    if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
        LlClearPlaneA(0);
    }
    if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
        LlClearPlaneB(0);
    }
    LlBeginStep(self, 0);
    LlEndStep(self);

    // view setup off host->m_70
    if (!LlSetupView(PTR(PTR(self, 0x4), 0x70))) {
        goto fail0;
    }
    if (!LlSetupView2(PTR(PTR(self, 0x4), 0x70), PTR(self, 0x4))) {
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
        if (!LlInstallCtx(PTR(self, 0x4), 0xfa, ctx)) {
            goto fail0;
        }
    }
    if (!LlSizeView(PTR(self, 0x320), I32(self, 0x20))) {
        goto fail0;
    }

    // ---- the WarpStone bute scan (single-mode only) ----
    gameReg = g_64556c;
    if (I32(gameReg, 0x134) != 1) {
        LlStrCtor(&warpTemp);
        if (LlButeBoolGet(0, 0x81ab, &areaTemp, 2)) {
            // strcmp the bute string against the cached one
            char* a = (char*)*(void**)LlSetField(g_64556c);
            char* b = (char*)warpTemp;
            i32 same = (LlStrCmp(b, a) == 0) ? 1 : 0;
            LlStrDtor(&areaTemp);
            if (same) {
                LlScanWarpStone(self);
            }
        }
        LlStrDtor(&warpTemp);
    }

    // ---- area-name -> level-list build ----
    if (I32(PTR(self, 0x4), 0x134) == 3) {
        LlAfterTitle(self);
    }
    LlSetTitle(PTR(PTR(self, 0x4), 0x58), I32(self, 0x1c), (char*)self + 0x1d0);
    LlStrCtor(&warpTemp);
    {
        gameReg = g_64556c;
        I32(PTR(gameReg, 0x68), 0x2a0) = 0;
        i32 count = I32(self, 0x1c);
        i32 i = count;
        i32 base = count - ((count - 1 & 3) ^ ((count - 1) >> 31))
                   - (((count - 1) >> 31)); // round-to-4 idiom
        (void)base;
        for (; i < count; ++i) {
            LlWsFormat(&warpTemp, "Level%i", i);
            void* bm = PTR(g_64556c, 0x68);
            void* v = (void*)LlButeLookup(g_buteMgr, (const char*)warpTemp, "WarpStone", 0);
            LlButeStore((char*)bm + 0x260, PTR(bm, 0x268), v);
        }
    }
    LlClearTiles(PTR(self, 0x2dc), I32(self, 0x1c));

    // ---- CursorSnapSprite registration ----
    set = LlRegisterNamespace(0, "CursorSnapSprite", 0x40001, 0x13880, 0, 0, 0);
    I32(self, 0x4e4) = (i32)set;
    if (set != 0) {
        void* host8 = PTR(PTR(self, 0xc), 0x8);
        (*(void (**)(void*, i32))((char*)*(void**)host8 + 0x24))(host8, 0); // host8 vtable +0x24
        if (I32(&warpTemp, 0) == 0) {
            // empty cursor-snap set -> reset the resource-install flag
            void* tiles = PTR(self, 0x2dc);
            i32 id = (I32(tiles, 0) == 0) ? 0x1a9 : 0x249;
            if (!LlEqSet(PTR(self, 0x3f4), id)) {
                void* spr = PTR(self, 0x3f4);
                if (spr != 0) {
                    LlSpriteRelease(spr);
                    RezFree(spr);
                    I32(self, 0x3f4) = 0;
                }
            }
        } else {
            // load the level map + the four map sub-steps
            if (LlLoadMap(self, (void*)warpTemp, (char*)nameBuf + 0x20) && LlLoadStep1(self)
                && LlLoadStep2(self) && LlLoadStep3(self)) {
                void* host8b = PTR(PTR(self, 0xc), 0x8);
                (*(void (**)(void*, i32))((char*)*(void**)host8b + 0x24))(host8b, 0);
                LlPostMap(PTR(self, 0x2dc));
                LlHideMenu(g_645570);
                while (ShowCursor(0) >= 0)
                    ;
                LlFinalizeLevel(PTR(self, 0x4));
                if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
                    LlClearPlaneA(0);
                }
                if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
                    LlClearPlaneB(0);
                }
                LlBeginStep(self, 0);
                LlEndStep(self);
                if ((*(i32(**)(void*, i32))((char*)*(void**)self + 0xa4))(self, (i32)warpTemp)) {
                    goto okContinue;
                }
            }
            goto fail1;
        }
    }

okContinue:
    LlBeginStep(self, 0);
    LlEndStep(self);
    LlBeginStep(self, 1);
    ActiveWait(0x64);
    LlEndStep(self);

    gameReg = g_64556c;
    if (I32(gameReg, 0x114) == 0) {
        void* mapHost = PTR(PTR(PTR(PTR(self, 0xc), 0x4), 0x10), 0x2c);
        LlSetCue(mapHost, 0x32, 0);
        gameReg = g_64556c;
    }

    // ---- loading-screen blit (mode != 2 && m_114 == 0) ----
    if (I32(gameReg, 0x134) != 2 && I32(gameReg, 0x114) == 0) {
        I32(self, 0x4f8) = 1;
        I32(self, 0x484) = 0;
        LlStrCtor(&warpTemp);
        i32 rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = 0x280;
        rect[3] = 0x1e0;
        if (LlButeBoolGet(0, 0x8128, &areaTemp, 4)) {
            LlSpriteResize(PTR(self, 0xc), rect, (char*)nameBuf + 0x4, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        LlStrDtor(&areaTemp);
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
    LlAssignStr((char*)self + 0x410, g_emptyString);
    I32(self, 0x40c) = 0;
    I32(self, 0x470) = 0;
    I32(self, 0x474) = 0;
    I32(self, 0x478) = 0;
    I32(self, 0x47c) = 0;
    I32(self, 0x4b0) = 0;
    I32(self, 0x514) = 3;
    I32(self, 0x408) = 1;
    I32(self, 0x4ec) = 1;
    g_64e35c = 0;
    LlStartGame(self);
    if (I32(g_64556c, 0x134) == 2) {
        g_64e35c = 1;
        I32(self, 0x4ec) = 0;
        LlFinishMap(PTR(self, 0x4));
    }
    LlFinishMap2(PTR(PTR(self, 0x4), 0x5c));
    return 1;

fail1:
    LlStrDtor(&warpTemp);
    return 0;
fail0:
    return 0;
}

#undef I32
#undef PTR

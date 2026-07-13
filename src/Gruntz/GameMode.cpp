// GameMode.cpp - the free menu/HUD helpers the CState base drives + the CGameModeBase
// cleanup pair (out-of-line COMDATs). The concrete leaf states were split into per-class
// TUs (per-class TU cut of the former god-TU):
//   CMenuState     -> src/Gruntz/MenuState.cpp
//   CCreditsState  -> src/Gruntz/CreditsState.cpp  (+ CCreditzOwner)
//   CBootyState + CMultiBootyState -> src/Gruntz/BootyStateActivate.cpp
// The CState BASE implementation (ctor @0x08c750 + slot-0 ??_G @0x08c710 + slot-1/2
// vtable-order anchors) was carved to src/Gruntz/State.cpp (unit "state", REHOME D7),
// which now stamps ??_7CState and hosts all 18 CState inline virtuals.
// See GameMode.h for the hierarchy. Names are placeholders; only offsets + code bytes
// are load-bearing.
//
// Functions in this TU (ascending retail-RVA order):
//   CBootyState::GenMenuRandPos @0x019cd0 - the per-selector edge-spawn RNG helper.
//   CState::LoadGruntEffectSprites @0x01a040 - preload the in-game effect sprite set.
//   CState::LevelMsgHudDriver @0x01a700 - the per-frame level-message HUD + explosion driver.
//   CGameModeBase::ResetPreview @0x0de140 (@interleaver -> levelpreview) / ::Reset
//     @0x0f9840 (@interleaver -> scattered COMDAT) - the base cleanup pair, out-of-line.
#include <Bute/SymTab.h>                  // CSymTab (LoadGruntEffectSprites m_30 ResolvePath)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // RemoveKeysEqual_157c70 (CGameModeBase::ResetPreview)
#include <Gruntz/SpriteRefTable.h>        // CSpriteRefTable (LoadGruntEffectSprites m_74 GetSel)
#include <Gruntz/GameMode.h>              // CState / CGameModeBase / CSpriteFactoryHolder
#include <Bute/ButeMgr.h>                 // CButeMgr g_buteMgr (SecretColor wormhole tint)
#include <Gruntz/SpriteFactory.h>         // CSpriteFactory (m_world->m_8 CreateSprite)
#include <Gruntz/UserLogic.h>             // CGameObject (the created effect sprites)
#include <Gruntz/WwdGameReg.h>            // g_gameReg (GenMenuRandPos Rand/RandRange)
#include <Gruntz/GameRegistry.h>          // CSpriteFactoryHolder (the real m_world class)
#include <Gruntz/Grunt.h>                 // GruntSoundCat full def (m_world->m_8 factory)
#include <Gruntz/SoundCue.h> // CSndSubMgr/CSndHost/CSndFinder/CSoundCueMgr (LevelMsgHudDriver cue)
#include <Gruntz/LeafCue.h>  // LeafCue (PlayIfElapsed_01f940 + m_10/m_14/m_18)
#include <rva.h>

// (CState's ??_G scalar-deleting dtor - and the `operator delete` it reaches - moved to
// src/Gruntz/State.cpp with the ctor; REHOME package D7.)

// The global game registry (canonical <Gruntz/WwdGameReg.h>): the Rand()/RandRange()
// __thiscall helpers GenMenuRandPos calls (all reloc-masked).
extern "C" WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A (reloc-masked)

// The glitter/effect factory chain (CGlitterMgr view of g_gameReg, *0x24556c): the
// selection source (m_74), the SecretColor->handle table (m_78), the sprite factory
// (m_world->m_8), the letter-count set (m_7c), the attract frame counter (m_80).
SIZE_UNKNOWN(CGlitterMgrM30);
struct CGlitterMgrM30 {
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08 the animation factory (CreateSprite @0x1597b0)
};
SIZE_UNKNOWN(CGlitterMgrSet);
struct CGlitterMgrSet {
    char m_pad00[0x4];
    i32 m_4; // +0x04 element count
};
SIZE_UNKNOWN(CGlitterColorTable);
struct CGlitterColorTable {
    char m_pad00[0x14];
    i32 m_arr14[1]; // +0x14  color->handle table
};
SIZE_UNKNOWN(CGlitterMgr);
struct CGlitterMgr {
    char m_pad00[0x30];
    CGlitterMgrM30* m_world; // +0x30
    char m_pad34[0x74 - 0x34];
    CSpriteRefTable* m_74;    // +0x74  selection source
    CGlitterColorTable* m_78; // +0x78  color->handle table
    CGlitterMgrSet* m_7c;     // +0x7c
    i32 m_80;                 // +0x80  attract frame counter (title rotation source)
};

// LoadGruntEffectSprites externs: the CButeMgr text-config singleton + the wormhole
// SecretColor bute tag + the go-kart install byte flag.
extern CButeMgr g_buteMgr;        // ?g_buteMgr@@3VCButeMgr@@A
extern char* g_wormholeSpawnKey;  // ?g_wormholeSpawnKey@@3PADA ("Wormhole" bute tag @0x60a7ac)
extern unsigned char g_dat60b588; // ?g_dat60b588@@3EA  (go-kart install byte flag)

// The geometry table (0x60b8fc, 0x10-byte rows): the effect sprites' x-position is
// (row.a + row.c) / 2. The loop init/bound relocs land on &row[0].c / &row[8].c.
SIZE_UNKNOWN(CEffGeomRow);
struct CEffGeomRow {
    i32 a;     // +0x00
    i32 pad4;  // +0x04 (417, unused)
    i32 c;     // +0x08
    i32 pad12; // +0x0c (245, unused)
};
DATA(0x0020b8fc)
extern CEffGeomRow g_effGeom[8]; // 0x60b8fc

// (the CEffLoaderSelf `this`-alias view is GONE - it WAS CBootyState. Every field it
// modeled sits in a CBootyState pad at the same offset, and the three it duplicated agree
// exactly (m_hudPhase@+0x1b4 == m_initGate, m_shownA@+0x284 == m_readyFlags,
// m_shownB@+0x2a4 == m_templateFlags); its top field ends at 0x31c, inside CBootyState's
// allocation-proven 0x320. The three methods that cast `this` to it are now CBootyState
// methods and use their own members directly - the `(CEffLoaderSelf*)this` casts, which
// were the symptom, fell out. See <Gruntz/GameMode.h> for the full proof.)

// ===========================================================================
// CBootyState::GenMenuRandPos (0x19cd0): a MEMBER whose body never touches `this` (so the
// callee is byte-identical to a __stdcall) - but its ONLY caller, BuildGruntSprintAnimation
// (0x19920), sets `mov ecx,ebp` right before the call, which only a member call emits.
// Generates a random
// {x,y} spawn position by edge, selected by `sel` (1..8). Rand() = signed game RNG;
// RandRange(0,N) = uniform [0,N).
// @early-stop
// regalloc coin-flip wall (~89%): all 8 cases + shared tails + idiv constants are
// byte-identical; the sole residual is outX/outY swapped between esi/edi (retail
// outX->edi/outY->esi, recompile outX->esi/outY->edi). A named-local pin
// (docs/patterns/pin-local-for-callee-saved-reg.md) did NOT flip it -> the pure
// allocator coin-flip that doc flags as the zero-register-pinning.md wall.
// ===========================================================================
RVA(0x00019cd0, 0x1df)
void CBootyState::GenMenuRandPos(i32 sel, i32* outX, i32* outY) {
    if (!outX || !outY) {
        return;
    }
    switch (sel) {
        case 1:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0x1e0;
            return;
        case 5:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0;
            return;
        case 3:
            *outX = 0;
            goto y_1e1;
        case 7:
            *outX = 0x280;
            goto y_1e1;
        y_1e1:
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case 2:
            if (g_gameReg->Rand() % 2) {
                *outX = 0;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141;
            *outY = 0x1e0;
            return;
        case 8:
            if (g_gameReg->Rand() % 2) {
                *outX = 0x280;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141 + 0x140;
            *outY = 0x1e0;
            return;
        y_f1:
            *outY = g_gameReg->Rand() % 0xf1 + 0xf0;
            return;
        case 4:
            if (g_gameReg->Rand() % 2) {
                *outX = g_gameReg->RandRange(0, 0x140);
                *outY = 0;
                return;
            }
            *outX = 0;
            goto y_f0;
        case 6:
            if (g_gameReg->RandRange(0, 1)) {
                *outX = g_gameReg->RandRange(0, 0x140) + 0x140;
                *outY = 0;
                return;
            }
            *outX = 0x280;
            goto y_f0;
        y_f0:
            *outY = g_gameReg->RandRange(0, 0xf0);
            return;
    }
}

// ===========================================================================
// CState::LoadGruntEffectSprites (0x1a040): preload the in-game effect/icon animation
// set. Really a CPlay-layout method (the trace homed it on the CState base); it walks
// the g_gameReg->m_world->m_8 SimpleAnimation factory and stores ~15 named effect
// sprites into the +0x2fc.. block plus three parallel 8-element sprite arrays at
// +0x224/+0x244/+0x264, positioned from the geometry table.
// @confidence: med
// @source: string-xref
// @early-stop
// ~96.3%: complete + correct, dev-authentic shape (natural array indexing throughout).
// Residual is two scheduling walls: (1) the SecretColor block schedules the
// g_gameReg->m_78 load AFTER the GetIntDef call (retail hoists it before); (2) the
// (a+c)/2 geom pair loads a/c in the opposite eax/edx order (commutative). All
// externs/strings named.
// ===========================================================================
RVA(0x0001a040, 0x55e)
i32 CBootyState::LoadGruntEffectSprites() {
    i32 handleA = g_gameReg->m_74->GetSel(0, 0);
    if (handleA == 0) {
        return 0;
    }
    i32 handleB = g_gameReg->m_74->GetSel(0, 1);

    void* img = m_gruntzBank->ResolvePath("IMAGEZ_GOKARTGRUNT");
    if (img == 0) {
        return 0;
    }
    m_c->m_10->InstallTree(img, "GRUNTZ_GOKARTGRUNT", (const char*)&g_dat60b588);

    CSpriteFactory* f = g_gameReg->m_world->m_8;

    CGameObject* sw = f->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[0] = sw;
    if (sw == 0) {
        return 0;
    }
    sw->ApplyName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    m_icons[0]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_icons[0]->m_stateFlags |= 1;

    CGameObject* wh = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[7] = wh;
    if (wh == 0) {
        return 0;
    }
    i32 tint = ((CGlitterMgr*)g_gameReg)
                   ->m_78->m_arr14[g_buteMgr.GetIntDef(g_wormholeSpawnKey, "SecretColor", 1)];
    m_icons[7]->ApplyName("GAME_WORMHOLE");
    m_icons[7]->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    CGameObject* p318 = m_icons[7];
    p318->m_drawActive = 1;
    p318->m_drawFillCmd = 7;
    p318->m_drawFillArg = tint;

    CGameObject* ex = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[1] = ex;
    if (ex == 0) {
        return 0;
    }
    ex->ApplyName("GRUNTZ_EXITZ");
    m_icons[1]->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
    CGameObject* p300 = m_icons[1];
    p300->m_drawActive = 1;
    p300->m_drawFillCmd = 0xa;
    p300->m_drawFillArg = handleA;
    m_icons[1]->m_stateFlags |= 1;

    CGameObject* dt = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[2] = dt;
    if (dt == 0) {
        return 0;
    }
    dt->ApplyName("GRUNTZ_NORMALGRUNT_DEATH");
    m_icons[2]->ApplyLookupGeometry("GAME_GRUNTTWITCH", 0);
    CGameObject* p304 = m_icons[2];
    p304->m_drawActive = 1;
    p304->m_drawFillCmd = 0xa;
    p304->m_drawFillArg = handleA;
    m_icons[2]->m_stateFlags |= 1;

    CGameObject* gl = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[3] = gl;
    if (gl == 0) {
        return 0;
    }
    gl->ApplyName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    m_icons[3]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p308 = m_icons[3];
    p308->m_drawActive = 1;
    p308->m_drawFillCmd = 0xa;
    p308->m_drawFillArg = handleA;
    m_icons[3]->m_stateFlags |= 1;

    CGameObject* bb = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[4] = bb;
    if (bb == 0) {
        return 0;
    }
    bb->ApplyName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    m_icons[4]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p30c = m_icons[4];
    p30c->m_drawActive = 1;
    p30c->m_drawFillCmd = 0xa;
    p30c->m_drawFillArg = handleA;
    m_icons[4]->m_stateFlags |= 1;

    CGameObject* rz = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[5] = rz;
    if (rz == 0) {
        return 0;
    }
    rz->ApplyName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    m_icons[5]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p310 = m_icons[5];
    p310->m_drawActive = 1;
    p310->m_drawFillCmd = 0xa;
    p310->m_drawFillArg = handleA;
    m_icons[5]->m_stateFlags |= 1;

    CGameObject* cn = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[6] = cn;
    if (cn == 0) {
        return 0;
    }
    cn->ApplyName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    m_icons[6]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p314 = m_icons[6];
    p314->m_drawActive = 1;
    p314->m_drawFillCmd = 0xa;
    p314->m_drawFillArg = handleA;
    m_icons[6]->m_stateFlags |= 1;

    // The three per-direction sprite arrays sit contiguously (bomb/go-kart/explosion),
    // positioned from the geometry table row's {a,c} midpoint; MSVC fuses the three
    // parallel array walks + the geom walk into single induction pointers.
    for (i32 i = 0; i < 8; i++) {
        CGameObject* b = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_bomb[i] = b;
        if (b == 0) {
            return 0;
        }
        b->ApplyName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        m_bomb[i]->ApplyLookupGeometry("GAME_GRUNTBOMBSPRINT", 0);
        CGameObject* bp = m_bomb[i];
        bp->m_drawActive = 1;
        bp->m_drawFillCmd = 0xa;
        bp->m_drawFillArg = handleA;
        m_bomb[i]->m_screenX = 0x2c6;
        m_bomb[i]->m_screenY = (g_effGeom[i].a + g_effGeom[i].c) / 2;
        m_bomb[i]->m_stateFlags |= 1;

        CGameObject* e = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_expl[i] = e;
        if (e == 0) {
            return 0;
        }
        e->ApplyName("GAME_EXPLOSION");
        m_expl[i]->m_stateFlags |= 1;

        CGameObject* g = g_gameReg->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_gokart[i] = g;
        if (g == 0) {
            return 0;
        }
        g->ApplyName("GRUNTZ_GOKARTGRUNT_EAST");
        m_gokart[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        CGameObject* gp = m_gokart[i];
        gp->m_drawActive = 1;
        gp->m_drawFillCmd = 0xa;
        gp->m_drawFillArg = handleB;
        m_gokart[i]->m_screenX = -70;
        m_gokart[i]->m_screenY = (g_effGeom[i].a + g_effGeom[i].c) / 2;
        m_gokart[i]->m_stateFlags |= 1;
    }
    return 1;
}

// ===========================================================================
// CState::LevelMsgHudDriver (0x1a700): the per-frame level-message HUD + explosion
// eye-candy driver (really a CBootyState/CPlay-layout method - the only caller is
// CBootyState::Render @0x1c210 on its own `this`, so it is __thiscall; Ghidra mislabeled
// it as the free `?LevelMsgHudDriver@@YAHXZ`). It runs the eight message slots: on the
// reveal pass (m_hudPhase == 0) it walks the current slot (m_slot), sliding the
// bomb/gokart sprites toward the icon and, as each crosses its box, popping the level-
// message text (rectsA) then the formatted stat line (rectsB) via ShowHudMessage and
// firing the explosion cue; on the drive/finalize pass (m_hudPhase != 0) it re-draws all
// slots and, once every slot has landed, latches the explosion sprites active.
//
// The message-slot .rdata tables (named so the DIR32 datum reloc-masks): rectsA the
// level-message text box, rectsB the stat/formatted-text box, iconPos the icon {x,y},
// strings the CString message set. ::CopyRect is the engine's CopyRect trampoline.
DATA(0x0020b838)
extern RECT g_levelMsgRectsA[8]; // 0x60b838  (shared with BootyMessages - stays extern)
// g_levelMsgIconPos ({x,y} icon-slide targets) is gamemode-private (extern-only) and
// sits cleanly between RectsA and RectsB (no overlap): DEFINED here (owner gamemode.obj's
// .data, real initializer) - REHOME DD-D. Subsumes the interior 0x20b8bc loop refs.
DATA(0x0020b8b8)
i32 g_levelMsgIconPos[16] = {
    0xea,
    0x80,
    0xec,
    0xae,
    0xeb,
    0xe3,
    0xe9,
    0x10b,
    0xe9,
    0x12f,
    0xe7,
    0x159,
    0xe8,
    0x17c,
    0xe9,
    0x1a8
}; // 0x60b8b8
DATA(0x0020b8f8)
extern RECT g_levelMsgRectsB[8]; // 0x60b8f8  (shared with BootyMessages - stays extern)
DATA(0x00229ef8)
extern CString g_levelMsgStrings[8]; // 0x629ef8
DATA(0x0021ab24)
extern i32 g_sndCueTag; // 0x61ab24
DATA(0x0021ab20)
extern i32 g_sndEnabled; // 0x61ab20
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // 0x6bf3c0

// ShowHudMessage (0x1154b0, glyphstr): draw a CString into a RECT via the render/HUD
// sink (m_c). FormatHudText (0x1af70, shared with CMenuState; called on this): fill the
// CString with the slot's formatted stat line. Both external, reloc-masked.
extern void ShowHudMessage(
    CSpriteFactoryHolder* sink,
    RECT* box,
    CString* text,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0x1154b0

// @early-stop
// /GX branchy megafunction wall (~complete reconstruction): the whole body - the reveal
// pass (m_hudPhase == 0) slot slide + rectsA/rectsB ShowHudMessage pops + explosion cue,
// and the drive pass (m_hudPhase != 0) redraw loop + explosion-active finalize - matches
// retail's logic (all externs/strings/tables named). Residual is the documented /GX
// EH-state numbering + the parallel-induction-pointer loop shape (retail hoists the
// rectsA/rectsB/iconPos/strings/icon walks into 6 induction pointers with a data-address
// bound `cmp ebp,offset g_effGeom`) that MSVC5 will not reproduce from counted C loops.
// Final-sweep candidate (docs/patterns/big-seh-fuzzy-desync.md; jumptable-data-overlap.md).
RVA(0x0001a700, 0x6b6)
i32 CBootyState::LevelMsgHudDriver() {
    if (m_initGate != 0) {
        // ---- drive/finalize pass ----
        if (m_slot == 8) {
            // every slot landed: latch the explosion sprites visible once their anim
            // sub-mgr reports active-but-not-idle, then done.
            for (i32 i = 0; i < 8; i++) {
                CGameObject* e = m_expl[i];
                if (e->m_1c8 != 0 && e->m_1c0 == 0) {
                    e->m_stateFlags |= 1;
                }
            }
            return 1;
        }
        // redraw every slot's level message (rectsA) + stat line (rectsB), sliding the
        // explosion sprite into place once the slot has scrolled far enough.
        i32 shown = 0;
        for (i32 i = 0; i < 8; i++) {
            RECT box;
            m_bomb[i]->m_stateFlags |= 1;
            m_gokart[i]->m_stateFlags |= 1;
            m_icons[i]->m_stateFlags &= ~1;
            m_icons[i]->m_screenX = g_levelMsgIconPos[i * 2];
            m_icons[i]->m_screenY = g_levelMsgIconPos[i * 2 + 1];
            ::CopyRect(&box, &g_levelMsgRectsA[i]);
            CString text = g_levelMsgStrings[i];
            m_templateFlags[i] = 1;
            ShowHudMessage(m_c, &box, &text, 0x78, 1, 0xff, 0xff, 0, 1);
            ::CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, i);
            m_readyFlags[i] = 1;
            ShowHudMessage(m_c, &box, &text, 0x78, 1, 0xff, 0xff, 0, 1);
            if (i >= m_slot && (i != m_slot || m_expl[i]->m_geoId == 0)) {
                CGameObject* e = m_expl[i];
                e->m_stateFlags &= ~1;
                e->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
                e->m_screenX = (g_levelMsgRectsB[i].right + g_levelMsgRectsB[i].left) / 2;
                e->m_screenY = (g_levelMsgRectsB[i].bottom + g_levelMsgRectsB[i].top) / 2 - 0x10;
                if (shown == 0) {
                    // the +0x30 holder cast to its REAL class (this TU's g_gameReg is
                    // the WwdGameReg facet whose m_world is still a glitter-view type)
                    CSndHost* host = ((CSpriteFactoryHolder*)g_gameReg->m_world)->m_28;
                    if (host->m_emitGate == 0) {
                        void* cue_ob = 0;
                        host->m_10.Lookup("GAME_EXPLOSION1", cue_ob);
                        LeafCue* cue = (LeafCue*)cue_ob;
                        if (cue != 0) {
                            cue->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);
                        }
                    }
                    shown = 1;
                }
            }
        }
        m_slot = 8;
        return 1;
    }

    // ---- reveal pass (m_hudPhase == 0) ----
    if (m_slot < 8) {
        if (m_slot == 0 && ((m_bomb[0]->m_stateFlags & 1) || (m_gokart[0]->m_stateFlags & 1))) {
            m_bomb[0]->m_stateFlags &= ~1;
            m_gokart[0]->m_stateFlags &= ~1;
        }
        m_bomb[m_slot]->m_screenX -= 10;
        i32 gx = m_gokart[m_slot]->m_screenX + 10;
        m_gokart[m_slot]->m_screenX = gx;
        i32 s = m_slot;
        if (m_templateFlags[s] == 0
            && (g_levelMsgRectsA[s].right + g_levelMsgRectsA[s].left) / 2 <= gx) {
            RECT box;
            m_templateFlags[s] = 1;
            ::CopyRect(&box, &g_levelMsgRectsA[m_slot]);
            CString text = g_levelMsgStrings[m_slot];
            m_templateFlags[m_slot] = 1;
            ShowHudMessage(m_c, &box, &text, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        s = m_slot;
        if (m_readyFlags[s] == 0 && g_levelMsgIconPos[s * 2] <= gx) {
            m_readyFlags[s] = 1;
            m_icons[m_slot]->m_stateFlags &= ~1;
            m_icons[m_slot]->m_screenX = g_levelMsgIconPos[m_slot * 2];
            m_icons[m_slot]->m_screenY = g_levelMsgIconPos[m_slot * 2 + 1];
        }
    }
    // latch the already-landed explosion sprites active.
    for (i32 j = 0; j < m_slot; j++) {
        CGameObject* e = m_expl[j];
        if (e->m_1c8 != 0 && e->m_1c0 == 0) {
            e->m_stateFlags |= 1;
        }
    }
    // finalize the slots from m_slot onward once the bomb/gokart pair has crossed.
    for (i32 i = m_slot; i < 8; i++) {
        if (m_bomb[i]->m_screenX <= m_gokart[i]->m_screenX) {
            RECT box;
            CString text;
            ::CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, i);
            m_readyFlags[i] = 1;
            ShowHudMessage(m_c, &box, &text, 0x78, 1, 0xff, 0xff, 0, 1);
            CGameObject* e = m_expl[i];
            e->m_stateFlags &= ~1;
            e->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
            e->m_screenX = (g_levelMsgRectsB[i].left + g_levelMsgRectsB[i].right) / 2;
            e->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2 - 0x10;
            m_bomb[i]->m_stateFlags |= 1;
            m_gokart[i]->m_stateFlags |= 1;
            m_slot++;
            CSndHost* host = ((CSpriteFactoryHolder*)g_gameReg->m_world)->m_28;
            if (host->m_emitGate == 0) {
                void* cue_ob = 0;
                host->m_10.Lookup("GAME_EXPLOSION1", cue_ob);
                LeafCue* cue = (LeafCue*)cue_ob;
                if (cue != 0 && g_sndEnabled != 0
                    && (u32)(g_killCueClock - cue->m_14) >= (u32)cue->m_18) {
                    cue->m_14 = g_killCueClock;
                    cue->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                }
            }
            if (m_slot >= 8) {
                return 1;
            }
            m_bomb[m_slot]->m_stateFlags &= ~1;
            m_gokart[m_slot]->m_stateFlags &= ~1;
        }
    }
    return 0;
}

// ===========================================================================
// CState - the base game-state class. Its ctor (0x0008c750) + slot-0 scalar-deleting
// dtor ??_G (0x0008c710) + the slot-1/2 vtable-order anchors were carved to their own
// obj src/Gruntz/State.cpp (REHOME package D7) - that TU stamps ??_7CState. The two
// CState methods above (LoadGruntEffectSprites / LevelMsgHudDriver) are NON-virtual
// out-of-line members, so this TU no longer emits the CState vtable.
// ===========================================================================

// ===========================================================================
// CGameModeBase cleanup pair (the base the game-state classes chain their teardown to).
// Stop the owned sound (SoundStream::Stop), clear/prune the sub-manager map, then
// BaseCleanup. m_c->m_28 is re-read each statement (retail does not cache it).
// ===========================================================================

// 0x0de140 - ResetPreview: prune the PREVIEW-prefixed keys instead of clearing.
// @interleaver CGameModeBase::ResetPreview emitted-in levelpreview
// Lone CGameModeBase method inside the levelpreview .text obj block (CPreviewState::Enter
// @0x0de030 precedes it, CPreviewState::NextScreenCmd_0de190 follows it - both levelpreview).
// Homing into LevelPreview.cpp is deferred (CGameModeBase not modeled there); flagged
// in-host per REHOME rule (c). REHOME package D7.
// @early-stop
// ~98.8% - m_28-intermediate regalloc wall (retail reuses eax->eax->ecx; cl picks fresh
// ecx/edx) - a 2-3 byte modrm micro-diff, not source-steerable.
extern char s_PREVIEW_6135e8[]; // "PREVIEW" (bound in Globals.cpp; reloc-masked)
RVA(0x000de140, 0x33)
void CGameModeBase::ResetPreview() {
    if (m_c->m_28->m_2c != 0) {
        ((SoundStream*)m_c->m_28->m_2c)->Stop();
    }
    m_c->m_28->RemoveKeysEqual_157c70(s_PREVIEW_6135e8, "_");
    BaseCleanup();
}

// 0x0f9840 - Reset: ClearMap the whole sub-manager map.
// @interleaver CGameModeBase::Reset emitted-in <scattered COMDAT; home ambiguous>
// Lone CGameModeBase method at a unit BOUNDARY (CSplashState::LoadSounds @0x0f9780 before,
// CTitleApp::OnStart @0x0f9880 after) - a separate 1-fn obj whose owning TU the xref does
// not crack (its two siblings BaseCleanup @0xfa150 and ResetPreview @0xde140 are each in
// yet-other objs). Kept in-host with its RVA name; owner-recovery deferred. REHOME D7.
// @early-stop
// ~98.7% - same m_28-intermediate regalloc wall as ResetPreview.
RVA(0x000f9840, 0x29)
void CGameModeBase::Reset() {
    if (m_c->m_28->m_2c != 0) {
        ((SoundStream*)m_c->m_28->m_2c)->Stop();
    }
    m_c->m_28->ClearMap();
    BaseCleanup();
}

#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (full def)
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Bute/SymTab.h>                  // CSymTab (LoadGruntEffectSprites m_30 ResolvePath)
#include <Gruntz/SoundState.h>            // g_sndEnabled/g_sndCueTag
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // the m_c->m_soundRegistry leaf-scan facet
#include <Gruntz/SpriteRefTable.h>        // CSpriteRefTable (LoadGruntEffectSprites m_74 GetSel)
#include <Gruntz/GameMode.h>              // CState / CDDrawSurfaceMgr
#include <Bute/ButeMgr.h>                 // CButeMgr g_buteMgr (SecretColor wormhole tint)
#include <DDrawMgr/DDrawChildGroup.h>     // CDDrawChildGroup (m_world->m_childGroup CreateSprite)
#include <Gruntz/UserLogic.h>             // CGameObject (the created effect sprites)
#include <Gruntz/WwdGameReg.h>
#include <Gruntz/LightFxMgr.h> // m_78->m_tables (the glitter handle table)            // g_gameReg (GenMenuRandPos Rand/RandRange)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (the real m_world class)
#include <Gruntz/Grunt.h>        // GruntSoundCat full def (m_world->m_childGroup factory)
#include <Gruntz/SoundCue.h> // CSndSubMgr/CDDrawSubMgrLeafScan/CSndFinder/DSoundCloneInst (LevelMsgHudDriver cue)
#include <Gruntz/LeafCue.h> // LeafCue (PlayIfElapsed + m_10/m_14/m_18)
#include <rva.h>
#include <Gruntz/BootyMessages.h> // g_levelMsgRectsA (ex .cpp extern)

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

// ===========================================================================
// CBootyState::GenMenuRandPos (0x19cd0): a MEMBER whose body never touches `this` (so the
// callee is byte-identical to a __stdcall) - but its ONLY caller, BuildGruntSprintAnimation
// (0x19920), sets `mov ecx,ebp` right before the call, which only a member call emits.
// Generates a random
// {x,y} spawn position by edge, selected by `sel` (1..8). Rand() = signed game RNG;
// RandRange(0,N) = uniform [0,N).
// @early-stop
#include <Gruntz/GlyphStringDraw.h> // ShowHudMessage (ex .cpp extern)
RVA(0x00019cd0, 0x200)
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
// the g_gameReg->m_world->m_childGroup SimpleAnimation factory and stores ~15 named effect
// sprites into the +0x2fc.. block plus three parallel 8-element sprite arrays at
// +0x224/+0x244/+0x264, positioned from the geometry table.
// @confidence: med
// @source: string-xref
// @early-stop
RVA(0x0001a040, 0x55e)
i32 CBootyState::LoadGruntEffectSprites() {
    CShadeTable* handleA = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (handleA == 0) {
        return 0;
    }
    CShadeTable* handleB = g_gameReg->m_spriteFactory->GetSel(0, 1);

    void* img = m_gruntzBank->ResolvePath("IMAGEZ_GOKARTGRUNT");
    if (img == 0) {
        return 0;
    }
    m_world->m_imageRegistry->InstallTree(img, "GRUNTZ_GOKARTGRUNT", "_");

    CDDrawChildGroup* f = g_gameReg->m_world->m_childGroup;

    CWwdGameObjectA* sw = f->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[0] = sw;
    if (sw == 0) {
        return 0;
    }
    sw->ApplyName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    m_icons[0]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_icons[0]->m_stateFlags |= 1;

    CWwdGameObjectA* wh =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[7] = wh;
    if (wh == 0) {
        return 0;
    }
    CShadeTable* tint =
        g_gameReg->m_logicPump->m_tables[g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1)];
    m_icons[7]->ApplyName("GAME_WORMHOLE");
    m_icons[7]->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    CWwdGameObjectA* icon7 = m_icons[7];
    icon7->m_drawActive = 1;
    icon7->m_drawFillCmd = 7;
    icon7->m_drawFillArg = tint;

    CWwdGameObjectA* ex =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[1] = ex;
    if (ex == 0) {
        return 0;
    }
    ex->ApplyName("GRUNTZ_EXITZ");
    m_icons[1]->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
    CWwdGameObjectA* icon1 = m_icons[1];
    icon1->m_drawActive = 1;
    icon1->m_drawFillCmd = 0xa;
    icon1->m_drawFillArg = handleA;
    m_icons[1]->m_stateFlags |= 1;

    CWwdGameObjectA* dt =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[2] = dt;
    if (dt == 0) {
        return 0;
    }
    dt->ApplyName("GRUNTZ_NORMALGRUNT_DEATH");
    m_icons[2]->ApplyLookupGeometry("GAME_GRUNTTWITCH", 0);
    CWwdGameObjectA* icon2 = m_icons[2];
    icon2->m_drawActive = 1;
    icon2->m_drawFillCmd = 0xa;
    icon2->m_drawFillArg = handleA;
    m_icons[2]->m_stateFlags |= 1;

    CWwdGameObjectA* gl =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[3] = gl;
    if (gl == 0) {
        return 0;
    }
    gl->ApplyName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    m_icons[3]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon3 = m_icons[3];
    icon3->m_drawActive = 1;
    icon3->m_drawFillCmd = 0xa;
    icon3->m_drawFillArg = handleA;
    m_icons[3]->m_stateFlags |= 1;

    CWwdGameObjectA* bb =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[4] = bb;
    if (bb == 0) {
        return 0;
    }
    bb->ApplyName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    m_icons[4]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* p30c = m_icons[4];
    p30c->m_drawActive = 1;
    p30c->m_drawFillCmd = 0xa;
    p30c->m_drawFillArg = handleA;
    m_icons[4]->m_stateFlags |= 1;

    CWwdGameObjectA* rz =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[5] = rz;
    if (rz == 0) {
        return 0;
    }
    rz->ApplyName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    m_icons[5]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon5 = m_icons[5];
    icon5->m_drawActive = 1;
    icon5->m_drawFillCmd = 0xa;
    icon5->m_drawFillArg = handleA;
    m_icons[5]->m_stateFlags |= 1;

    CWwdGameObjectA* cn =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[6] = cn;
    if (cn == 0) {
        return 0;
    }
    cn->ApplyName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    m_icons[6]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon6 = m_icons[6];
    icon6->m_drawActive = 1;
    icon6->m_drawFillCmd = 0xa;
    icon6->m_drawFillArg = handleA;
    m_icons[6]->m_stateFlags |= 1;

    // The three per-direction sprite arrays sit contiguously (bomb/go-kart/explosion),
    // positioned from the geometry table row's {a,c} midpoint; MSVC fuses the three
    // parallel array walks + the geom walk into single induction pointers.
    for (i32 i = 0; i < 8; i++) {
        CWwdGameObjectA* b =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_bomb[i] = b;
        if (b == 0) {
            return 0;
        }
        b->ApplyName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        m_bomb[i]->ApplyLookupGeometry("GAME_GRUNTBOMBSPRINT", 0);
        CWwdGameObjectA* bp = m_bomb[i];
        bp->m_drawActive = 1;
        bp->m_drawFillCmd = 0xa;
        bp->m_drawFillArg = handleA;
        m_bomb[i]->m_screenX = 0x2c6;
        m_bomb[i]->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2;
        m_bomb[i]->m_stateFlags |= 1;

        CWwdGameObjectA* e =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_expl[i] = e;
        if (e == 0) {
            return 0;
        }
        e->ApplyName("GAME_EXPLOSION");
        m_expl[i]->m_stateFlags |= 1;

        CWwdGameObjectA* g =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_gokart[i] = g;
        if (g == 0) {
            return 0;
        }
        g->ApplyName("GRUNTZ_GOKARTGRUNT_EAST");
        m_gokart[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        CWwdGameObjectA* gp = m_gokart[i];
        gp->m_drawActive = 1;
        gp->m_drawFillCmd = 0xa;
        gp->m_drawFillArg = handleB;
        m_gokart[i]->m_screenX = -70;
        m_gokart[i]->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2;
        m_gokart[i]->m_stateFlags |= 1;
    }
    return 1;
}

// @early-stop
RVA(0x0001a700, 0x6b6)
i32 CBootyState::LevelMsgHudDriver() {
    if (m_initGate != 0) {
        // ---- drive/finalize pass ----
        if (m_slot == 8) {
            // every slot landed: latch the explosion sprites visible once their anim
            // sub-mgr reports active-but-not-idle, then done.
            for (i32 i = 0; i < 8; i++) {
                CWwdGameObjectA* e = m_expl[i];
                if (e->m_1a0.m_28 != 0 && e->m_1a0.m_20 == 0) {
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
            CopyRect(&box, &g_levelMsgRectsA[i]);
            CString text = g_levelMsgStrings[i];
            m_templateFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, i);
            m_readyFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            if (i >= m_slot && (i != m_slot || m_expl[i]->m_1a0.m_14 == 0)) {
                CWwdGameObjectA* e = m_expl[i];
                e->m_stateFlags &= ~1;
                e->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
                e->m_screenX = (g_levelMsgRectsB[i].right + g_levelMsgRectsB[i].left) / 2;
                e->m_screenY = (g_levelMsgRectsB[i].bottom + g_levelMsgRectsB[i].top) / 2 - 0x10;
                if (shown == 0) {
                    // the +0x30 holder cast to its REAL class (this TU's g_gameReg is
                    // the WwdGameReg facet whose m_world is still a glitter-view type)
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* cue_ob = 0;
                        host->m_10.Lookup("GAME_EXPLOSION1", cue_ob);
                        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
                        if (cue != 0) {
                            cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
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
        // `gx >= mid`, not `mid <= gx`: retail's left `cmp` operand is gx and the skip is
        // `jl` (0x1aa80). The two spellings are identical in C and differ in both.
        if (m_templateFlags[s] == 0
            && gx >= (g_levelMsgRectsA[s].right + g_levelMsgRectsA[s].left) / 2) {
            RECT box;
            m_templateFlags[s] = 1;
            CopyRect(&box, &g_levelMsgRectsA[m_slot]);
            CString text = g_levelMsgStrings[m_slot];
            m_templateFlags[m_slot] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        s = m_slot;
        if (m_readyFlags[s] == 0 && gx >= g_levelMsgIconPos[s * 2]) {
            m_readyFlags[s] = 1;
            m_icons[m_slot]->m_stateFlags &= ~1;
            m_icons[m_slot]->m_screenX = g_levelMsgIconPos[m_slot * 2];
            m_icons[m_slot]->m_screenY = g_levelMsgIconPos[m_slot * 2 + 1];
        }
    }
    // latch the already-landed explosion sprites active.
    for (i32 j = 0; j < m_slot; j++) {
        CWwdGameObjectA* e = m_expl[j];
        if (e->m_1a0.m_28 != 0 && e->m_1a0.m_20 == 0) {
            e->m_stateFlags |= 1;
        }
    }
    // finalize the slots from m_slot onward once the bomb/gokart pair has crossed.
    for (i32 i = m_slot; i < 8; i++) {
        if (m_gokart[i]->m_screenX >= m_bomb[i]->m_screenX) {
            RECT box;
            CString text;
            CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, i);
            m_readyFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            CWwdGameObjectA* e = m_expl[i];
            e->m_stateFlags &= ~1;
            e->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
            e->m_screenX = (g_levelMsgRectsB[i].left + g_levelMsgRectsB[i].right) / 2;
            e->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2 - 0x10;
            m_bomb[i]->m_stateFlags |= 1;
            m_gokart[i]->m_stateFlags |= 1;
            m_slot++;
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* cue_ob = 0;
                host->m_10.Lookup("GAME_EXPLOSION1", cue_ob);
                LeafCue* cue = static_cast<LeafCue*>(cue_ob);
                if (cue != 0 && g_sndEnabled != 0
                    && static_cast<u32>((g_killCueClock - cue->m_14))
                           >= static_cast<u32>(cue->m_18)) {
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

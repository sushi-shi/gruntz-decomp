#include <rva.h>

#include <Gruntz/GameMode.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/BootyMessages.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameReg.h>
#include <Rez/FrameClock.h>

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
};

// @early-stop
#include <Gruntz/GlyphStringDraw.h>
#include <Mfc.h>
#include <Gruntz/Random.h>
#include <Gruntz/GruntDirection.h>
#include <Wap32/ScreenGeometry.h>
RVA(0x00019cd0, 0x200)
void CBootyState::GenMenuRandPos(GruntDirection sel, i32* outX, i32* outY) {
    if (!outX || !outY) {
        return;
    }
    switch (sel) {
        case DIR_NORTH:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = SCREEN_H_PX;
            return;
        case DIR_SOUTH:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0;
            return;
        case DIR_EAST:
            *outX = 0;
            goto y_1e1;
        case DIR_WEST:
            *outX = SCREEN_W_PX;
            goto y_1e1;
        y_1e1:
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case DIR_NORTHEAST:
            if (g_gameReg->Rand() % 2) {
                *outX = 0;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141;
            *outY = SCREEN_H_PX;
            return;
        case DIR_NORTHWEST:
            if (g_gameReg->Rand() % 2) {
                *outX = SCREEN_W_PX;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141 + SCREEN_HALF_W_PX;
            *outY = SCREEN_H_PX;
            return;
        y_f1:
            *outY = g_gameReg->Rand() % 0xf1 + SCREEN_HALF_H_PX;
            return;
        case DIR_SOUTHEAST:
            if (g_gameReg->Rand() % 2) {
                *outX = g_gameReg->RandRange(0, SCREEN_HALF_W_PX);
                *outY = 0;
                return;
            }
            *outX = 0;
            goto y_f0;
        case DIR_SOUTHWEST:
            if (g_gameReg->RandRange(0, 1)) {
                *outX = g_gameReg->RandRange(0, SCREEN_HALF_W_PX) + SCREEN_HALF_W_PX;
                *outY = 0;
                return;
            }
            *outX = SCREEN_W_PX;
            goto y_f0;
        y_f0:
            *outY = g_gameReg->RandRange(0, SCREEN_HALF_H_PX);
            return;
    }
}

// @early-stop
RVA(0x00019f50, 0xb2)
i32 CGruntzMgr::RandRange(i32 lo, i32 hi) {
    i32 span = hi - lo + 1;
    i32 seed;
    if (span == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            return lo;
        }
        return hi;
    }
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return lo + ((g_randSeed >> 0x10) & 0x7fff) % span;
}

// @interleaver Rng2Next - 70 B lone body at 0x15cbe0, between Deserialize
// (wwdfactoryobject) and GetFrame (wwdfactoryobject): a first-use placement.

RVA(0x0001a040, 0x55e)
i32 CBootyState::LoadGruntEffectSprites() {
    CShadeTable* handleA = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (handleA == NULL) {
        return 0;
    }
    CShadeTable* handleB = g_gameReg->m_spriteFactory->GetSel(0, 1);

    void* img = m_gruntzBank->ResolvePath("IMAGEZ_GOKARTGRUNT");
    if (img == NULL) {
        return 0;
    }
    m_world->m_imageRegistry->InstallTree(img, "GRUNTZ_GOKARTGRUNT", "_");

    CDDrawChildGroup* f = g_gameReg->m_world->m_childGroup;

    CWwdGameObjectA* sw = f->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[0] = sw;
    if (sw == NULL) {
        return 0;
    }
    sw->ApplyName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    m_icons[0]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_icons[0]->m_stateFlags |= 1;

    CWwdGameObjectA* wh =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[7] = wh;
    if (wh == NULL) {
        return 0;
    }
    CShadeTable* tint =
        g_gameReg->m_logicPump->m_tables[g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1)];
    m_icons[7]->ApplyName("GAME_WORMHOLE");
    m_icons[7]->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    CWwdGameObjectA* icon7 = m_icons[7];
    icon7->m_drawActive = 1;
    icon7->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    icon7->m_drawFillArg = tint;

    CWwdGameObjectA* ex =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[1] = ex;
    if (ex == NULL) {
        return 0;
    }
    ex->ApplyName("GRUNTZ_EXITZ");
    m_icons[1]->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
    CWwdGameObjectA* icon1 = m_icons[1];
    icon1->m_drawActive = 1;
    icon1->m_drawFillCmd = SHADE_PAL_16;
    icon1->m_drawFillArg = handleA;
    m_icons[1]->m_stateFlags |= 1;

    CWwdGameObjectA* dt =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[2] = dt;
    if (dt == NULL) {
        return 0;
    }
    dt->ApplyName("GRUNTZ_NORMALGRUNT_DEATH");
    m_icons[2]->ApplyLookupGeometry("GAME_GRUNTTWITCH", 0);
    CWwdGameObjectA* icon2 = m_icons[2];
    icon2->m_drawActive = 1;
    icon2->m_drawFillCmd = SHADE_PAL_16;
    icon2->m_drawFillArg = handleA;
    m_icons[2]->m_stateFlags |= 1;

    CWwdGameObjectA* gl =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[3] = gl;
    if (gl == NULL) {
        return 0;
    }
    gl->ApplyName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    m_icons[3]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon3 = m_icons[3];
    icon3->m_drawActive = 1;
    icon3->m_drawFillCmd = SHADE_PAL_16;
    icon3->m_drawFillArg = handleA;
    m_icons[3]->m_stateFlags |= 1;

    CWwdGameObjectA* bb =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[4] = bb;
    if (bb == NULL) {
        return 0;
    }
    bb->ApplyName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    m_icons[4]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* p30c = m_icons[4];
    p30c->m_drawActive = 1;
    p30c->m_drawFillCmd = SHADE_PAL_16;
    p30c->m_drawFillArg = handleA;
    m_icons[4]->m_stateFlags |= 1;

    CWwdGameObjectA* rz =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[5] = rz;
    if (rz == NULL) {
        return 0;
    }
    rz->ApplyName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    m_icons[5]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon5 = m_icons[5];
    icon5->m_drawActive = 1;
    icon5->m_drawFillCmd = SHADE_PAL_16;
    icon5->m_drawFillArg = handleA;
    m_icons[5]->m_stateFlags |= 1;

    CWwdGameObjectA* cn =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[6] = cn;
    if (cn == NULL) {
        return 0;
    }
    cn->ApplyName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    m_icons[6]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon6 = m_icons[6];
    icon6->m_drawActive = 1;
    icon6->m_drawFillCmd = SHADE_PAL_16;
    icon6->m_drawFillArg = handleA;
    m_icons[6]->m_stateFlags |= 1;

    for (i32 i = 0; i < 8; i++) {
        CWwdGameObjectA* b =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_bomb[i] = b;
        if (b == NULL) {
            return 0;
        }
        b->ApplyName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        m_bomb[i]->ApplyLookupGeometry("GAME_GRUNTBOMBSPRINT", 0);
        CWwdGameObjectA* bp = m_bomb[i];
        bp->m_drawActive = 1;
        bp->m_drawFillCmd = SHADE_PAL_16;
        bp->m_drawFillArg = handleA;
        m_bomb[i]->m_screenX = 0x2c6;
        m_bomb[i]->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2;
        m_bomb[i]->m_stateFlags |= 1;

        CWwdGameObjectA* e =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_expl[i] = e;
        if (e == NULL) {
            return 0;
        }
        e->ApplyName("GAME_EXPLOSION");
        m_expl[i]->m_stateFlags |= 1;

        CWwdGameObjectA* g =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_gokart[i] = g;
        if (g == NULL) {
            return 0;
        }
        g->ApplyName("GRUNTZ_GOKARTGRUNT_EAST");
        m_gokart[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        CWwdGameObjectA* gp = m_gokart[i];
        gp->m_drawActive = 1;
        gp->m_drawFillCmd = SHADE_PAL_16;
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

        if (m_slot == 8) {

            for (i32 i = 0; i < 8; i++) {
                CWwdGameObjectA* e = m_expl[i];
                if (e->m_animCursor.m_finished != 0 && e->m_animCursor.m_frameTicksLeft == 0) {
                    e->m_stateFlags |= 1;
                }
            }
            return 1;
        }

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
            this->FormatHudText(&text, static_cast<BootyStatRow>(i));
            m_readyFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            if (i >= m_slot && (i != m_slot || m_expl[i]->m_animCursor.m_animation == NULL)) {
                CWwdGameObjectA* e = m_expl[i];
                e->m_stateFlags &= ~1;
                e->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
                e->m_screenX = (g_levelMsgRectsB[i].right + g_levelMsgRectsB[i].left) / 2;
                e->m_screenY = (g_levelMsgRectsB[i].bottom + g_levelMsgRectsB[i].top) / 2 - 0x10;
                if (shown == 0) {

                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* cue_ob = 0;
                        host->m_cues.Lookup("GAME_EXPLOSION1", cue_ob);
                        LeafCue* cue = static_cast<LeafCue*>(cue_ob);
                        if (cue != NULL) {
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

    for (i32 j = 0; j < m_slot; j++) {
        CWwdGameObjectA* e = m_expl[j];
        if (e->m_animCursor.m_finished != 0 && e->m_animCursor.m_frameTicksLeft == 0) {
            e->m_stateFlags |= 1;
        }
    }

    for (i32 i = m_slot; i < 8; i++) {
        if (m_gokart[i]->m_screenX >= m_bomb[i]->m_screenX) {
            RECT box;
            CString text;
            CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, static_cast<BootyStatRow>(i));
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
                host->m_cues.Lookup("GAME_EXPLOSION1", cue_ob);
                LeafCue* cue = static_cast<LeafCue*>(cue_ob);
                if (cue != NULL && g_sndEnabled != 0
                    && static_cast<u32>((g_killCueClock - cue->m_lastPlayTime))
                           >= static_cast<u32>(cue->m_replayDelay)) {
                    cue->m_lastPlayTime = g_killCueClock;
                    cue->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
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

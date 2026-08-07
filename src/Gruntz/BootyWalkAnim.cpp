#include <rva.h>

#include <Gruntz/BootyWalkAnim.h>

#include <Mfc.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BzState.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WarpLetter.h>
#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>

// @early-stop
RVA(0x0001b450, 0x1ac)
i32 CBootyState::BuildBootyWalkingGruntz() {
    if (g_gameReg->m_scoreHud->m_isCustomLevel != 0) {
        return 1;
    }
    if (g_gameReg->m_scoreHud->m_count > IDX(QUESTLEVEL_LAST)) {
        return 1;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (sel == NULL) {
        return 0;
    }
    for (i32 i = 0; i < WARPLETTER_COUNT; i++) {
        m_animSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 1, "SimpleAnimation", 3);
        if (m_animSprites[i] == NULL) {
            return 0;
        }
        m_animSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_NORTH_WALK");
        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_WALK", 0);
        m_animSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
        m_animSprites[i]->m_drawActive = 1;
        m_animSprites[i]->m_drawFillCmd = SHADE_PAL_16;
        m_animSprites[i]->m_drawFillArg = sel;
        m_visSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 1, "SimpleAnimation", 3);
        if (m_visSprites[i] == NULL) {
            return 0;
        }
        static CString buf;
        const char* prefix =
            (i < (g_gameReg->m_scoreHud->m_count - 1) % 4 + 1) ? "GAME_INGAMEICONZ_" : "BOOTY_DIM";
        buf.Format("%sSECRET%c", prefix, g_secretChars[i]);
        m_visSprites[i]->ApplyName(buf);
        m_visSprites[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        m_visSprites[i]->m_screenX = g_idleSpriteIds[i] + 0xfa;
        m_visSprites[i]->m_screenY = 0xdc;
    }
    return 1;
}

// @early-stop
RVA(0x0001b690, 0x7e0)
i32 CBootyState::UpdateBootyWalkingGruntz() {
    CBattlezData* rec = g_gameReg->m_scoreHud;
    if (rec->m_isCustomLevel != 0) {
        return 1;
    }
    i32 n = rec->m_count;
    if (n > 0x24) {
        return 1;
    }
    if (m_stepIndex >= WARPLETTER_COUNT) {
        return 1;
    }

    if (m_initGate != 0) {

        if (n < 0x24) {
            for (i32 i = 0; i < WARPLETTER_COUNT; i++) {
                if (i <= (g_gameReg->m_scoreHud->m_count - 1) % 4) {
                    m_visSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                    m_animSprites[i]->m_screenX = g_idleSpriteIds[i];
                    m_animSprites[i]->m_screenY = 0xdc;
                    m_animSprites[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                    if ((g_gameReg->m_scoreHud)->GetRecordValue(i) == 0) {
                        m_animSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    } else {
                        CString letter;
                        switch (static_cast<WarpLetter>(i)) {
                            case WARPLETTER_W:
                                letter = "W";
                                break;
                            case WARPLETTER_A:
                                letter = "A";
                                break;
                            case WARPLETTER_R:
                                letter = "R";
                                break;
                            case WARPLETTER_P:
                                letter = "P";
                                break;
                        }
                        m_animSprites[i]->ApplyName("GRUNTZ_PICKUPS");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    }
                } else {
                    m_visSprites[i]->m_screenX = g_idleSpriteIds[i];
                    m_visSprites[i]->m_screenY = 0xdc;
                    m_visSprites[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                    m_animSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                }
            }
        }
        m_stepIndex = 4;
        return 1;
    }

    if (m_visSprites[0]->m_screenX != g_idleSpriteIds[0]) {
        for (i32 k = 0; k < 4; k++) {
            m_visSprites[k]->m_screenX -= 10;
        }
    }
    if (m_stepIndex == 0 && HAS(m_animSprites[0]->m_stateFlags, SPRITE_STATE_HIDDEN)) {
        m_animSprites[0]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_animSprites[0]->m_screenX = g_idleSpriteIds[0];
        m_animSprites[0]->m_screenY = 0x1f4;
    }

    if (m_soundStarted == 0 && m_animSprites[m_stepIndex]->m_screenY <= 0x195) {
        if ((g_gameReg->m_scoreHud)->GetRecordValue(m_stepIndex) == 0) {
            m_soundStarted = 1;
            CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
            if (ss->m_emitGate == 0) {
                LeafCue* res = 0;
                MapLookup(ss->m_cues, "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", res);
                if (res != NULL) {
                    res->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }

    if (m_soundStarted != 0) {
        CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
        LeafCue* res = 0;
        MapLookup(ss->m_cues, "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", res);
        if (res == NULL) {
            return 1;
        }
        if (res->m_sound->IsPlaying() != 0) {
            m_visSprites[m_stepIndex]->m_stateFlags ^= SPRITE_STATE_HIDDEN;
        } else {
            m_visSprites[m_stepIndex]->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
    }

    if (m_walkStarted == 0 && m_animSprites[m_stepIndex]->m_screenY <= 0xdc) {
        {
            CString letter;
            switch (static_cast<WarpLetter>(m_stepIndex)) {
                case WARPLETTER_W:
                    letter = "W";
                    break;
                case WARPLETTER_A:
                    letter = "A";
                    break;
                case WARPLETTER_R:
                    letter = "R";
                    break;
                case WARPLETTER_P:
                    letter = "P";
                    break;
            }
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(0, 0);
            if (sel != NULL) {
                if ((g_gameReg->m_scoreHud)->GetRecordValue(m_stepIndex) != 0) {
                    CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
                    if (ss->m_emitGate == 0) {
                        LeafCue* res = 0;
                        MapLookup(ss->m_cues, "GAME_FLAGRISE", res);
                        if (res != NULL && g_sndEnabled != 0) {
                            u32 clock = g_killCueClock;
                            if (clock - res->m_lastPlayTime >= res->m_replayDelay) {
                                res->m_lastPlayTime = clock;
                                res->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                            }
                        }
                    }
                    m_animSprites[m_stepIndex]->ApplyName("GRUNTZ_PICKUPS");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    CWwdGameObjectA* g = m_animSprites[m_stepIndex];
                    g->m_drawActive = 1;
                    g->m_drawFillCmd = SHADE_PAL_16;
                    g->m_drawFillArg = sel;
                    m_visSprites[m_stepIndex]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                    g_gameReg->m_cueSink
                        ->SpawnVoiceDriver(0, 0x3bf, GetRandomNumber() % 0x11, 1, -1, -1);
                    m_walkStarted = 1;
                } else {
                    m_animSprites[m_stepIndex]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    CWwdGameObjectA* g = m_animSprites[m_stepIndex];
                    g->m_drawActive = 1;
                    g->m_drawFillCmd = SHADE_PAL_16;
                    g->m_drawFillArg = sel;
                    m_visSprites[m_stepIndex]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                    m_stepIndex++;
                    g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x441, 0, 1, -1, -1);
                    if (m_stepIndex == g_gameReg->m_scoreHud->m_count % 4) {
                        m_stepIndex = 4;
                        return 1;
                    }
                    if (m_stepIndex < 4) {
                        m_animSprites[m_stepIndex]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                        m_animSprites[m_stepIndex]->m_screenX = g_idleSpriteIds[m_stepIndex];
                        m_animSprites[m_stepIndex]->m_screenY = 0x1f4;
                        m_soundStarted = 0;
                        m_walkStarted = 0;
                    }
                }
            }
        }
    } else if (m_walkStarted != 0) {

        CWwdGameObjectA* spr = m_animSprites[m_stepIndex];
        if (spr->m_animCursor.m_finished != 0 && spr->m_animCursor.m_frameTicksLeft == 0) {
            m_stepIndex++;
            if (m_stepIndex == g_gameReg->m_scoreHud->m_count % 4) {
                m_stepIndex = 4;
                return 1;
            }
            if (m_stepIndex < 4) {
                m_animSprites[m_stepIndex]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                m_animSprites[m_stepIndex]->m_screenX = g_idleSpriteIds[m_stepIndex];
                m_animSprites[m_stepIndex]->m_screenY = 0x1f4;
                m_walkStarted = 0;
                m_soundStarted = 0;
            }
        }
    } else {
        m_animSprites[m_stepIndex]->m_screenY -= 3;
    }
    return 0;
}

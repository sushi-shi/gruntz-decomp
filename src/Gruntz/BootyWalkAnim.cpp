#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/BattlezData.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/LeafCue.h>
#include <Mfc.h>

#include <Gruntz/GameMode.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/BzState.h>

#include <rva.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Random.h>
#include <Gruntz/BootyWalkAnim.h>
#include <Utils/MapTyped.h>

DATA(0x001e9068)
i32 g_idleSpriteIds[4] = {420, 475, 530, 585};

DATA(0x001e9078)
Coord g_multiBootyGeom[8][4] = {
    {{190, 437}, {306, 437}, {422, 437}, {538, 437}},
    {{190, 394}, {306, 394}, {422, 394}, {538, 394}},
    {{190, 351}, {306, 351}, {422, 351}, {538, 351}},
    {{190, 308}, {306, 308}, {422, 308}, {538, 308}},
    {{190, 265}, {306, 265}, {422, 265}, {538, 265}},
    {{190, 222}, {306, 222}, {422, 222}, {538, 222}},
    {{218, 180}, {334, 180}, {450, 180}, {566, 180}},
    {{218, 138}, {334, 138}, {450, 138}, {566, 138}},
};

DATA(0x001e93a8)
char g_secretChars[] = "WARP";

// @early-stop
RVA(0x0001b450, 0x1ac)
i32 CBootyState::BuildBootyWalkingGruntz() {
    if (g_gameReg->m_scoreHud->m_08 != 0) {
        return 1;
    }
    if (g_gameReg->m_scoreHud->m_count > 0x24) {
        return 1;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (sel == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; i++) {
        m_animSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 1, "SimpleAnimation", 3);
        if (m_animSprites[i] == 0) {
            return 0;
        }
        m_animSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_NORTH_WALK");
        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_WALK", 0);
        m_animSprites[i]->m_stateFlags |= 1;
        m_animSprites[i]->m_drawActive = 1;
        m_animSprites[i]->m_drawFillCmd = 0xa;
        m_animSprites[i]->m_drawFillArg = sel;
        m_visSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 1, "SimpleAnimation", 3);
        if (m_visSprites[i] == 0) {
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
    if (rec->m_08 != 0) {
        return 1;
    }
    i32 n = rec->m_count;
    if (n > 0x24) {
        return 1;
    }
    if (m_stepIndex >= 4) {
        return 1;
    }

    if (m_initGate != 0) {

        if (n < 0x24) {
            for (i32 i = 0; i < 4; i++) {
                if (i <= (g_gameReg->m_scoreHud->m_count - 1) % 4) {
                    m_visSprites[i]->m_stateFlags |= 1;
                    m_animSprites[i]->m_screenX = g_idleSpriteIds[i];
                    m_animSprites[i]->m_screenY = 0xdc;
                    m_animSprites[i]->m_stateFlags &= ~1;
                    if ((g_gameReg->m_scoreHud)->GetRecordValue(i) == 0) {
                        m_animSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    } else {
                        CString letter;
                        switch (i) {
                            case 0:
                                letter = "W";
                                break;
                            case 1:
                                letter = "A";
                                break;
                            case 2:
                                letter = "R";
                                break;
                            case 3:
                                letter = "P";
                                break;
                        }
                        m_animSprites[i]->ApplyName("GRUNTZ_PICKUPS");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    }
                } else {
                    m_visSprites[i]->m_screenX = g_idleSpriteIds[i];
                    m_visSprites[i]->m_screenY = 0xdc;
                    m_visSprites[i]->m_stateFlags &= ~1;
                    m_animSprites[i]->m_stateFlags |= 1;
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
    if (m_stepIndex == 0 && (m_animSprites[0]->m_stateFlags & 1)) {
        m_animSprites[0]->m_stateFlags &= ~1;
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
                if (res != 0) {
                    res->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }

    if (m_soundStarted != 0) {
        CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
        LeafCue* res = 0;
        MapLookup(ss->m_cues, "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", res);
        if (res == 0) {
            return 1;
        }
        if (res->m_sound->IsPlaying() != 0) {
            m_visSprites[m_stepIndex]->m_stateFlags ^= 1;
        } else {
            m_visSprites[m_stepIndex]->m_stateFlags |= 1;
        }
    }

    if (m_walkStarted == 0 && m_animSprites[m_stepIndex]->m_screenY <= 0xdc) {
        {
            CString letter;
            switch (m_stepIndex) {
                case 0:
                    letter = "W";
                    break;
                case 1:
                    letter = "A";
                    break;
                case 2:
                    letter = "R";
                    break;
                case 3:
                    letter = "P";
                    break;
            }
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(0, 0);
            if (sel != 0) {
                if ((g_gameReg->m_scoreHud)->GetRecordValue(m_stepIndex) != 0) {
                    CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
                    if (ss->m_emitGate == 0) {
                        LeafCue* res = 0;
                        MapLookup(ss->m_cues, "GAME_FLAGRISE", res);
                        if (res != 0 && g_sndEnabled != 0) {
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
                    g->m_drawFillCmd = 0xa;
                    g->m_drawFillArg = sel;
                    m_visSprites[m_stepIndex]->m_stateFlags |= 1;
                    u32 x;
                    if (!(g_randSeeded & 1)) {
                        g_randSeeded |= 1;
                        x = ::timeGetTime();
                    } else {
                        x = g_randSeed;
                    }
                    g_randSeed = x * 214013 + 2531011;
                    g_gameReg->m_cueSink->SpawnVoiceDriver(
                        0,
                        0x3bf,
                        ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % 0x11,
                        1,
                        -1,
                        -1
                    );
                    m_walkStarted = 1;
                } else {
                    m_animSprites[m_stepIndex]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    CWwdGameObjectA* g = m_animSprites[m_stepIndex];
                    g->m_drawActive = 1;
                    g->m_drawFillCmd = 0xa;
                    g->m_drawFillArg = sel;
                    m_visSprites[m_stepIndex]->m_stateFlags |= 1;
                    m_stepIndex++;
                    g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x441, 0, 1, -1, -1);
                    if (m_stepIndex == g_gameReg->m_scoreHud->m_count % 4) {
                        m_stepIndex = 4;
                        return 1;
                    }
                    if (m_stepIndex < 4) {
                        m_animSprites[m_stepIndex]->m_stateFlags &= ~1;
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
                m_animSprites[m_stepIndex]->m_stateFlags &= ~1;
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

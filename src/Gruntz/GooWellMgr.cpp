#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Warlord.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <stddef.h>

// @early-stop
RVA(0x0006eb80, 0x5ef)
i32 CTriggerMgr::UpdateFrame(i32 deltaMs) {
    if (g_gameReg->m_soundEnabled) {

        if (m_rollingballWanted) {
            if (!m_rollingballLoop) {
                SoundCue* out = g_gameReg->m_world->m_soundRegistry->FindCue("LEVEL_ROLLINGBALL");
                if (out && out->m_sound) {
                    m_rollingballLoop = static_cast<SoundBuffer*>(out->m_sound->AcquireInstance());
                    if (m_rollingballLoop) {
                        m_rollingballLoop->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
                    }
                }
            }
        } else if (m_rollingballLoop) {
            m_rollingballLoop->StopAndRewind();
            m_rollingballLoop = NULL;
        }

        if (m_teleportWanted) {
            if (!m_teleportLoop) {
                SoundCue* out = g_gameReg->m_world->m_soundRegistry->FindCue("GAME_TELEPORTLOOP");
                if (out && out->m_sound) {
                    m_teleportLoop = static_cast<SoundBuffer*>(out->m_sound->AcquireInstance());
                    if (m_teleportLoop) {
                        m_teleportLoop->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
                    }
                }
            }
        } else if (m_teleportLoop) {
            m_teleportLoop->StopAndRewind();
            m_teleportLoop = NULL;
        }
    }
    m_rollingballWanted = false;
    m_teleportWanted = false;

    i32 count = 0;
    GruntzPlayer* pslot = NULL;
    for (i32 k = 0; k < 4; k++) {
        pslot = &g_gameReg->m_players[k];
        if (pslot->m_joined && !pslot->m_doneFlag && !pslot->m_clearedRound) {
            count++;
        }
    }
    if (count <= 1 && m_phase == FINISH_STATE_DEFEAT
        && (static_cast<CPlay*>(g_gameReg->m_curState))->m_statusBar->m_levelOverlayActive == false
        && (static_cast<CPlay*>(g_gameReg->m_curState))->m_statusBar->m_quitConfirmationActive
               == false
        && m_pendingFx == NULL) {
        if (static_cast<i64>(g_frameTime) - m_cueTimer.m_base >= m_cueTimer.m_window) {
            (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(false);
        }
    }

    if (m_countdownActive == false) {
        goto done;
    }

    if (m_phase == FINISH_STATE_DEFEAT) {
        if (m_pendingFx != NULL) {
            goto done;
        }
        if (static_cast<i64>(g_frameTime) - m_cueTimer.m_base >= m_cueTimer.m_window) {
            if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {

                (static_cast<CMulti*>(g_gameReg->m_curState))->m_roundComplete = true;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(false);
            m_countdownActive = false;
            return 0;
        }
        goto done;
    }

    if (m_phase == FINISH_STATE_VICTORY) {
        if (static_cast<i64>(g_frameTime) - m_cueTimer.m_base < m_cueTimer.m_window) {
            goto done;
        }
        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ && m_pendingFx != NULL) {
            goto done;
        }
        (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(false);
        m_countdownActive = false;
        return 0;
    }

    {
        CPlay* obj = static_cast<CPlay*>(g_gameReg->m_curState);
        if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
            i32 idx = obj->ClearPlacedObjects();
            if (idx != -1) {
                GruntzPlayer* lastSlot = pslot;
                i32 i;
                for (i = 0; i < 4; i++) {
                    if (i != idx) {
                        if (g_curPlayer == i) {
                            LoadFinishLevelSprite(FINISH_REASON_BATTLEZ_DEFEAT);
                        }
                        GruntzPlayer* slot = &g_gameReg->m_players[i];
                        if (slot && slot->m_joined && !slot->m_doneFlag && !slot->m_clearedRound) {
                            slot->m_clearedRound = true;
                            CGameObject* out = NULL;
                            if (MapLookupById(
                                    g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                    slot->m_warlordObjectId,
                                    out
                                )
                                && out) {
                                if (out->m_logicRecord->m_userLogic) {
                                    (static_cast<CWarlord*>(out->m_logicRecord->m_userLogic))
                                        ->ResolveDeathAnimation();
                                }
                            }
                            StartPlayerDefeatSequence(i);
                        }
                    } else {
                        if (g_curPlayer == i) {
                            g_gameReg->m_triggerMgr->LoadFinishLevelSprite(
                                FINISH_REASON_BATTLEZ_VICTORY
                            );
                        }
                        if (lastSlot && lastSlot->m_joined && !lastSlot->m_doneFlag
                            && !lastSlot->m_clearedRound) {
                            CGameObject* out = NULL;
                            if (MapLookupById(
                                    g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                    lastSlot->m_warlordObjectId,
                                    out
                                )
                                && out) {
                                if (out->m_logicRecord->m_userLogic) {
                                    (static_cast<CWarlord*>(out->m_logicRecord->m_userLogic))
                                        ->ResolveJoyAnimation();
                                }
                            }
                            StartPlayerVictorySequence(i);
                        }
                    }
                }
                g_gameReg->m_gameStats->RecordFlagCapture(idx, i);
                return 0;
            }
        }

        if (m_overlay) {
            m_overlay->RefreshIfActive(deltaMs);
        }
        if (g_gameReg->m_gameMode == GAMEMODE_BATTLEZ) {
            if (obj->m_winLoseBanner != false && m_unitCountByPlayer[g_curPlayer] == 0) {
                LoadFinishLevelSprite(FINISH_REASON_TIME_EXPIRED);
                return 0;
            }
        }
        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            if (m_unitCountByPlayer[g_curPlayer] != 0) {
                goto done;
            }
            if (obj->m_winLoseBanner != false) {
                LoadFinishLevelSprite(FINISH_REASON_TIME_EXPIRED);
            } else {
                LoadFinishLevelSprite(FINISH_REASON_NO_GRUNTZ_REMAIN);
            }
            return 0;
        }

        if (static_cast<i64>(g_frameTime) - m_gooTimer.m_base >= m_gooTimer.m_window) {
            obj->m_statusBar->AdvanceGruntWell(1);
            m_gooTimer.m_window = g_buteMgr.GetDwordDef("Multiplayer", "TimePerGoo", 0x258);
            m_gooTimer.m_base = g_frameTime;
        }

        if (static_cast<i64>(g_frameTime) - m_resourceTimer.m_base >= m_resourceTimer.m_window) {
            obj->m_statusBar->UpdateRezMachineWakeStatusBar();
            m_resourceTimer.m_window =
                g_buteMgr.GetDwordDef("Multiplayer", "TimePerResource", 0x7530);
            m_resourceTimer.m_base = g_frameTime;
        }

        for (i32 i = 0; i < 4; i++) {
            if (i == g_curPlayer) {
                continue;
            }
            GruntzPlayer* slot = &g_gameReg->m_players[i];
            if (slot->m_joined && !slot->m_doneFlag && !slot->m_clearedRound) {
                goto done;
            }
        }
        LoadFinishLevelSprite(FINISH_REASON_BATTLEZ_VICTORY);
    }
done:
    return 0;
}

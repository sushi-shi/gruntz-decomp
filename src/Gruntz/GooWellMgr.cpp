#include <StdAfx.h>

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
                SoundCue* out = g_gameReg->World()->SoundRegistry()->FindCue("LEVEL_ROLLINGBALL");
                if (out && out->GetSound()) {
                    m_rollingballLoop =
                        static_cast<SoundBuffer*>(out->GetSound()->AcquireInstance());
                    if (m_rollingballLoop) {
                        m_rollingballLoop->ApplyAndPlay(g_gameReg->GetSoundVolume(), 0, 0, true);
                    }
                }
            }
        } else if (m_rollingballLoop) {
            m_rollingballLoop->StopAndRewind();
            m_rollingballLoop = NULL;
        }

        if (m_teleportWanted) {
            if (!m_teleportLoop) {
                SoundCue* out = g_gameReg->World()->SoundRegistry()->FindCue("GAME_TELEPORTLOOP");
                if (out && out->GetSound()) {
                    m_teleportLoop = static_cast<SoundBuffer*>(out->GetSound()->AcquireInstance());
                    if (m_teleportLoop) {
                        m_teleportLoop->ApplyAndPlay(g_gameReg->GetSoundVolume(), 0, 0, true);
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
        pslot = &g_gameReg->GetPlayer(k);
        if (pslot->HasJoinedRound() && !pslot->HasDropped() && !pslot->IsEliminated()) {
            count++;
        }
    }
    if (count <= 1) {
        CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
        if (m_finishState == FINISH_STATE_DEFEAT && play->m_statusBar->m_levelOverlayActive == false
            && play->m_statusBar->m_quitConfirmationActive == false && m_localWarlord == NULL) {
            if (m_finishDelayTiming.Expired()) {
                play->OpenLevelOverlay(false);
            }
        }
    }

    if (m_countdownActive == false) {
        return 0;
    }

    if (m_finishState == FINISH_STATE_DEFEAT) {
        if (m_localWarlord != NULL) {
            return 0;
        }
        if (m_finishDelayTiming.Expired()) {
            if (g_gameReg->GetGameMode() == GAMEMODE_MULTIPLAYER) {

                (static_cast<CMulti*>(g_gameReg->m_curState))->m_roundComplete = true;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(false);
            m_countdownActive = false;
            return 0;
        }
        return 0;
    }

    if (m_finishState == FINISH_STATE_VICTORY) {
        if (!m_finishDelayTiming.Expired()) {
            goto done;
        }
        if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ && m_localWarlord != NULL) {
            return 0;
        }
        (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(false);
        m_countdownActive = false;
        return 0;
    }

    {
        CPlay* obj = static_cast<CPlay*>(g_gameReg->m_curState);
        if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
            i32 idx = obj->ClearPlacedObjects();
            if (idx != -1) {
                GruntzPlayer* lastSlot = pslot;
                i32 i;
                for (i = 0; i < 4; i++) {
                    if (i != idx) {
                        if (g_curPlayer == i) {
                            BeginLevelFinish(FINISH_REASON_BATTLEZ_DEFEAT);
                        }
                        GruntzPlayer* slot = &g_gameReg->GetPlayer(i);
                        if (slot && slot->HasJoinedRound() && !slot->HasDropped()
                            && !slot->IsEliminated()) {
                            slot->m_clearedRound = true;
                            CGameObject* out = NULL;
                            if (MapLookupById(
                                    g_gameReg->World()->ChildGroup()->m_registeredGameObjectsById,
                                    slot->m_warlordObjectId,
                                    out
                                )
                                && out) {
                                if (out->GetLogicRecord()->UserLogic()) {
                                    (static_cast<CWarlord*>(out->GetLogicRecord()->UserLogic()))
                                        ->ResolveDeathAnimation();
                                }
                            }
                            StartPlayerDefeatSequence(i);
                        }
                    } else {
                        if (g_curPlayer == i) {
                            g_gameReg->GetTriggerMgr()->BeginLevelFinish(
                                FINISH_REASON_BATTLEZ_VICTORY
                            );
                        }
                        if (lastSlot && lastSlot->HasJoinedRound() && !lastSlot->HasDropped()
                            && !lastSlot->IsEliminated()) {
                            CGameObject* out = NULL;
                            if (MapLookupById(
                                    g_gameReg->World()->ChildGroup()->m_registeredGameObjectsById,
                                    lastSlot->m_warlordObjectId,
                                    out
                                )
                                && out) {
                                if (out->GetLogicRecord()->UserLogic()) {
                                    (static_cast<CWarlord*>(out->GetLogicRecord()->UserLogic()))
                                        ->ResolveJoyAnimation();
                                }
                            }
                            StartPlayerVictorySequence(i);
                        }
                    }
                }
                g_gameReg->GetGameStats()->RecordFlagCapture(idx, i);
                return 0;
            }
        }

        if (m_actionOptionsMenu) {
            m_actionOptionsMenu->RefreshIfActive(deltaMs);
        }
        if (g_gameReg->GetGameMode() == GAMEMODE_BATTLEZ) {
            if (obj->m_levelTimeExpired != false && m_unitCountByPlayer[g_curPlayer] == 0) {
                BeginLevelFinish(FINISH_REASON_TIME_EXPIRED);
                return 0;
            }
        }
        if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
            if (m_unitCountByPlayer[g_curPlayer] != 0) {
                return 0;
            }
            if (obj->m_levelTimeExpired != false) {
                BeginLevelFinish(FINISH_REASON_TIME_EXPIRED);
            } else {
                BeginLevelFinish(FINISH_REASON_NO_GRUNTZ_REMAIN);
            }
            return 0;
        }

        if (m_gooTimer.Expired()) {
            obj->m_statusBar->AdvanceGruntWell(1);
            m_gooTimer.Start(g_buteMgr.GetDword("Multiplayer", "TimePerGoo", 0x258));
        }

        if (m_resourceTimer.Expired()) {
            obj->m_statusBar->UpdateRezMachineWakeStatusBar();
            m_resourceTimer.Start(g_buteMgr.GetDword("Multiplayer", "TimePerResource", 0x7530));
        }

        for (i32 i = 0; i < 4; i++) {
            if (i == g_curPlayer) {
                continue;
            }
            GruntzPlayer* slot = &g_gameReg->GetPlayer(i);
            if (slot->HasJoinedRound() && !slot->HasDropped() && !slot->IsEliminated()) {
                return 0;
            }
        }
        BeginLevelFinish(FINISH_REASON_BATTLEZ_VICTORY);
    }
done:
    return 0;
}

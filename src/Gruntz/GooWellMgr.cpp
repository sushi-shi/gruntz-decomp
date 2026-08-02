#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Warlord.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

// @early-stop
RVA(0x0006eb80, 0x5ef)
i32 CTriggerMgr::LoadTeleporterGooConfig(i32 off) {
    if (g_gameReg->m_soundEnabled) {

        if (m_rollingballWanted) {
            if (!m_rollingballLoop) {
                void* out_v = 0;
                g_gameReg->m_world->m_soundRegistry->m_cues.Lookup("LEVEL_ROLLINGBALL", out_v);
                LeafCue* out = static_cast<LeafCue*>(out_v);
                if (out && out->m_sound) {
                    m_rollingballLoop = static_cast<DirectSoundMgr*>(out->m_sound->GetItem());
                    if (m_rollingballLoop) {
                        m_rollingballLoop->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                    }
                }
            }
        } else if (m_rollingballLoop) {
            m_rollingballLoop->StopAndRewind();
            m_rollingballLoop = 0;
        }

        if (m_teleportWanted) {
            if (!m_teleportLoop) {
                void* out_v = 0;
                g_gameReg->m_world->m_soundRegistry->m_cues.Lookup("GAME_TELEPORTLOOP", out_v);
                LeafCue* out = static_cast<LeafCue*>(out_v);
                if (out && out->m_sound) {
                    m_teleportLoop = static_cast<DirectSoundMgr*>(out->m_sound->GetItem());
                    if (m_teleportLoop) {
                        m_teleportLoop->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                    }
                }
            }
        } else if (m_teleportLoop) {
            m_teleportLoop->StopAndRewind();
            m_teleportLoop = 0;
        }
    }
    m_rollingballWanted = 0;
    m_teleportWanted = 0;

    i32 count = 0;
    GruntzPlayer* pslot = 0;
    for (i32 k = 0; k < 4; k++) {
        pslot = &g_gameReg->m_options[k];
        if (pslot->m_joined && !pslot->m_doneFlag && !pslot->m_clearedRound) {
            count++;
        }
    }
    if (count <= 1 && m_phase == 2
        && (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_toggleActive == 0
        && (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_toggleHandle == 0
        && m_pendingFx == 0) {
        if (static_cast<i64>(g_frameTime) - m_timerBase >= m_timerWindow) {
            (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(0);
        }
    }

    if (m_countdownActive == 0) {
        goto done;
    }

    if (m_phase == 2) {
        if (m_pendingFx != 0) {
            goto done;
        }
        if (static_cast<i64>(g_frameTime) - m_timerBase >= m_timerWindow) {
            if (g_gameReg->m_gameMode == 2) {

                (static_cast<CMulti*>(g_gameReg->m_curState))->m_roundComplete = 1;
            }
            (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(0);
            m_countdownActive = 0;
            return 0;
        }
        goto done;
    }

    if (m_phase == 1) {
        if (static_cast<i64>(g_frameTime) - m_timerBase < m_timerWindow) {
            goto done;
        }
        if (g_gameReg->m_gameMode == 1 && m_pendingFx != 0) {
            goto done;
        }
        (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(0);
        m_countdownActive = 0;
        return 0;
    }

    {
        CPlay* obj = static_cast<CPlay*>(g_gameReg->m_curState);
        if (g_gameReg->m_gameMode != 1) {
            i32 idx = obj->ClearPlacedObjects();
            if (idx != -1) {
                GruntzPlayer* lastSlot = pslot;
                i32 i;
                for (i = 0; i < 4; i++) {
                    if (i != idx) {
                        if (g_curPlayer == i) {
                            LoadFinishLevelSprite(5);
                        }
                        GruntzPlayer* slot = &g_gameReg->m_options[i];
                        if (slot && slot->m_joined && !slot->m_doneFlag && !slot->m_clearedRound) {
                            slot->m_clearedRound = 1;
                            CGameObject* out = 0;
                            if (MapLookupById(
                                    g_gameReg->m_world->m_childGroup->m_map48,
                                    slot->m_warlordObjectId,
                                    out
                                )
                                && out) {
                                if (out->m_animWorker->m_logic) {
                                    (static_cast<CWarlord*>(out->m_animWorker->m_logic))
                                        ->ResolveDeathAnimation();
                                }
                            }
                            ClearRowAndRefresh(i);
                        }
                    } else {
                        if (g_curPlayer == i) {
                            g_gameReg->m_cmdGrid->LoadFinishLevelSprite(2);
                        }
                        if (lastSlot && lastSlot->m_joined && !lastSlot->m_doneFlag
                            && !lastSlot->m_clearedRound) {
                            CGameObject* out = 0;
                            if (MapLookupById(
                                    g_gameReg->m_world->m_childGroup->m_map48,
                                    lastSlot->m_warlordObjectId,
                                    out
                                )
                                && out) {
                                if (out->m_animWorker->m_logic) {
                                    (static_cast<CWarlord*>(out->m_animWorker->m_logic))
                                        ->RaiseBattleAlert();
                                }
                            }
                            ClearRow(i);
                        }
                    }
                }
                g_gameReg->m_scoreHud->MarkFlag(idx, i);
                return 0;
            }
        }

        if (m_overlay) {
            m_overlay->Activate(off);
        }
        if (g_gameReg->m_gameMode == 3) {
            if (obj->m_winLoseBanner != 0 && m_rowCount[g_curPlayer] == 0) {
                LoadFinishLevelSprite(4);
                return 0;
            }
        }
        if (g_gameReg->m_gameMode == 1) {
            if (m_rowCount[g_curPlayer] != 0) {
                goto done;
            }
            if (obj->m_winLoseBanner != 0) {
                LoadFinishLevelSprite(4);
            } else {
                LoadFinishLevelSprite(3);
            }
            return 0;
        }

        if (static_cast<i64>(g_frameTime) - m_gooTimerBaseLo >= m_gooIntervalLo) {
            obj->m_guts->AdvanceGauge(1);
            m_gooIntervalLo = g_buteMgr.GetDwordDef("Multiplayer", "TimePerGoo", 0x258);
            m_gooTimerBaseLo = g_frameTime;
        }

        if (static_cast<i64>(g_frameTime) - m_resourceTimerBaseLo >= m_resourceIntervalLo) {
            obj->m_guts->UpdateRezMachineWakeStatusBar();
            m_resourceIntervalLo = g_buteMgr.GetDwordDef("Multiplayer", "TimePerResource", 0x7530);
            m_resourceTimerBaseLo = g_frameTime;
        }

        for (i32 i = 0; i < 4; i++) {
            if (i == g_curPlayer) {
                continue;
            }
            GruntzPlayer* slot = &g_gameReg->m_options[i];
            if (slot->m_joined && !slot->m_doneFlag && !slot->m_clearedRound) {
                goto done;
            }
        }
        LoadFinishLevelSprite(2);
    }
done:
    return 0;
}

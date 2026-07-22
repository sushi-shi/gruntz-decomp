#include <Gruntz/BattlezData.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/TriggerMgr.h>       // the canonical class this TU's method extends
#include <Wwd/WwdGameObjectFamily.h> // CGameObject (the wide-object family base)
#include <Gruntz/Grunt.h>
#include <Gruntz/StatusBarMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <Bute/ButeMgr.h>                // CButeMgr (g_buteMgr.GetDwordDef)
#include <Gruntz/GameRegistry.h>         // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/ActionOptionsMenuBar.h> // the real +0x25c overlay (Activate @0x9300)
#include <Gruntz/LeafCue.h>  // the name-map VALUE (its +0x10 DSoundCloneInst plays the cue)
#include <Gruntz/Multi.h>    // CMulti : CPlay - the +0x594 battlez gate is ITS member
#include <Gruntz/Play.h>     // the real CPlay (EnterOverlayDrag / ClearPlacedObjects)
#include <Gruntz/SoundCue.h> // CDDrawSubMgrLeafScan - the world holder's +0x28 named-cue registry
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup + GruntObjEntry - the +0x08 id->object map

// ---------------------------------------------------------------------------
// 0x6eb80 (__thiscall, ret 4) - the per-frame goo-well / win-condition update.
// @early-stop
// ~96.4%, logic byte-exact (tails + the whole 64-bit-countdown / respawn body
// match). Residual is the source-invariant Lookup out-param zero-init scheduling
// wall (docs/patterns/outparam-zeroinit-scheduling.md): retail SINKS `mov [&out],0`
// past the arg pushes at 3 of the 4 ...->Lookup(key, &out) sites (the i==winner
// site already matches), an identical-multiset permutation /O2 won't steer. The
// only other residue is two regalloc coin-flips: the count<=1 guard clusters the
// m_phase load into a reg + hoists g->m_2c before the `cmp $2`, and the per-row
// `g + 0x150 + off` slot lea picks the opposite base/index register pair.
RVA(0x0006eb80, 0x5ef)
i32 CTriggerMgr::LoadTeleporterGooConfig(i32 off) {
    if (g_gameReg->m_soundEnabled) {
        // LEVEL_ROLLINGBALL loop (m_rollingballLoop), wanted by m_rollingballWanted.
        if (m_rollingballWanted) {
            if (!m_rollingballLoop) {
                void* out_v = 0;
                g_gameReg->m_world->m_soundRegistry->m_10.Lookup("LEVEL_ROLLINGBALL", out_v);
                LeafCue* out = static_cast<LeafCue*>(out_v);
                if (out && out->m_10) {
                    m_rollingballLoop = static_cast<DirectSoundMgr*>(out->m_10->GetItem());
                    if (m_rollingballLoop) {
                        m_rollingballLoop->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                    }
                }
            }
        } else if (m_rollingballLoop) {
            m_rollingballLoop->StopAndRewind();
            m_rollingballLoop = 0;
        }
        // GAME_TELEPORTLOOP loop (m_teleportLoop), wanted by m_teleportWanted.
        if (m_teleportWanted) {
            if (!m_teleportLoop) {
                void* out_v = 0;
                g_gameReg->m_world->m_soundRegistry->m_10.Lookup("GAME_TELEPORTLOOP", out_v);
                LeafCue* out = static_cast<LeafCue*>(out_v);
                if (out && out->m_10) {
                    m_teleportLoop = static_cast<DirectSoundMgr*>(out->m_10->GetItem());
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

    // Count joined-and-alive players; remember the last slot scanned.
    i32 count = 0;
    GruntzPlayer* pslot = 0;
    for (i32 k = 0; k < 4; k++) {
        pslot = &g_gameReg->m_options[k];
        if (pslot->m_joined && !pslot->m_doneFlag && !pslot->m_clearedRound) {
            count++;
        }
    }
    if (count <= 1 && m_phase == 2 && (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_toggleActive == 0
        && (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_toggleHandle == 0 && m_pendingFx == 0) {
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
            if (g_gameReg->m_134 == 2) {
                // +0x594 lives past CPlay's tail: it is CMulti::m_594 (CMulti : CPlay),
                // and the m_134 == 2 arm is exactly the mode where m_curState is a CMulti.
                (static_cast<CMulti*>(g_gameReg->m_curState))->m_594 = 1;
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
        if (g_gameReg->m_134 == 1 && m_pendingFx != 0) {
            goto done;
        }
        (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(0);
        m_countdownActive = 0;
        return 0;
    }

    {
        CPlay* obj = static_cast<CPlay*>(g_gameReg->m_curState);
        if (g_gameReg->m_134 != 1) {
            i32 idx = obj->ClearPlacedObjects();
            if (idx != -1) {
                GruntzPlayer* lastSlot = pslot;
                i32 i;
                for (i = 0, off = 0; off < 0x8e0; i++, off += 0x238) {
                    if (i != idx) {
                        if (g_curPlayer == i) {
                            LoadFinishLevelSprite(5);
                        }
                        GruntzPlayer* slot = &g_gameReg->m_options[i];
                        if (slot && slot->m_joined && !slot->m_doneFlag && !slot->m_clearedRound) {
                            slot->m_clearedRound = 1;
                            CGameObject* out = 0;
                            if (g_gameReg->m_world->m_childGroup->m_map48
                                    .Lookup(reinterpret_cast<void*>(slot->m_00c), reinterpret_cast<void*&>(out))
                                && out) {
                                if (out->m_7c->m_logic) {
                                    (static_cast<CGrunt*>(out->m_7c->m_logic))->ResolveDeathAnimation();
                                }
                            }
                            ClearRowAndRefresh(i);
                        }
                    } else {
                        if (g_curPlayer == i) {
                            g_gameReg->m_cmdGrid->LoadFinishLevelSprite(2);
                        }
                        if (lastSlot && lastSlot->m_joined && !lastSlot->m_doneFlag && !lastSlot->m_clearedRound) {
                            CGameObject* out = 0;
                            if (g_gameReg->m_world->m_childGroup->m_map48
                                    .Lookup(reinterpret_cast<void*>(lastSlot->m_00c), reinterpret_cast<void*&>(out))
                                && out) {
                                if (out->m_7c->m_logic) {
                                    (static_cast<CGrunt*>(out->m_7c->m_logic))->ResolveAnimation();
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

        // Tail: reached when the mode is 1, or no winner resolved.
        if (m_overlay) {
            m_overlay->Activate(off);
        }
        if (g_gameReg->m_134 == 3) {
            if (obj->m_winLoseBanner != 0 && m_rowCount[g_curPlayer] == 0) {
                LoadFinishLevelSprite(4);
                return 0;
            }
        }
        if (g_gameReg->m_134 == 1) {
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
        // Goo respawn timer.
        if (static_cast<i64>(g_frameTime) - m_gooTimerBase >= m_gooInterval) {
            obj->m_guts->AdvanceGauge(1);
            m_gooInterval = g_buteMgr.GetDwordDef("Multiplayer", "TimePerGoo", 0x258);
            m_gooTimerBase = g_frameTime;
        }
        // Resource respawn timer.
        if (static_cast<i64>(g_frameTime) - m_resourceTimerBase >= m_resourceInterval) {
            obj->m_guts->UpdateRezMachineWakeStatusBar();
            m_resourceInterval = g_buteMgr.GetDwordDef("Multiplayer", "TimePerResource", 0x7530);
            m_resourceTimerBase = g_frameTime;
        }
        // Last-player-standing: any other live player blocks the win Notify.
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

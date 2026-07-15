// GooWellMgr.cpp - the multiplayer (Battlez) goo-well / resource respawn +
// win-condition step of CTriggerMgr (the g_gameReg->m_68 / m_cmdGrid object).
// THE `CGooWellMgr` VIEW IS DISSOLVED (2026-07-14): it was another fake name for
// CTriggerMgr - every field overlaid the canonical exactly (m_playerFlag[4]@+0x10c
// == m_rowCount, m_overlay@+0x25c, m_phase@+0x288, the +0x290..+0x2c8 i64 timer
// pairs, m_2a0==m_pendingFx, m_2a4==m_countdownActive, the +0x3f0..+0x3fc loop
// channels), its `Notify` @0x7c3d0 is CTriggerMgr::LoadFinishLevelSprite and its
// ClearRowAndRefresh/ClearRow @0x7a510/0x7d140 the already-reconstructed
// CTriggerMgr methods. The per-frame Update (0x6eb80) does four things over the
// 4 player slots (g_gameReg + 0x150, stride 0x238):
//   1. start/stop the LEVEL_ROLLINGBALL and GAME_TELEPORTLOOP looping sounds
//      (handles cached in m_rollingballLoop/m_teleportLoop, gated by the
//      m_rollingballWanted/m_teleportWanted flags);
//   2. drive the 64-bit "match over" countdown (m_timerBase/m_timerWindow) once
//      one player is left, dispatched on the game-mode (g_gameReg->m_134) value;
//   3. on a resolved winner (g_gameReg->m_curState->ClearPlacedObjects() != -1), walk
//      the player rows clearing/refreshing the HUD and resolving death anims;
//   4. run the goo (m_gooTimerBase/m_gooInterval, "TimePerGoo") and resource
//      (m_resourceTimerBase/m_resourceInterval, "TimePerResource") respawn timers,
//      reading the intervals from g_buteMgr.
#include <Gruntz/BattlezData.h>
#include <Gruntz/TriggerMgr.h>       // the canonical class this TU's method extends
#include <Wwd/WwdGameObjectFamily.h> // CWwdGameObjectE (the wide-object family base)
#include <Gruntz/Grunt.h>
#include <Gruntz/StatusBarMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <Bute/ButeMgr.h>                // CButeMgr (g_buteMgr.GetDwordDef)
#include <Gruntz/GameRegistry.h>         // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/ActionOptionsMenuBar.h> // the real +0x25c overlay (Activate @0x9300)
#include <Gruntz/LeafCue.h>       // the name-map VALUE (its +0x10 DSoundCloneInst plays the cue)
#include <Gruntz/Multi.h>         // CMulti : CPlay - the +0x594 battlez gate is ITS member
#include <Gruntz/Play.h>          // the real CPlay (EnterOverlayDrag / ClearPlacedObjects)
#include <Gruntz/SoundCue.h>      // CSndHost - the world holder's +0x28 named-cue registry
#include <Gruntz/SpriteFactory.h> // CSpriteFactory + GruntObjEntry - the +0x08 id->object map

// The free-running game clock (DAT_00645588), read as an unsigned 32-bit tick and
// zero-extended into the 64-bit countdown subtracts.
extern "C" u32 g_frameTime;

// The local player index (DAT_00644c54): selects this client's row.

// The bute attribute store (?g_buteMgr@@3VCButeMgr@@A, from <Bute/ButeMgr.h>): the respawn intervals.

// ---------------------------------------------------------------------------
// THE VIEWS ARE DISSOLVED (2026-07-13). Every hop this TU used to re-model as a
// private struct already had a canonical class, and three sibling TUs write the
// IDENTICAL chains through them:
//
//   CMgrHolderX  -> CSpriteFactoryHolder  (<Gruntz/GameRegistry.h>). Its "m_idMap"
//        (+0x08) is the typed CSpriteFactory* m_8 and its "m_nameMap" (+0x28) is the
//        typed CSndHost* m_28. g_gameReg->m_world was ALREADY declared as this class -
//        the lateral view was re-deriving two members the canonical holder had.
//   CResHolder   -> CSndHost              (<Gruntz/SoundCue.h>): its +0x10 IS the map.
//   CResHolder2  -> CSpriteFactory        (<Gruntz/SpriteFactory.h>): m_objMap @+0x48.
//   CResMapInt   -> GruntObjMap           (same header; the MFC CMapPtrToPtr @+0x48).
//   CLookObj     -> was a CONFLATION of the two maps' value types, which are different
//        classes: the NAME map yields a LeafCue (+0x10 DSoundCloneInst, <Gruntz/LeafCue.h>),
//        the ID map yields a GruntObjEntry (+0x7c inner, <Gruntz/SpriteFactory.h>).
//   CObj7c       -> AnimWorkerObj (canonical): its m_logic is the bound
//        CUserLogic leaf, downcast to CGrunt for the animation resolve.
//   CGaugeObj    -> CStatusBarMgr         (<Gruntz/StatusBarMgr.h>): +0x550 m_toggleActive,
//        +0x554 m_toggleHandle. The file already CAST this member to CStatusBarMgr to call
//        AdvanceGauge/UpdateRezMachineWakeStatusBar - it disproved its own view.
//   CGameObj2c   -> CPlay                 (<Gruntz/Play.h>): +0x2dc m_guts, +0x4f4
//        m_winLoseBanner. The "Play.h is too heavy here" note was false - it includes fine.
//        Its +0x594 is NOT a CPlay member: it is CMulti::m_594 (<Gruntz/Multi.h>,
//        CMulti : CPlay), and the store is guarded by the m_134 == 2 mode test.
//   CPlay / CActionOptionsMenuBar (TU-local decl-only shadows) -> the real headers.
//
// The id->object lookup below is verbatim the chain GruntzMgrCmd.cpp's 0x8106 cheat
// already writes cast-free through the canonicals (m_world->m_8->m_objMap ->
// GruntObjEntry::m_7c->m_18 -> CGrunt::ResolveDeathAnimation).
//
// THE MAP CLASS WAS INVERTED: the +0x10 registry's Lookup @0x1b8438 is
// ?Lookup@CMapStringToPtr@@ (python -m gruntz.analysis.mfc_class 0x1b8438; CMapStringToOb's
// is the DIFFERENT body at 0x1b8008). The old `CMapStringToOb m_map10` bound every call
// here to the wrong library routine. CMapStringToPtr is a void* container, so the
// (LeafCue*) at the use site is the devs' own cast.
// ---------------------------------------------------------------------------

// One player slot is CFocusSlot, the g_gameReg->m_focusSlots[] element
// (<Gruntz/GameRegistry.h>): m_28 joined, m_2c done, m_24 the "already cleared
// this round" mark, m_0c the row's sound id.

// The game-registry singleton, canonical CGameRegistry view. The slots this TU
// walks are typed there; the casts that remain are AUTHENTIC view/downcasts:
//   +0x2c  m_2c is CState*; CPlay/CMulti are the concrete play-mode DOWNCASTs.
//   +0x68  m_68 is a genuinely REUSED slot (placement/cue grid in single-player,
//          the CTriggerMgr goo-well step in battlez) - a real per-mode object.
// The m_10/m_11c/m_134 scalars match, and the per-player slot array at +0x150
// (stride 0x238) is reached via raw offset.
extern "C" CGameRegistry* g_gameReg;

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
                g_gameReg->m_world->m_28->m_10.Lookup("LEVEL_ROLLINGBALL", out_v);
                LeafCue* out = (LeafCue*)out_v;
                if (out && out->m_10) {
                    m_rollingballLoop = (DirectSoundMgr*)out->m_10->GetItem();
                    if (m_rollingballLoop) {
                        m_rollingballLoop->ApplyAndPlay(g_gameReg->m_inputFlag, 0, 0, 1);
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
                g_gameReg->m_world->m_28->m_10.Lookup("GAME_TELEPORTLOOP", out_v);
                LeafCue* out = (LeafCue*)out_v;
                if (out && out->m_10) {
                    m_teleportLoop = (DirectSoundMgr*)out->m_10->GetItem();
                    if (m_teleportLoop) {
                        m_teleportLoop->ApplyAndPlay(g_gameReg->m_inputFlag, 0, 0, 1);
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
    CFocusSlot* pslot = 0;
    for (i32 k = 0; k < 4; k++) {
        pslot = &g_gameReg->m_focusSlots[k];
        if (pslot->m_28 && !pslot->m_2c && !pslot->m_24) {
            count++;
        }
    }
    if (count <= 1 && m_phase == 2 && ((CPlay*)g_gameReg->m_curState)->m_guts->m_toggleActive == 0
        && ((CPlay*)g_gameReg->m_curState)->m_guts->m_toggleHandle == 0 && m_pendingFx == 0) {
        if ((i64)g_frameTime - m_timerBase >= m_timerWindow) {
            ((CPlay*)g_gameReg->m_curState)->EnterOverlayDrag(0);
        }
    }

    if (m_countdownActive == 0) {
        goto done;
    }

    if (m_phase == 2) {
        if (m_pendingFx != 0) {
            goto done;
        }
        if ((i64)g_frameTime - m_timerBase >= m_timerWindow) {
            if (g_gameReg->m_134 == 2) {
                // +0x594 lives past CPlay's tail: it is CMulti::m_594 (CMulti : CPlay),
                // and the m_134 == 2 arm is exactly the mode where m_curState is a CMulti.
                ((CMulti*)g_gameReg->m_curState)->m_594 = 1;
            }
            ((CPlay*)g_gameReg->m_curState)->EnterOverlayDrag(0);
            m_countdownActive = 0;
            return 0;
        }
        goto done;
    }

    if (m_phase == 1) {
        if ((i64)g_frameTime - m_timerBase < m_timerWindow) {
            goto done;
        }
        if (g_gameReg->m_134 == 1 && m_pendingFx != 0) {
            goto done;
        }
        ((CPlay*)g_gameReg->m_curState)->EnterOverlayDrag(0);
        m_countdownActive = 0;
        return 0;
    }

    {
        CPlay* obj = (CPlay*)g_gameReg->m_curState;
        if (g_gameReg->m_134 != 1) {
            i32 idx = obj->ClearPlacedObjects();
            if (idx != -1) {
                CFocusSlot* lastSlot = pslot;
                i32 i;
                for (i = 0, off = 0; off < 0x8e0; i++, off += 0x238) {
                    if (i != idx) {
                        if (g_curPlayer == i) {
                            LoadFinishLevelSprite(5);
                        }
                        CFocusSlot* slot = (CFocusSlot*)((char*)g_gameReg + 0x150 + off);
                        if (slot && slot->m_28 && !slot->m_2c && !slot->m_24) {
                            slot->m_24 = 1;
                            CWwdGameObjectE* out = 0;
                            if (((CMapPtrToPtr*)&g_gameReg->m_world->m_8->m_objMap)
                                    ->Lookup((void*)slot->m_0c, (void*&)out)
                                && out) {
                                if (out->m_7c->m_logic) {
                                    ((CGrunt*)out->m_7c->m_logic)->ResolveDeathAnimation();
                                }
                            }
                            ClearRowAndRefresh(i);
                        }
                    } else {
                        if (g_curPlayer == i) {
                            g_gameReg->m_cmdGrid->LoadFinishLevelSprite(2);
                        }
                        if (lastSlot && lastSlot->m_28 && !lastSlot->m_2c && !lastSlot->m_24) {
                            CWwdGameObjectE* out = 0;
                            if (((CMapPtrToPtr*)&g_gameReg->m_world->m_8->m_objMap)
                                    ->Lookup((void*)lastSlot->m_0c, (void*&)out)
                                && out) {
                                if (out->m_7c->m_logic) {
                                    ((CGrunt*)out->m_7c->m_logic)->ResolveAnimation();
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
        if ((i64)g_frameTime - m_gooTimerBase >= m_gooInterval) {
            obj->m_guts->AdvanceGauge(1);
            m_gooInterval = g_buteMgr.GetDwordDef("Multiplayer", "TimePerGoo", 0x258);
            m_gooTimerBase = g_frameTime;
        }
        // Resource respawn timer.
        if ((i64)g_frameTime - m_resourceTimerBase >= m_resourceInterval) {
            obj->m_guts->UpdateRezMachineWakeStatusBar();
            m_resourceInterval = g_buteMgr.GetDwordDef("Multiplayer", "TimePerResource", 0x7530);
            m_resourceTimerBase = g_frameTime;
        }
        // Last-player-standing: any other live player blocks the win Notify.
        for (i32 i = 0; i < 4; i++) {
            if (i == g_curPlayer) {
                continue;
            }
            CFocusSlot* slot = &g_gameReg->m_focusSlots[i];
            if (slot->m_28 && !slot->m_2c && !slot->m_24) {
                goto done;
            }
        }
        LoadFinishLevelSprite(2);
    }
done:
    return 0;
}

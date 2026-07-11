// GooWellMgr.cpp - CGooWellMgr, the multiplayer (Battlez) goo-well / resource
// respawn + win-condition manager held at g_gameReg->m_68. Its per-frame Update
// (0x6eb80) does four things over the 4 player slots (g_gameReg + 0x150, stride
// 0x238):
//   1. start/stop the LEVEL_ROLLINGBALL and GAME_TELEPORTLOOP looping sounds
//      (sound handles cached in m_rollingballLoop/m_teleportLoop, gated by the m_rollingballWanted/m_teleportWanted flags);
//   2. drive the 64-bit "match over" countdown (m_countdownBase/m_countdownLength) once one player is
//      left, dispatched on the game-mode (g_gameReg->m_134) value;
//   3. on a resolved winner (g_gameReg->m_curState->ClearPlacedObjects() != -1), walk
//      the player rows clearing/refreshing the HUD and resolving death anims;
//   4. run the goo (m_gooTimerBase/m_gooInterval, "TimePerGoo") and resource (m_resourceTimerBase/m_resourceInterval,
//      "TimePerResource") respawn timers, reading the intervals from g_buteMgr.
//
// Field names are recovered from usage; only the OFFSETS + the
// per-method call/branch structure are load-bearing (campaign doctrine). Every
// callee/global is an external no-body decl so its `call rel32` / DIR32 operand
// reloc-masks.
#include <Gruntz/BattlezData.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/SoundCueMgr.h>
#include <rva.h>
#include <Bute/ButeMgr.h>        // CButeMgr (g_buteMgr.GetDwordDef)
#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view

struct CGooWellMgr;
struct CGameObj2c;
struct CGaugeObj;
struct CMgrHolderX;
struct CResHolder;
struct CResHolder2;
struct CLookObj;
struct CObj7c;
struct CGrunt;
struct CSoundHandle;

// The free-running game clock (DAT_00645588), read as an unsigned 32-bit tick and
// zero-extended into the 64-bit countdown subtracts.
DATA(0x00245588)
extern "C" u32 g_645588;

// The local player index (DAT_00644c54): selects this client's row.
DATA(0x00244c54)
extern "C" i32 g_644c54;

// The bute attribute store (?g_buteMgr@@3VCButeMgr@@A): the respawn intervals.
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// ---------------------------------------------------------------------------
// The looked-up resource record (returned through CMapStringToOb / CMap...To...
// Lookup out-params): its +0x10 is a sound factory, its +0x7c a host whose +0x18
// drives the per-grunt animation resolve.
struct CObj7c {
    char _0[0x18];
    CGrunt* m_anim; // +0x18
};
struct CLookObj {
    char _0[0x10];
    CSoundCueMgr* m_soundFactory; // +0x10
    char _14[0x7c - 0x14];
    CObj7c* m_host; // +0x7c
};

// The two keyed stores reached through g_gameReg->m_world: a name->record map at
// (->m_nameMap + 0x10) and an id->record map at (->m_idMap + 0x48).
struct CResMapStr {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at each call
struct CResMapInt {}; // MFC CMapPtrToPtr (Lookup @0x1b8760); cast at each call
struct CResHolder {
    char _0[0x10];
    CResMapStr m_map10; // +0x10
};
struct CResHolder2 {
    char _0[0x48];
    CResMapInt m_map48; // +0x48
};
struct CMgrHolderX {
    char _0[8];
    CResHolder2* m_idMap; // +0x8
    char _c[0x28 - 0xc];
    CResHolder* m_nameMap; // +0x28
};

// The gauge/HUD sub-object (g_gameReg->m_curState->m_gauge) the respawn timers poke.
struct CGaugeObj {
    char _0[0x550];
    i32 m_550; // +0x550
    i32 m_554; // +0x554
};

// The active game-mode object (g_gameReg->m_curState).
struct CGameObj2c {
    char _0[0x2dc];
    CGaugeObj* m_gauge; // +0x2dc
    char _2e0[0x4f4 - 0x2e0];
    i32 m_4f4; // +0x4f4
    char _4f8[0x594 - 0x4f8];
    i32 m_594; // +0x594
    // EnterOverlayDrag @0xd6440 / ClearPlacedObjects @0xda030 are both CPlay
    // methods (the ex-"CGameModeObj" view folded onto CPlay, wave3-J); cast at
    // each call. Local decl-only view of the real CPlay (Play.h is too heavy here).
};
class CPlay {
public:
    i32 EnterOverlayDrag(i32);
    i32 ClearPlacedObjects();
};

// A per-player overlay the tail re-activates (g_gameReg->m_68->m_overlay).
// The +0x25c overlay is a CActionOptionsMenuBar (Activate @0x9300); TU-local decl.
class CActionOptionsMenuBar {
public:
    i32 Activate(i32);
};

// The battlez score tracker (g_gameReg->m_7c).

// One player slot is CFocusSlot, the g_gameReg->m_focusSlots[] element
// (<Gruntz/GameRegistry.h>): m_28 joined, m_2c done, m_24 the "already cleared
// this round" mark, m_0c the row's sound id.

// The game-registry singleton, canonical CGameRegistry view. The slots this TU
// walks are typed there; the local casts below are AUTHENTIC view/downcasts, not
// squeeze-hacks:
//   +0x2c  m_2c is CState* (current game-state); CGameObj2c is the concrete
//          battlez play-mode DOWNCAST (its gauge/placement/overlay methods).
//   +0x30  m_30 is the typed CSpriteFactoryHolder resource mgr; CMgrHolderX is
//          this TU's LATERAL view of its two keyed-lookup facets (the sound name
//          map at +0x28->+0x10 and the id map at +0x08->+0x48, both engine Lookup
//          helpers). The canonical holder keeps those facets untyped so the loader
//          cluster's CreateSprite/FindByKey shapes stay byte-exact; unifying the
//          inner map types into CSpriteFactoryHolder is a follow-up matcher.
//   +0x68  m_68 is a genuinely REUSED slot (placement/cue grid in single-player,
//          this CGooWellMgr in battlez) - a real per-mode object downcast.
//   +0x7c  m_7c is the score/HUD sink; CBzData is its battlez MarkFlag facet.
// The m_10/m_11c/m_134 scalars match, and the per-player slot array at +0x150
// (stride 0x238) is reached via raw offset.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

struct CGooWellMgr {
    char _0[0x10c];
    i32 m_playerFlag[4]; // +0x10c per-player flag
    char _11c[0x25c - 0x11c];
    CActionOptionsMenuBar* m_overlay; // +0x25c
    char _260[0x288 - 0x260];
    i32 m_phase; // +0x288 phase
    char _28c[0x290 - 0x28c];
    i64 m_countdownBase;   // +0x290 countdown base
    i64 m_countdownLength; // +0x298 countdown length
    i32 m_2a0;             // +0x2a0
    i32 m_countdownActive; // +0x2a4
    char _2a8[0x2b0 - 0x2a8];
    i64 m_gooTimerBase;      // +0x2b0 goo timer base
    i64 m_gooInterval;       // +0x2b8 goo interval
    i64 m_resourceTimerBase; // +0x2c0 resource timer base
    i64 m_resourceInterval;  // +0x2c8 resource interval
    char _2d0[0x3f0 - 0x2d0];
    DirectSoundMgr* m_rollingballLoop; // +0x3f0 rollingball loop handle
    DirectSoundMgr* m_teleportLoop;    // +0x3f4 teleportloop handle
    i32 m_rollingballWanted;           // +0x3f8 rollingball wanted
    i32 m_teleportWanted;              // +0x3fc teleportloop wanted

    i32 LoadTeleporterGooConfig(i32 off); // 0x6eb80
    void Notify(i32);                     // 0x7c3d0
    void ClearRowAndRefresh(i32);         // 0x7a510
    void ClearRow(i32);                   // 0x7d140
};

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
i32 CGooWellMgr::LoadTeleporterGooConfig(i32 off) {
    if (g_gameReg->m_soundEnabled) {
        // LEVEL_ROLLINGBALL loop (m_rollingballLoop), wanted by m_rollingballWanted.
        if (m_rollingballWanted) {
            if (!m_rollingballLoop) {
                CLookObj* out = 0;
                ((CMapStringToOb*)&((CMgrHolderX*)g_gameReg->m_world)->m_nameMap->m_map10)
                    ->Lookup("LEVEL_ROLLINGBALL", (CObject*&)out);
                if (out && out->m_soundFactory) {
                    m_rollingballLoop = (DirectSoundMgr*)out->m_soundFactory->GetItem();
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
                CLookObj* out = 0;
                ((CMapStringToOb*)&((CMgrHolderX*)g_gameReg->m_world)->m_nameMap->m_map10)
                    ->Lookup("GAME_TELEPORTLOOP", (CObject*&)out);
                if (out && out->m_soundFactory) {
                    m_teleportLoop = (DirectSoundMgr*)out->m_soundFactory->GetItem();
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
    if (count <= 1 && m_phase == 2 && ((CGameObj2c*)g_gameReg->m_curState)->m_gauge->m_550 == 0
        && ((CGameObj2c*)g_gameReg->m_curState)->m_gauge->m_554 == 0 && m_2a0 == 0) {
        if ((i64)g_645588 - m_countdownBase >= m_countdownLength) {
            ((CPlay*)g_gameReg->m_curState)->EnterOverlayDrag(0);
        }
    }

    if (m_countdownActive == 0) {
        goto done;
    }

    if (m_phase == 2) {
        if (m_2a0 != 0) {
            goto done;
        }
        if ((i64)g_645588 - m_countdownBase >= m_countdownLength) {
            if (g_gameReg->m_134 == 2) {
                ((CGameObj2c*)g_gameReg->m_curState)->m_594 = 1;
            }
            ((CPlay*)g_gameReg->m_curState)->EnterOverlayDrag(0);
            m_countdownActive = 0;
            return 0;
        }
        goto done;
    }

    if (m_phase == 1) {
        if ((i64)g_645588 - m_countdownBase < m_countdownLength) {
            goto done;
        }
        if (g_gameReg->m_134 == 1 && m_2a0 != 0) {
            goto done;
        }
        ((CPlay*)g_gameReg->m_curState)->EnterOverlayDrag(0);
        m_countdownActive = 0;
        return 0;
    }

    {
        CGameObj2c* obj = (CGameObj2c*)g_gameReg->m_curState;
        if (g_gameReg->m_134 != 1) {
            i32 idx = ((CPlay*)obj)->ClearPlacedObjects();
            if (idx != -1) {
                CFocusSlot* lastSlot = pslot;
                i32 i;
                for (i = 0, off = 0; off < 0x8e0; i++, off += 0x238) {
                    if (i != idx) {
                        if (g_644c54 == i) {
                            Notify(5);
                        }
                        CFocusSlot* slot = (CFocusSlot*)((char*)g_gameReg + 0x150 + off);
                        if (slot && slot->m_28 && !slot->m_2c && !slot->m_24) {
                            slot->m_24 = 1;
                            CLookObj* out = 0;
                            if (((CMapPtrToPtr*)&((CMgrHolderX*)g_gameReg->m_world)
                                     ->m_idMap->m_map48)
                                    ->Lookup((void*)slot->m_0c, (void*&)out)
                                && out) {
                                if (out->m_host->m_anim) {
                                    out->m_host->m_anim->ResolveDeathAnimation();
                                }
                            }
                            ClearRowAndRefresh(i);
                        }
                    } else {
                        if (g_644c54 == i) {
                            ((CGooWellMgr*)g_gameReg->m_cmdGrid)->Notify(2);
                        }
                        if (lastSlot && lastSlot->m_28 && !lastSlot->m_2c && !lastSlot->m_24) {
                            CLookObj* out = 0;
                            if (((CMapPtrToPtr*)&((CMgrHolderX*)g_gameReg->m_world)
                                     ->m_idMap->m_map48)
                                    ->Lookup((void*)lastSlot->m_0c, (void*&)out)
                                && out) {
                                if (out->m_host->m_anim) {
                                    out->m_host->m_anim->ResolveAnimation();
                                }
                            }
                            ClearRow(i);
                        }
                    }
                }
                ((CBattlezData*)g_gameReg->m_scoreHud)->MarkFlag(idx, i);
                return 0;
            }
        }

        // Tail: reached when the mode is 1, or no winner resolved.
        if (m_overlay) {
            m_overlay->Activate(off);
        }
        if (g_gameReg->m_134 == 3) {
            if (obj->m_4f4 != 0 && m_playerFlag[g_644c54] == 0) {
                Notify(4);
                return 0;
            }
        }
        if (g_gameReg->m_134 == 1) {
            if (m_playerFlag[g_644c54] != 0) {
                goto done;
            }
            if (obj->m_4f4 != 0) {
                Notify(4);
            } else {
                Notify(3);
            }
            return 0;
        }
        // Goo respawn timer.
        if ((i64)g_645588 - m_gooTimerBase >= m_gooInterval) {
            ((CSBI_RectOnly*)obj->m_gauge)->AdvanceGauge(1);
            m_gooInterval = g_buteMgr.GetDwordDef("Multiplayer", "TimePerGoo", 0x258);
            m_gooTimerBase = g_645588;
        }
        // Resource respawn timer.
        if ((i64)g_645588 - m_resourceTimerBase >= m_resourceInterval) {
            ((CSBI_RectOnly*)obj->m_gauge)->UpdateRezMachineWakeStatusBar();
            m_resourceInterval = g_buteMgr.GetDwordDef("Multiplayer", "TimePerResource", 0x7530);
            m_resourceTimerBase = g_645588;
        }
        // Last-player-standing: any other live player blocks the win Notify.
        for (i32 i = 0; i < 4; i++) {
            if (i == g_644c54) {
                continue;
            }
            CFocusSlot* slot = &g_gameReg->m_focusSlots[i];
            if (slot->m_28 && !slot->m_2c && !slot->m_24) {
                goto done;
            }
        }
        Notify(2);
    }
done:
    return 0;
}

SIZE_UNKNOWN(CObj7c);
SIZE_UNKNOWN(CSoundHandle);
SIZE_UNKNOWN(CSoundFactory);
SIZE_UNKNOWN(CLookObj);
SIZE_UNKNOWN(CResMapStr);
SIZE_UNKNOWN(CResMapInt);
SIZE_UNKNOWN(CResHolder);
SIZE_UNKNOWN(CResHolder2);
SIZE_UNKNOWN(CMgrHolderX);
SIZE_UNKNOWN(CGaugeObj);
SIZE_UNKNOWN(CGameObj2c);
SIZE_UNKNOWN(CActionOptionsMenuBar);
SIZE_UNKNOWN(CBzData);
SIZE_UNKNOWN(CGooWellMgr);

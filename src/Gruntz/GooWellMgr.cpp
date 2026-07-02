// GooWellMgr.cpp - CGooWellMgr, the multiplayer (Battlez) goo-well / resource
// respawn + win-condition manager held at g_gameReg->m_68. Its per-frame Update
// (0x6eb80) does four things over the 4 player slots (g_gameReg + 0x150, stride
// 0x238):
//   1. start/stop the LEVEL_ROLLINGBALL and GAME_TELEPORTLOOP looping sounds
//      (sound handles cached in m_3f0/m_3f4, gated by the m_3f8/m_3fc flags);
//   2. drive the 64-bit "match over" countdown (m_290/m_298) once one player is
//      left, dispatched on the game-mode (g_gameReg->m_134) value;
//   3. on a resolved winner (g_gameReg->m_2c->ClearPlacedObjects() != -1), walk
//      the player rows clearing/refreshing the HUD and resolving death anims;
//   4. run the goo (m_2b0/m_2b8, "TimePerGoo") and resource (m_2c0/m_2c8,
//      "TimePerResource") respawn timers, reading the intervals from g_buteMgr.
//
// Field names are placeholders recovered from usage; only the OFFSETS + the
// per-method call/branch structure are load-bearing (campaign doctrine). Every
// callee/global is an external no-body decl so its `call rel32` / DIR32 operand
// reloc-masks.
#include <rva.h>
#include <Bute/ButeMgr.h>         // CButeMgr (g_buteMgr.GetDwordDef)
#include <Gruntz/CGameRegistry.h> // g_gameReg singleton (0x24556c) canonical view

struct CGooWellMgr;
struct CGameObj2c;
struct CGaugeObj;
struct CMgrHolderX;
struct CResHolder;
struct CResHolder2;
struct CLookObj;
struct CObj7c;
struct CAnimObj;
struct CSoundFactory;
struct CSoundHandle;
struct CActivatable;
struct CBzData;
struct PlayerSlot;

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
struct CAnimObj {
    void ResolveDeathAnimation(); // 0x455f0
    void ResolveAnimation();      // 0x457b0
};
struct CObj7c {
    char _0[0x18];
    CAnimObj* m_18; // +0x18
};
struct CSoundHandle {
    void ApplyAndPlay(i32, i32, i32, i32); // 0x136300
    void StopAndRewind();                  // 0x135380
};
struct CSoundFactory {
    CSoundHandle* GetItem(); // 0x135d70
};
struct CLookObj {
    char _0[0x10];
    CSoundFactory* m_10; // +0x10
    char _14[0x7c - 0x14];
    CObj7c* m_7c; // +0x7c
};

// The two keyed stores reached through g_gameReg->m_30: a name->record map at
// (->m_28 + 0x10) and an id->record map at (->m_8 + 0x48).
struct CResMapStr {
    i32 Lookup(const char* key, CLookObj*& out); // 0x1b8438
};
struct CResMapInt {
    i32 Lookup(i32 key, CLookObj*& out); // 0x1b8760
};
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
    CResHolder2* m_8; // +0x8
    char _c[0x28 - 0xc];
    CResHolder* m_28; // +0x28
};

// The gauge/HUD sub-object (g_gameReg->m_2c->m_2dc) the respawn timers poke.
struct CGaugeObj {
    char _0[0x550];
    i32 m_550;                            // +0x550
    i32 m_554;                            // +0x554
    void AdvanceGauge(i32);               // 0x105750
    void UpdateRezMachineWakeStatusBar(); // 0x107a10
};

// The active game-mode object (g_gameReg->m_2c).
struct CGameObj2c {
    char _0[0x2dc];
    CGaugeObj* m_2dc; // +0x2dc
    char _2e0[0x4f4 - 0x2e0];
    i32 m_4f4; // +0x4f4
    char _4f8[0x594 - 0x4f8];
    i32 m_594;                  // +0x594
    void EnterOverlayDrag(i32); // 0xd6440
    i32 ClearPlacedObjects();   // 0xda030 -> winner row, or -1
};

// A per-player overlay the tail re-activates (g_gameReg->m_68->m_25c).
struct CActivatable {
    void Activate(i32); // 0x9300
};

// The battlez score tracker (g_gameReg->m_7c).
struct CBzData {
    void MarkFlag(i32, i32); // 0xfcb50
};

// One player slot (g_gameReg + 0x150, stride 0x238): m_28 = joined, m_2c = a
// done flag, m_24 = the "already cleared this round" mark, m_c = the row's sound id.
struct PlayerSlot {
    char _0[0xc];
    i32 m_c; // +0xc
    char _10[0x24 - 0x10];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
    char _30[0x238 - 0x30];
};

// The game-registry singleton, canonical CGameRegistry view. The game-mode object
// (+0x2c -> CGameObj2c), resource holder (+0x30 -> CMgrHolderX), goo-well mgr
// (+0x68 -> CGooWellMgr) and battlez tracker (+0x7c -> CBzData) are void*/typed
// slots cast locally at the deref sites; the m_10/m_11c/m_134 scalars match, and
// the per-player slot array at +0x150 (stride 0x238) is reached via raw offset.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

struct CGooWellMgr {
    char _0[0x10c];
    i32 m_10c[4]; // +0x10c per-player flag
    char _11c[0x25c - 0x11c];
    CActivatable* m_25c; // +0x25c
    char _260[0x288 - 0x260];
    i32 m_288; // +0x288 phase
    char _28c[0x290 - 0x28c];
    i64 m_290; // +0x290 countdown base
    i64 m_298; // +0x298 countdown length
    i32 m_2a0; // +0x2a0
    i32 m_2a4; // +0x2a4
    char _2a8[0x2b0 - 0x2a8];
    i64 m_2b0; // +0x2b0 goo timer base
    i64 m_2b8; // +0x2b8 goo interval
    i64 m_2c0; // +0x2c0 resource timer base
    i64 m_2c8; // +0x2c8 resource interval
    char _2d0[0x3f0 - 0x2d0];
    CSoundHandle* m_3f0; // +0x3f0 rollingball loop handle
    CSoundHandle* m_3f4; // +0x3f4 teleportloop handle
    i32 m_3f8;           // +0x3f8 rollingball wanted
    i32 m_3fc;           // +0x3fc teleportloop wanted

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
// m_288 load into a reg + hoists g->m_2c before the `cmp $2`, and the per-row
// `g + 0x150 + off` slot lea picks the opposite base/index register pair.
RVA(0x0006eb80, 0x5ef)
i32 CGooWellMgr::LoadTeleporterGooConfig(i32 off) {
    if (g_gameReg->m_10) {
        // LEVEL_ROLLINGBALL loop (m_3f0), wanted by m_3f8.
        if (m_3f8) {
            if (!m_3f0) {
                CLookObj* out = 0;
                ((CMgrHolderX*)g_gameReg->m_30)->m_28->m_map10.Lookup("LEVEL_ROLLINGBALL", out);
                if (out && out->m_10) {
                    m_3f0 = out->m_10->GetItem();
                    if (m_3f0) {
                        m_3f0->ApplyAndPlay(g_gameReg->m_11c, 0, 0, 1);
                    }
                }
            }
        } else if (m_3f0) {
            m_3f0->StopAndRewind();
            m_3f0 = 0;
        }
        // GAME_TELEPORTLOOP loop (m_3f4), wanted by m_3fc.
        if (m_3fc) {
            if (!m_3f4) {
                CLookObj* out = 0;
                ((CMgrHolderX*)g_gameReg->m_30)->m_28->m_map10.Lookup("GAME_TELEPORTLOOP", out);
                if (out && out->m_10) {
                    m_3f4 = out->m_10->GetItem();
                    if (m_3f4) {
                        m_3f4->ApplyAndPlay(g_gameReg->m_11c, 0, 0, 1);
                    }
                }
            }
        } else if (m_3f4) {
            m_3f4->StopAndRewind();
            m_3f4 = 0;
        }
    }
    m_3f8 = 0;
    m_3fc = 0;

    // Count joined-and-alive players; remember the last slot scanned.
    i32 count = 0;
    PlayerSlot* pslot = 0;
    for (i32 k = 0; k < 4; k++) {
        pslot = (PlayerSlot*)((char*)g_gameReg + 0x150 + k * 0x238);
        if (pslot->m_28 && !pslot->m_2c && !pslot->m_24) {
            count++;
        }
    }
    if (count <= 1 && m_288 == 2 && ((CGameObj2c*)g_gameReg->m_2c)->m_2dc->m_550 == 0
        && ((CGameObj2c*)g_gameReg->m_2c)->m_2dc->m_554 == 0 && m_2a0 == 0) {
        if ((i64)g_645588 - m_290 >= m_298) {
            ((CGameObj2c*)g_gameReg->m_2c)->EnterOverlayDrag(0);
        }
    }

    if (m_2a4 == 0) {
        goto done;
    }

    if (m_288 == 2) {
        if (m_2a0 != 0) {
            goto done;
        }
        if ((i64)g_645588 - m_290 >= m_298) {
            if (g_gameReg->m_134 == 2) {
                ((CGameObj2c*)g_gameReg->m_2c)->m_594 = 1;
            }
            ((CGameObj2c*)g_gameReg->m_2c)->EnterOverlayDrag(0);
            m_2a4 = 0;
            return 0;
        }
        goto done;
    }

    if (m_288 == 1) {
        if ((i64)g_645588 - m_290 < m_298) {
            goto done;
        }
        if (g_gameReg->m_134 == 1 && m_2a0 != 0) {
            goto done;
        }
        ((CGameObj2c*)g_gameReg->m_2c)->EnterOverlayDrag(0);
        m_2a4 = 0;
        return 0;
    }

    {
        CGameObj2c* obj = (CGameObj2c*)g_gameReg->m_2c;
        if (g_gameReg->m_134 != 1) {
            i32 idx = obj->ClearPlacedObjects();
            if (idx != -1) {
                PlayerSlot* lastSlot = pslot;
                i32 i;
                for (i = 0, off = 0; off < 0x8e0; i++, off += 0x238) {
                    if (i != idx) {
                        if (g_644c54 == i) {
                            Notify(5);
                        }
                        PlayerSlot* slot = (PlayerSlot*)((char*)g_gameReg + 0x150 + off);
                        if (slot && slot->m_28 && !slot->m_2c && !slot->m_24) {
                            slot->m_24 = 1;
                            CLookObj* out = 0;
                            if (((CMgrHolderX*)g_gameReg->m_30)->m_8->m_map48.Lookup(slot->m_c, out)
                                && out) {
                                if (out->m_7c->m_18) {
                                    out->m_7c->m_18->ResolveDeathAnimation();
                                }
                            }
                            ClearRowAndRefresh(i);
                        }
                    } else {
                        if (g_644c54 == i) {
                            ((CGooWellMgr*)g_gameReg->m_68)->Notify(2);
                        }
                        if (lastSlot && lastSlot->m_28 && !lastSlot->m_2c && !lastSlot->m_24) {
                            CLookObj* out = 0;
                            if (((CMgrHolderX*)g_gameReg->m_30)
                                    ->m_8->m_map48.Lookup(lastSlot->m_c, out)
                                && out) {
                                if (out->m_7c->m_18) {
                                    out->m_7c->m_18->ResolveAnimation();
                                }
                            }
                            ClearRow(i);
                        }
                    }
                }
                ((CBzData*)g_gameReg->m_7c)->MarkFlag(idx, i);
                return 0;
            }
        }

        // Tail: reached when the mode is 1, or no winner resolved.
        if (m_25c) {
            m_25c->Activate(off);
        }
        if (g_gameReg->m_134 == 3) {
            if (obj->m_4f4 != 0 && m_10c[g_644c54] == 0) {
                Notify(4);
                return 0;
            }
        }
        if (g_gameReg->m_134 == 1) {
            if (m_10c[g_644c54] != 0) {
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
        if ((i64)g_645588 - m_2b0 >= m_2b8) {
            obj->m_2dc->AdvanceGauge(1);
            m_2b8 = g_buteMgr.GetDwordDef("Multiplayer", "TimePerGoo", 0x258);
            m_2b0 = g_645588;
        }
        // Resource respawn timer.
        if ((i64)g_645588 - m_2c0 >= m_2c8) {
            obj->m_2dc->UpdateRezMachineWakeStatusBar();
            m_2c8 = g_buteMgr.GetDwordDef("Multiplayer", "TimePerResource", 0x7530);
            m_2c0 = g_645588;
        }
        // Last-player-standing: any other live player blocks the win Notify.
        for (i32 i = 0; i < 4; i++) {
            if (i == g_644c54) {
                continue;
            }
            PlayerSlot* slot = (PlayerSlot*)((char*)g_gameReg + 0x150 + i * 0x238);
            if (slot->m_28 && !slot->m_2c && !slot->m_24) {
                goto done;
            }
        }
        Notify(2);
    }
done:
    return 0;
}

SIZE_UNKNOWN(CAnimObj);
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
SIZE_UNKNOWN(CActivatable);
SIZE_UNKNOWN(CBzData);
SIZE_UNKNOWN(PlayerSlot);
SIZE_UNKNOWN(CGooWellMgr);

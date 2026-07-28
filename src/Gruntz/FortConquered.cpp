// FortConquered.cpp - CExitTrigger::AdvanceAnim @0x03f5f0 (1318 B), the
// exit-trigger's per-frame fort-conquest check.
//
// original TU: filename unknown (@identity-TODO). Split out of FortressFlag.cpp
// (wave3-I): the retail body's BIRTH POSITION is the lone 0x3f5f0-0x3fb16 text
// interval between the WormholeActs block (0x3f210-0x3f57d) and the wormhole trio
// (0x3fc70+), and its three private .data cells (0x20d154/0x20d168/0x20d16c) sit
// BEFORE the wormhole trio's band (0x20d194) in the 98%-monotone .data
// contribution order - so it CANNOT belong to the fortressflag obj at 0x45d30
// (whose band is 0x20d384+). CExitTrigger::RegisterActs proves the class identity:
// each registry-insertion arm stores ILT 0x1938 as its "A" handler, and that thunk
// jumps here. The +0x54/+0x58 accesses also agree with CExitTrigger's two derived
// fields, which do not exist in the 0x54-byte CFortressFlag.
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/GameRegMfcPtr.h>     // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>         // the manager's +0x150 roster / +0x13c view bounds
#include <Gruntz/GruntzPlayer.h>      // the per-player roster record (GetName / the gates)
#include <Gruntz/CurPlayer.h>         // g_curPlayer
#include <Gruntz/TriggerMgr.h>        // the +0x68 command grid (HitTestApply/FindGruntAt/...)
#include <Gruntz/BattlezData.h>       // the +0x7c HUD sink (MarkFlag)
#include <Gruntz/FontConfig.h>        // the +0x5c chat log (AddItem)
#include <Gruntz/SpriteRefTable.h>    // the +0x74 sprite table (GetSel)
#include <Gruntz/Warlord.h>           // the bound warlord logic (m_warlordLogic)
#include <Gruntz/Play.h>              // CPlay::m_startMarkers (the +0x370 CPtrArray)
#include <Gruntz/GameObjectFactory.h> // CreateGruntCreationPoint / CreateFortressFlag
#include <Gruntz/FreeNodePool.h>      // g_coordPool (the {x,y} node freelist)
#include <Gruntz/GameRegistry.h>      // CDDrawSurfaceMgr (the +0x30 world root)
#include <DDrawMgr/DDrawChildGroup.h> // the walked child list + the +0x48 id->object map
#include <Wwd/WwdGameObjectFamily.h>  // CWwdGameObjectA / CGameObject / NextChild
#include <Rez/FrameClock.h>           // g_engineFrameDelta (the anim-cursor tick)
#include <Utils/MapTyped.h>           // typed MFC map lookups
#include <rva.h>

// Structure decoded:
// the +0x1a0 sub-clock tick, the g_gameReg->m_134 mode gate, HitTestCell +
// dedup vs owner->m_124, the 5-CString "<A> was conquered by <B>!" HUD message,
// the config re-tag, the two handler-type re-home list walks + a g_freeList pop,
// and a per-object GAME_EXPLOSION3 eye-candy spawn.
// @confidence: high
// @source: pmf-xref:registeracts-ilt-0x1938+class-layout
// @early-stop
// ~78% (was a 0.5% stub): the whole body is reconstructed and every callee/global/
// string is bound - the head anim tick, the m_134 HitTestApply fast path, the
// FindGruntAt hit/miss split, the 5-CString conquest message, MarkFlag/ClearRowAndRefresh/
// CellDispatch, the warlord death + battle-alert resolve, both child-list re-home walks
// (incl. the g_coordPool pop + CPlay::m_startMarkers append) and the GAME_EXPLOSION3
// spawn. Two residues, both codegen-layout:
//   (a) ZERO-REGISTER COLOURING. retail dedicates ebx to the constant 0 for the whole
//       body (`cmp r,ebx` / `push ebx` / `mov [x],ebx`) and spills `this` to esp+0x1c,
//       reloading it in the second walk; cl gives ebx to `this` (never reloads) and
//       materialises the zero inline (`test r,r` / `push 0`/`xor eax,eax`). Both are
//       4-callee-saved solutions of the same pressure - a pure colour assignment, and
//       it is what cascades into the two `[eax+edi+0x158]` CSE-vs-recompute sites and
//       the `lea eax,[edx+ecx*8]`+0x174 vs `+0x150`+0x24 displacement folding.
//   (b) EPILOGUE DUPLICATION. retail tail-merges every exit into ONE bottom epilogue
//       (13 `jmp 0x3fb01`); cl inlines a full 9-instruction /GX epilogue at the three
//       exits that are the physical end of an if/else arm (base 4 rets vs retail 1).
//       Both the `goto done` and the if/else spellings produce byte-identical output,
//       so it is docs/patterns/identical-return-epilogue-tailmerge.md in reverse, not a
//       gate-spelling choice (the positive-gate lever does not run this way).
RVA(0x0003f5f0, 0x526)
i32 CExitTrigger::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (g_gameReg->m_134 == 1) {
        CWwdGameObjectA* trig = m_object;
        g_gameReg->m_cmdGrid->HitTestApply(trig->m_screenX, trig->m_screenY, &trig->m_area);
    } else if (m_resolved != 0) {
        i32 hitPlayer;
        i32 hitRow;
        CWwdGameObjectA* obj = m_object;
        CGrunt* hit =
            g_gameReg->m_cmdGrid
                ->FindGruntAt(obj->m_screenX, obj->m_screenY, &obj->m_area, &hitPlayer, &hitRow, 0);
        if (hit != 0) {
            i32 owningPlayer = m_object->m_124;
            if (hitPlayer == owningPlayer) {
                goto done;
            }
            m_resolved = 0;
            GruntzPlayer* loser = &g_gameReg->m_options[owningPlayer];
            GruntzPlayer* winner = &g_gameReg->m_options[hitPlayer];
            if (loser != 0) {
                g_gameReg->m_chatLog->AddItem(
                    static_cast<const char*>(
                        loser->GetName() + " was conquered by " + winner->GetName() + "!"
                    ),
                    0,
                    0x11
                );
                loser->m_clearedRound = 1;
            }
            g_gameReg->m_scoreHud->MarkFlag(hitPlayer, owningPlayer);
            g_gameReg->m_cmdGrid->ClearRowAndRefresh(owningPlayer);
            g_gameReg->m_cmdGrid->CellDispatch(hitPlayer, hitRow, 0xd, -1);
            if (m_warlordLogic != 0) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = 0;
            }
            GruntzPlayer* claimed = &g_gameReg->m_options[hitPlayer];
            if (claimed != 0) {
                CGameObject* found = 0;
                CGameObject* warlordObj = 0;
                if (MapLookupById(
                        g_gameReg->m_world->m_childGroup->m_map48,
                        claimed->m_00c,
                        found
                    )) {
                    warlordObj = found;
                }
                CWarlord* wl = static_cast<CWarlord*>(warlordObj->m_7c->m_logic);
                if (wl != 0) {
                    wl->RaiseBattleAlert();
                }
            }
            CDDrawChildGroup* grp = g_gameReg->m_world->m_childGroup;
            POSITION pos = grp->m_list.GetHeadPosition();
            while (pos != 0) {
                CGameObject* cur = grp->NextChild(pos);
                if (cur->m_7c->m_notify == CreateGruntCreationPoint && cur->m_124 == owningPlayer) {
                    cur->m_124 = hitPlayer;
                    CShadeTable* tbl = g_gameReg->m_spriteFactory->GetSel(
                        g_gameReg->m_options[owningPlayer].m_008,
                        0
                    );
                    cur->m_drawActive = 1;
                    cur->m_drawFillCmd = 0xa;
                    cur->m_drawFillArg = tbl;
                    if (hitPlayer == g_curPlayer) {
                        CoordPoolNode* head = g_coordPool.m_freeHead;
                        Coord* mark = 0;
                        if (head->m_next != 0) {
                            mark = &head->m_coord;
                            head = head->m_next;
                            g_coordPool.m_freeHead = head;
                        }
                        mark->m_x = (cur->m_screenX & ~0x1f) + 0x10;
                        mark->m_y = (cur->m_screenY & ~0x1f) + 0x10;
                        CPtrArray& marks =
                            static_cast<CPlay*>(g_gameReg->m_curState)->m_startMarkers;
                        marks.SetAtGrow(marks.GetSize(), mark);
                    }
                }
                if (cur->m_7c->m_notify == CreateFortressFlag && cur->m_124 == owningPlayer) {
                    cur->m_124 = hitPlayer;
                    CShadeTable* tbl = g_gameReg->m_spriteFactory->GetSel(
                        g_gameReg->m_options[owningPlayer].m_008,
                        0
                    );
                    cur->m_drawActive = 1;
                    cur->m_drawFillCmd = 0xa;
                    cur->m_drawFillArg = tbl;
                }
            }
            if (owningPlayer == g_curPlayer) {
                g_gameReg->m_cmdGrid->LoadFinishLevelSprite(5);
            } else {
                GruntzPlayer* board = &g_gameReg->m_options[owningPlayer];
                if (board != 0 && board->m_014 == 0) {
                    board->m_038.Clear();
                }
            }
        } else {
            // No grunt on the trigger: the fort's own player forfeits when its roster
            // slot is joined-but-not-cleared - re-home every creation point / fortress
            // flag it owns and pop a GAME_EXPLOSION3 over each one that sits inside the
            // visible world bounds.
            i32 lostPlayer = m_object->m_124;
            if (lostPlayer == g_curPlayer) {
                goto done;
            }
            GruntzPlayer* slot = &g_gameReg->m_options[lostPlayer];
            if (g_gameReg->m_options[lostPlayer].m_joined == 0) {
                goto done;
            }
            if (slot->m_clearedRound != 0) {
                goto done;
            }
            if (slot->m_doneFlag == 0) {
                goto done;
            }
            slot->m_clearedRound = 1;
            m_resolved = 0;
            if (m_warlordLogic != 0) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = 0;
            }
            CDDrawChildGroup* grp = g_gameReg->m_world->m_childGroup;
            POSITION pos = grp->m_list.GetHeadPosition();
            while (pos != 0) {
                CGameObject* cur = grp->NextChild(pos);
                GameObjNotifyFn who = cur->m_7c->m_notify;
                if (who == CreateGruntCreationPoint || who == CreateFortressFlag) {
                    if (cur->m_124 == m_object->m_124) {
                        i32 x = cur->m_screenX;
                        i32 y = cur->m_screenY;
                        if (x < g_gameReg->m_viewBounds.right && x >= g_gameReg->m_viewBounds.left
                            && y < g_gameReg->m_viewBounds.bottom
                            && y >= g_gameReg->m_viewBounds.top) {
                            CWwdGameObjectA* fx =
                                g_gameReg->m_world->m_childGroup
                                    ->CreateSprite(0, x, y, 0xf4240, "Explosion", 0x40003);
                            if (fx != 0) {
                                fx->ApplyLookupGeometry("GAME_EXPLOSION3", 0);
                                fx->m_124 = 0;
                                fx->m_114 = 0;
                            }
                        }
                        cur->m_flags |= 0x10000;
                    }
                }
            }
            g_gameReg->m_cmdGrid->ClearRow(m_object->m_124);
        }
    }

done:
    return 0;
}

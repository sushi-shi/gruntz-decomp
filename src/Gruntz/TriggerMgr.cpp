// TriggerMgr.cpp - CTriggerMgr, the playfield tile-object / switch-trigger grid
// manager (trace placeholder tomalla-23, C:\Proj\Gruntz). See TriggerMgr.h.
//
// ONE ORIGINAL TU = retail .text [0x77f80..0x7d7ca] (dossier 10b,
// docs/exe-map/interval-dossiers.md, verdict strong): the 37 CTriggerMgr leaves
// PLUS the iconloaders unit (4 sprite loaders) PLUS the seven foreign
// singletons embedded fn-by-fn between the CTriggerMgr runs (trigger-spawn /
// FX / icon / selection helpers - none forms a contiguous second obj). Its CRT
// init-fragment run (7@0x7d8f0) directly follows the block. The old
// `triggermgr` unit's other two intervals are split out per the same oracle:
// see TriggerMgrGrid.cpp (0x6b640) and TriggerMgrHitTest.cpp (0x759e0).
// ~CTriggerMgr @0x85c50 stays at end-of-file: it sits in the gruntzmgr-region
// manager-dtor pocket (0x85b50..0x86040, COMDAT-at-usage emission), not in
// this interval.
//
// Functions in retail-RVA order. Shared CTm* views + singleton externs live in
// <Gruntz/TriggerMgrViews.h>; each merged foreign singleton keeps its own
// donor views/externs (identities are placeholders, @identity-TODO per the
// dossier ledger - no class renames in the migration). The 0x64556c singleton
// is reached under FIVE historical names in this one TU (g_gameReg /
// g_gameReg / g_gameReg / g_gameReg / g_gameReg), one per
// donor view - the canonical-CGameRegistry fold that unifies them is deferred
// cleanup work.
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)

#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // canonical CUserLogic (switch/trigger logic virtuals)
#include <Gruntz/TileGrid.h>          // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>             // canonical CButeMgr (one shape)
#include <Wwd/WwdFile.h>              // CPlaneRender - the canonical plane (dims here)
#include <stdlib.h>                   // rand (0x11fee0, reloc-masked)
#include <Globals.h>

#include <Gruntz/Play.h>         // CPlay (PostHudRect/DispatchHudClick drive HudRect @0x78060)
#include <Gruntz/GameLevel.h>    // CLevelPlane (PositionUpdate @0x788d0 tail call)
#include <Gruntz/GameRegistry.h> // canonical singleton view (icon/selection donors)
#include <Gruntz/Grunt.h>        // CGrunt (the board cells) + g_gameReg
#include <Gruntz/GruntPuddle.h>  // CGruntPuddle (the baseList element - ex CTmCandidate)
#include <Gruntz/GruntSpawnConfig.h> // CGruntSpawnConfig (g_gameReg->m_cueSink SpawnVoiceDriver ReportError)
#include <Gruntz/String.h>
#include <Gruntz/PickupType.h>      // the shared pickup/toy/tool id space (0x7c620)
#include <Gruntz/Brickz.h>          // CBrickzGrid (rock-break ComputeCellFlags)
#include <Dsndmgr/DirectSoundMgr.h> // canonical DSoundCloneInst (ConfigureItem @0x1360d0)
#include <Gruntz/SoundCue.h>        // CSndHost (the finish-level cue holder)
#include <Gruntz/LeafCue.h>         // LeafCue (the finish-level looked-up cue)
#include <Gruntz/LightFx.h>         // CLightFx (resurrect flash Activate)
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/LevelInfo.h>              // CLevelSpawnInfo - the concrete state behind m_curState
#include <Gruntz/TileTriggerContainer.h>   // canonical CTileTriggerContainer (rock-break)
#include <Gruntz/TileActionEvent.h>        // canonical CTileActionEvent (rock-break)
#include <Gruntz/TileTriggerSwitchLogic.h> // canonical CTileTriggerSwitchLogic (rock-break)
#include <Gruntz/TileGridCommand.h>        // canonical CTileTriggerLogic (rock-break)

#include <Gruntz/TriggerMgrViews.h>

DATA(0x00244ca4)
i32 g_groupSentinel;

// 0x77f80: FindNearestInRow(g) - the grunt-to-cell proximity probe: scan the 15 cells
// of grid row g->m_tileOwnerHi for the live cell whose display object (cell->m_10) is nearest g's
// tile position, but only when that squared distance is below the cutoff 2*g->m_defenderRadius.
// @early-stop
// 84->89.5: the m_5c distance term now loads BEFORE m_60 (dx declared before dy) matching
// retail's load order. Residual (~89.5%) is the tx/ty callee-saved coloring: retail homes
// tx->ebp (via `mov ebp,edx`) and ty->eax, computing tx>>5 before rowIdx*15; our cl colors
// tx->ebx, ty->ebp and schedules rowIdx*15 first. Every instruction + offset matches modulo
// register names; not source-steerable. topic:wall topic:regalloc.
RVA(0x00077f80, 0xab)
CTmCell* CTriggerMgr::FindNearestInRow(CTmCell* g) {
    i32 tx = g->m_lastTilePxX >> 5;
    i32 rowIdx = g->m_tileOwnerHi;
    CTmCell** cell = &m_grid[rowIdx * TM_GRID_COLS];
    i32 ty = g->m_lastTilePxY >> 5;
    CTmCell* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 i = 15;
    do {
        CTmCell* c = *cell;
        if (c != 0) {
            CGameObject* o = c->m_object;
            i32 dx = (o->m_screenX >> 5) - tx;
            i32 dy = (o->m_screenY >> 5) - ty;
            i32 d = dx * dx + dy * dy;
            if (d < bestDist && d < g->m_defenderRadius * 2) {
                best = c;
                bestDist = d;
            }
        }
        cell++;
        i--;
    } while (i != 0);
    return best;
}

// -------------------------------------------------------------------------
// CTriggerMgr::HudRect (0x78060) - the combat-region scan
// CPlay::PostHudRect / CPlay::DispatchHudClick invoke on m_4->m_cmdGrid (decl in
// <Gruntz/Play.h>'s WorldTimeline view). Merged from Play.cpp per dossier 10b
// (embedded singleton, this TU by retail position; @identity-TODO - the
// WorldTimeline view's +0x1c grunt grid / +0x23c goal ARE CTriggerMgr's
// m_grid/m_goal, the +0x68 registry slot GameRegistry.h already resolves to
// CTriggerMgr): screen-transform the world rect via the active viewport, then
// for each occupied grunt slot whose 30x30 screen box hits the rect, either
// re-arm the local player's grunt (Method_36ed/ResetCell29cd on g_curPlayer)
// or arm a foe's combat state (health sprite + CombatTimeout clock).

// @early-stop
// regalloc/CSE wall (~80% - and 0x78060 is not play's .obj, so the frame is re-scored):
// logic + instruction selection match, but cl pins `this`->ebx (retail ebp) and CSEs
// view->m_viewport once where retail reloads it per rect pair (a symmetric ebx<->ebp swap).
// this CTriggerMgr: the "+0x1c grunt slots" are m_grid, "+0x22c viewHost" is
// m_level, and the combat facets are the canonical CGrunt/CGameLevel shapes.)
RVA(0x00078060, 0x18d)
void CTriggerMgr::HudRect(RECT r, i32 flag) {
    CGameLevel* view = m_world->m_level;
    r.left += view->m_mainPlane->m_originX - view->m_planeCtx.left;
    r.top += view->m_mainPlane->m_originY - view->m_planeCtx.top;
    r.right += view->m_mainPlane->m_originX - view->m_planeCtx.left;
    r.bottom += view->m_mainPlane->m_originY - view->m_planeCtx.top;
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++) {
            CTmCell* g = m_grid[j];
            if (g) {
                CGameObject* pos = g->m_object;
                i32 cx = pos->m_screenX;
                i32 cy = pos->m_screenY;
                RECT box;
                SetRect(&box, cx - 0xf, cy - 0xf, cx + 0xf, cy + 0xf);
                if (r.left <= box.right && r.right >= box.left && r.top <= box.bottom
                    && r.bottom >= box.top) {
                    if (i == g_curPlayer) {
                        if (flag == 0 && g->m_entranceCommitted != 0) {
                            ResetAll(); // 0x36ed -> @0x78430
                            flag = 1;
                        }
                        ResetCell(g_curPlayer, j, 1, 1); // 0x29cd -> @0x6bfd0
                    } else {
                        g->CreateHealthSprite();
                        g->m_888 = g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
                        g->m_88c = 0;
                        g->m_880 = g_frameTime;
                        g->m_884 = 0;
                    }
                }
            }
        }
    }
}

// 0x78260: RemoveCellRecord(x, y, fromSelection) - when fromSelection, first unlink the
// (x,y) node from whichever of the 10 selection lists (+0x2d0) holds it. Then find the
// (x,y) node in the record list (+0x244); if present, optionally StopPendingFx, clear the
// grid cell's sprites (grid[y+15*x]), flag+clear the active goal/record (+0x234..+0x23c),
// tick the overlay if it owns (x,y), recycle the node and RemoveAt the +0x240 list; ret 1.
// ret 0 when no record. (__stdcall: ret 0xc.)
// @early-stop
// regalloc wall: retail pins this->ebp and the selection counter in [esp+0x1c]; under
// our cl's pressure this spills to [esp+0x10] and is reloaded. Logic + offsets + the
// free-list recycle byte-exact; the record-scan/goal/overlay path matches. topic:wall.
RVA(0x00078260, 0x165)
i32 CTriggerMgr::RemoveCellRecord(i32 x, i32 y, i32 fromSelection) {
    if (fromSelection != 0) {
        CPtrList* list = m_selLists;
        i32 k = 10;
        do {
            CTmNode* n = reinterpret_cast<CTmNode*>(list->GetHeadPosition());
            while (n != 0) {
                CTmNode* cur = n;
                n = n->m_next;
                i32* p = cur->m_payload;
                if (p[0] == x && p[1] == y) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(p);
                    slot->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                    list->RemoveAt(reinterpret_cast<POSITION>(cur));
                }
            }
            list++;
            k--;
        } while (k != 0);
    }
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    if (n == 0) {
        return 0;
    }
    CTmNode* cur;
    i32* p;
    do {
        cur = n;
        n = n->m_next;
        p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            goto found;
        }
    } while (n != 0);
    return 0;
found:
    if (m_recList.GetCount() == 1) {
        StopPendingFx();
    }
    CTmCell* cell = m_grid[y + x * TM_GRID_COLS];
    if (cell != 0) {
        (static_cast<CGrunt*>(cell))
            ->ClearAllSprites(); // CTmCell IS CGrunt (0x4b240); bridge-cast, see note
    }
    if (m_recX == p[0] && m_recY == p[1]) {
        CWwdGameObjectA* goal = m_goal;
        if (goal != 0) {
            goal->m_flags |= 0x10000;
            m_goal = 0;
        }
        m_armed = 0;
    }
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0 && ov->m_gridX == p[0] && ov->m_gridY == p[1]) {
        OverlayTick();
    }
    CoordPoolNode* slot = g_coordPool.NodeOf(p);
    slot->m_next = g_coordPool.m_freeHead;
    g_coordPool.m_freeHead = slot;
    m_recList.RemoveAt(reinterpret_cast<POSITION>(cur));
    return 1;
}

// 0x78430: ResetAll - drain the record list (+0x244): for each node, clear the
// referenced grid cell's sprites (grid[ y + 15*x ] @+0x1c) and recycle the node to
// the free list; RemoveAll the +0x240 list, run StopPendingFx, flag the goal (+0x23c).
// @early-stop
// 99.39% - TRIGGER: TU recomposition. First the Phase-1 TriggerMgrEh merge (100%
// ->99.39, /GX EH-state artifact), later recovered; the wave2-G dossier-10b
// one-TU merge re-flipped it: the residual is `add ecx,eax` vs `add eax,ecx` on
// the idx = payload[1]+15*payload[0] sum plus an ecx->edx coloring of the
// g_coordPool.m_freeHead load - pure /O2 environment sensitivity (same source scored 100 in
// the smaller TU). Operand-commutation respell is canonicalized away; the
// permuter finds no better spelling (FINAL 99.024, no change). The same merges
// IMPROVED siblings (DestroyGroup 64.8->66.4, ToggleRegionA 68.9->75.4,
// BuildRockBreakParticles 81.1->83.9). Accepted per the migration mandate.
// @early-stop
// 99.39% (was 100.00) - HEADER-SHAPE RIPPLE from the CSBI_RectOnly/CStatusBarMgr host split
// (2026-07-12). Not one line of this function changed. What changed is the class this TU
// includes for world->m_2dc: the 0x630 status-bar host stopped pretending to be a
// polymorphic CStatusBarItem (vptr at +0) and took its true non-polymorphic shape - an i32
// at +0 and eight REAL MFC CPtrList members at +0x2c. CPtrList is a full class with
// virtuals, so pulling its definition into the host's body moves MSVC5's per-TU /O2
// inlining budget, which recolours registers here (and costs TriggerCell -7.10 /
// CenterOnGroup -4.04 the same way). Residual is one scheduling pair; logic byte-identical.
// Correct shape, accepted cost - do NOT revert to recover the 0.61%.
RVA(0x00078430, 0x7f)
void CTriggerMgr::ResetAll() {
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    if (n != 0) {
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            i32 idx = payload[1] + TM_GRID_COLS * payload[0];
            CTmCell* cell = m_grid[idx];
            if (cell != 0) {
                (static_cast<CGrunt*>(cell))
                    ->ClearAllSprites(); // CTmCell IS CGrunt (0x4b240); bridge-cast, see note
                CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                slot->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        } while (n != 0);
    }
    m_recList.RemoveAll();
    StopPendingFx();
    CWwdGameObjectA* goal = m_goal;
    if (goal != 0) {
        goal->m_flags |= 0x10000;
        m_goal = 0;
    }
}

// 0x784d0: RecordListHas(x, y) - scan the record list (+0x244) for a node whose
// payload (x,y) matches; ret 1 on hit, 0 otherwise.
// @early-stop
// loop-epilogue wall: retail loops with `jne top` and reuses the null next as the
// eax=0 return (fall-through); our cl emits `je end; jmp top` + a separate
// `xor eax,eax`. docs/patterns/identical-return-epilogue-tailmerge.md
RVA(0x000784d0, 0x3a)
i32 CTriggerMgr::RecordListHas(i32 x, i32 y) {
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    while (n != 0) {
        CTmNode* cur = n;
        n = n->m_next;
        i32* p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            return 1;
        }
    }
    return 0;
}

// 0x78520: ReportRecordsA(tag, gx, gy) - when the level flag (+0x400) is set, scan the
// record list (+0x244) tracking each node's payload[0] byte (firstByte) and collecting the
// payload[1] byte of each magic-group, un-triggered cell; if exactly one matched, EnqueueSingle
// it on g_gameReg->m_cmdSubMgr, else EnqueueMulti the whole collected array. (__stdcall: ret 0xc.)
RVA(0x00078520, 0x106)
void CTriggerMgr::ReportRecordsA(i32 tag, i32 gx, i32 gy) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 count = 0;
    u8 firstByte = 0;
    u8 bytes[0x70];
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    while (n != 0) {
        CTmNode* next = n->m_next;
        i32* payload = n->m_payload;
        firstByte = static_cast<u8>(payload[0]);
        CTmCell* cell = m_grid[payload[1] + payload[0] * TM_GRID_COLS];
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload[1]);
            count++;
        }
        n = next;
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        rep->EnqueueSingle(
            tag,
            firstByte,
            bytes[0],
            2,
            static_cast<i16>(gx),
            static_cast<i16>(gy),
            0,
            0
        );
    } else {
        rep->EnqueueMulti(
            tag,
            firstByte,
            count,
            bytes,
            2,
            static_cast<i16>(gx),
            static_cast<i16>(gy),
            0
        );
    }
}

// 0x78680: ReportRecordsB(tag, gx, gy, flag) - the four-way variant of ReportRecordsA: same
// firstByte/magic-group byte scan, then dispatch by (count==1, flag) to one of four
// EnqueueSingle/EnqueueMulti calls with report kind 3 (flag==0) or 9 (flag!=0). (ret 0x10.)
RVA(0x00078680, 0x189)
void CTriggerMgr::ReportRecordsB(i32 tag, i32 gx, i32 gy, i32 flag) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 count = 0;
    u8 firstByte = 0;
    u8 bytes[0x70];
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    while (n != 0) {
        CTmNode* next = n->m_next;
        i32* payload = n->m_payload;
        firstByte = static_cast<u8>(payload[0]);
        CTmCell* cell = m_grid[payload[1] + payload[0] * TM_GRID_COLS];
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = static_cast<u8>(payload[1]);
            count++;
        }
        n = next;
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        if (flag != 0) {
            rep->EnqueueSingle(
                tag,
                firstByte,
                bytes[0],
                9,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0,
                0
            );
        } else {
            rep->EnqueueSingle(
                tag,
                firstByte,
                bytes[0],
                3,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0,
                0
            );
        }
    } else {
        if (flag != 0) {
            rep->EnqueueMulti(
                tag,
                firstByte,
                count,
                bytes,
                9,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0
            );
        } else {
            rep->EnqueueMulti(
                tag,
                firstByte,
                count,
                bytes,
                3,
                static_cast<i16>(gx),
                static_cast<i16>(gy),
                0
            );
        }
    }
}

// 0x78880: ClearRecords - drain the record list (+0x244) back to the free list,
// then RemoveAll the +0x240 MFC pointer list. The free-list head is cached in a
// register across the loop (g_coordPool.m_freeHead read once, written each iteration).
// @early-stop
// prologue scheduling wall: the drain loop body is byte-exact, but retail batches
// `push edi; push esi` then loads esi=head/edi=bias; our cl interleaves the loads
// with the pushes. docs/patterns/zero-store-before-loop-inline-bound.md
RVA(0x00078880, 0x3c)
void CTriggerMgr::ClearRecords() {
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    if (n != 0) {
        i32 bias = g_coordPool.m_linkOffset;
        void* head = g_coordPool.m_freeHead;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            CoordPoolNode* slot =
                reinterpret_cast<CoordPoolNode*>(reinterpret_cast<char*>(cur->m_payload) - bias);
            slot->m_next = static_cast<CoordPoolNode*>(head);
            head = slot;
            g_coordPool.m_freeHead = static_cast<CoordPoolNode*>(head);
        } while (n != 0);
    }
    m_recList.RemoveAll();
}

// @early-stop
// /O2 x87 scheduling wall (~63%): logic byte-for-byte identical, but retail
// materialises the screen pos in GP regs and spills them to stack temps for the
// int->float `fild` (register pressure from the m_level->m_level->m_5c walk reusing
// edx), then uses `fmul mem`+fxch; our /O2 emits the shorter `fild [struct]`
// direct + `fmulp`. Confirmed NOT /O1 (o1 profile 45%). Pure scheduling/regalloc.
RVA(0x000788d0, 0x64)
i32 CTriggerMgr::ScrollToActiveRecord() {
    CGameObject* src = m_grid[m_recX * TM_GRID_COLS + m_recY]->m_object;
    i32 y = src->m_screenY;
    i32 x = src->m_screenX;
    CPlaneRender* t = m_world->m_level->m_mainPlane;
    float fy = static_cast<float>(y);
    float fx = static_cast<float>(x);
    if (!(t->m_flags & 1)) {
        fx *= t->m_scaleX;
        fy *= t->m_scaleY;
    }
    t->m_scaledX = fx;
    t->m_scaledY = fy;
    t->RecomputePlaneCoords();
    return 1;
}

extern "C" void IconClassInitA(); // 0x40288d
extern "C" void IconClassInitB(); // 0x402bad

RVA(0x00078960, 0x9b)
i32 CTriggerMgr::LoadCameraSprite() {
    if (m_goal != 0) {
        return 0;
    }

    i32 vx = g_gameReg->m_modeW;
    i32 vy = g_gameReg->m_modeH;
    i32 pos = (static_cast<CPlay*>(g_gameReg->m_curState))->m_guts->m_position;

    i32 ax, cx;
    if (pos != 0) {
        if (pos > 0 && pos <= 2) {
            ax = vx - 0x28;
            cx = vy - 0x28;
        }
    } else {
        ax = vx - 0xc8;
        cx = vy - 0x28;
    }

    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* spr = fac->CreateSprite(0, ax, cx, 0xf4240, "DoNothing", 1);
    m_goal = spr;
    spr->m_7c->m_notify(spr);
    m_goal->ApplyName("GAME_CAMERASPRITE");
    return 1;
}

RVA(0x00078a30, 0x10)
void CTriggerMgr::OverlayTick() {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov) {
        ov->Deactivate();
    }
}

// 0x78a50: PlaceObjectFull(x, y) - the largest tile-object trigger driver (0x845 B). Look up
// the magic-group record cell; if an overlay (+0x25c) owns it, forward (x,y) to it. Else, when
// no pending fx (+0x2a8) and the type-13 trigger check fails, rewind the world fx. Otherwise
// hit-test the (x,y) target (HitTest5) and run the dense per-kind jump table over the two
// coordinate sub-tables (DAT_00683ea0..eb4), building/dispatching the per-kind object and
// stashing the rebuilt cell. ret 1. (__thiscall: ret 0x8.) Reconstructed to plateau.
// @early-stop  (8.3% -> 18.3%)
// largest driver (0x845 B): the common path (record decode, overlay/fx fast-paths,
// CellHitTest, the viewport cell-type resolve, and the pending-fx>=0xdf tile-attr branch)
// is reconstructed faithfully. FRAME WALL: retail allocates all the tail's spill slots
// up front (`sub esp,0x18`, spilling this@[esp+0x18]/world@[esp+0x10]/cell@[esp+0x1c]/
// hitFlag@[esp+0x20]/kind@[esp+0x14]) for the dense per-kind jump table (0x78e09: kind-1
// -> the 0x4792cc byte table -> 0x479298 jumps, ~12 stanzas incl. the WrapCoord write-back
// to cell+0x414..0x428). The stubbed tail here uses fewer locals (`sub esp,0xc`), so the
// head's esp offsets + this-in-edi-vs-ebx regalloc shift wholesale - fully matching the
// head requires reconstructing that jump table. Deferred to the final sweep. topic:wall.
RVA(0x00078a50, 0x845)
i32 CTriggerMgr::PlaceObjectFull(i32 x, i32 y) {
    // Decode the single record cell (row*15 + col into the placed grid).
    CTmCell* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }
    // An active overlay eats the click.
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0 && ov->m_active != 0) {
        ov->Forward(x, y);
        return 1;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    if (m_pendingFxKind == 0) {
        // bridge-cast (as ClearAllSprites above): CTmCell IS CGrunt, so this binds to the
        // REAL ?CanShowStamina@CGrunt@@QAEHXZ (0x514a0). Declared on CTmCell it mangled to
        // ?CanShowStamina@CTmCell@@QAEHXZ - a PHANTOM no obj and no .LIB defines.
        if ((static_cast<CGrunt*>(cell))->CanShowStamina() == 0) {
            world->LoadCursorSprites(0, 0);
            return 1;
        }
    }
    // Hit-test the (x,y) target against the placed grid (startRow==5 -> all rows).
    i32 hitFlag = 0;
    if (CellHitTest(x, y, 0, 0, 5)) {
        hitFlag = 1;
    }
    // Resolve the tile-cell's type object from the level viewport (result discarded:
    // the virtual GetTypeId dispatch is kept for its side effect).
    CGameLevel* view = m_world->m_level;
    CPlaneRender* grid = view->m_mainPlane;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 cx = tx;
    if (tx < 0) {
        cx = 0;
    } else if (tx >= grid->m_gridW) {
        cx = grid->m_gridW - 1;
    }
    i32 cy = ty;
    if (ty < 0) {
        cy = 0;
    } else if (ty >= grid->m_gridH) {
        cy = grid->m_gridH - 1;
    }
    i32 cval = grid->m_tileGrid[grid->m_colOffsets[cy] + cx];
    if (cval != static_cast<i32>(0xeeeeeeee) && cval != -1) {
        // the tile's collision image set (m_imageSets data @+0x4c); slot 8 = GetCollisionAt
        CTileImageSet* tc = static_cast<CTileImageSet*>(view->m_imageSets.GetAt(cval & 0xffff));
        tc->GetCollisionAt(0, 0);
    }
    // Pending-fx dispatch.
    i32 pfk = m_pendingFxKind;
    if (pfk >= 0xdf) {
        i32 alt = cell->m_198;
        if (hitFlag != 0) {
            world->LoadCursorSprites(alt + 0xc8, 1);
            return 1;
        }
        CGruntzMapMgr* plane = g_gameReg->m_tileGrid;
        i32 attr;
        if (static_cast<u32>(tx) >= static_cast<u32>(plane->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(plane->m_height)) {
            attr = 1;
        } else {
            attr = plane->m_rowInts[ty][tx * 7];
        }
        if ((attr & 0x939) != 0 || (attr & 2) != 0) {
            world->LoadCursorSprites(pfk, 0);
        } else {
            world->LoadCursorSprites(alt + 0xc8, 1);
        }
        return 1;
    }
    // Block H (pfk < 0xdf): the dense per-kind jump table + write-back stanzas.
    // Reconstructed to plateau (see @early-stop above).
    static_cast<void>(hitFlag);
    return 1;
}

// 0x79520: ResetGroup(a14, a18, ...) - when the level flag (+0x400) is set, hit-test the
// magic group, classify the (a14,a18) target into one of three branches and place/report the
// matching cursor / lightfx / warpstone sprite; on factory success Init it and report it.
// ret 1 (0 on flag-clear / placement failure). (__stdcall: ret 0x1c.)
// @early-stop
// Reconstructed from-scratch 46.7->75.6% (was structurally wrong: a boolean-a28
// sel=0/1/2 dispatch). a28 is a SELECTOR: when nonzero sel=a28; else classify by
// hit/cell into sel 1/2/3, dispatched by a dec-ladder switch to three stanzas that
// report (ReportRecordsA / ReportRecordsB(flag) / EnqueueSingle) then per-stanza
// CreateSprite("LightFx")+notify with kindArg 2/1/3, sharing the Arm tail; stanzaB/C
// hit-owner gates funnel to a shared SpawnVoiceDriver reportError. Two block-layout
// inversions were needed (cell-decode je-layout, classify cell!=0-inline). Residual is
// the this/hit callee-saved coloring (retail this=edi/hit=esi, cl this=esi/hit=edi) and
// the a14/a18 register-vs-stack-spill in the PlaceA/CreateSprite calls - an intra-fn
// coloring wall that cascades through every operand; not source-steerable. topic:wall
// topic:regalloc.
RVA(0x00079520, 0x2e3)
i32 CTriggerMgr::ResetGroup(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28, i32 a2c) {
    static_cast<void>(a1c);
    static_cast<void>(a20);
    static_cast<void>(a24);
    if (m_groupFlag == 0) {
        return 0;
    }
    CTmCell* hit = this->Hit5(a14, a18, 0, 0, 5);
    CTmCell* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }

    // Classify into a selector: a28 (when nonzero) IS the selector; else derive from
    // hit/cell. sel 1/2/3 dispatch to the three report+spawn stanzas; else -> return 1.
    i32 sel;
    if (cell != 0) {
        if (cell->m_tileOwnerHi != g_curPlayer) {
            return 1;
        }
        if (a28 != 0) {
            sel = a28;
        } else if (hit == 0) {
            sel = 1;
        } else if (hit == cell) {
            m_pendingFxKind = 0;
            (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
            CGameObject* o = hit->m_object;
            this->PlaceA(o->m_screenX, o->m_screenY, a18, a14);
            return 1;
        } else {
            sel = 2;
        }
    } else {
        sel = (hit != 0) ? 2 : 1;
    }

    CGameObject* sprite;
    i32 kindArg;
    switch (sel) {
        case 1:
            this->ReportRecordsA(1, a14, a18);
            if (a2c == 0) {
                return 1;
            }
            sprite = m_world->m_childGroup->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
            sprite->m_7c->m_notify(sprite);
            kindArg = 2;
            goto arm;
        case 2:
            if (hit == 0) {
                this->ReportRecordsB(1, a14, a18, 0);
            } else {
                i32 owner = hit->m_tileOwnerHi;
                if (owner == g_curPlayer && g_traitorMode == 0) {
                    if (hit != cell) {
                        goto reportError;
                    }
                    i32 v = (hit->m_entranceReason <= 0x16) ? hit->m_entranceReason : hit->m_19c;
                    if (v != 0xf) {
                        i32 v2 =
                            (hit->m_entranceReason <= 0x16) ? hit->m_entranceReason : hit->m_19c;
                        if (v2 != 0x13) {
                            goto reportError;
                        }
                    }
                }
                this->ReportRecordsB(1, owner, hit->m_tileOwnerLo, 1);
            }
            if (a2c == 0) {
                return 1;
            }
            sprite = m_world->m_childGroup->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
            sprite->m_7c->m_notify(sprite);
            kindArg = 1;
            goto arm;
        case 3:
            if (hit != 0) {
                if (hit->m_tileOwnerHi == g_curPlayer && g_traitorMode == 0
                    && (hit != cell || hit->m_198 != 0x1e)) {
                    goto reportError;
                }
                g_gameReg->m_cmdSubMgr->EnqueueSingle(
                    1,
                    static_cast<char>(cell->m_tileOwnerHi),
                    static_cast<char>(cell->m_tileOwnerLo),
                    10,
                    static_cast<i16>(hit->m_tileOwnerHi),
                    static_cast<i16>(hit->m_tileOwnerLo),
                    0,
                    0
                );
            } else {
                g_gameReg->m_cmdSubMgr->EnqueueSingle(
                    1,
                    static_cast<char>(cell->m_tileOwnerHi),
                    static_cast<char>(cell->m_tileOwnerLo),
                    4,
                    static_cast<i16>(a14),
                    static_cast<i16>(a18),
                    0,
                    0
                );
            }
            if (a2c == 0) {
                return 1;
            }
            sprite = m_world->m_childGroup->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
            sprite->m_7c->m_notify(sprite);
            kindArg = 3;
            goto arm;
        default:
            return 1;
    }

arm:
    (static_cast<CUserLogic*>(sprite->m_7c->m_logic))
        ->Arm("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", kindArg, 1);
    return 1;

reportError:
    g_gameReg->m_cueSink->SpawnVoiceDriver(reinterpret_cast<i32>(cell), 0x324, -1, 0, -1, -1);
    return 0;
}

// 0x798d0: DestroyGroup(col, row, force) - lazily create the overlay sub-object (+0x25c) via
// new+ctor (the /GX frame guards the partially-constructed object); if it fails to take, tear
// it back down and ReportError(0x800a). When it already exists, route by the magic group to
// the place helper. ret 1 on placement. (__stdcall: ret 0x10.) Reconstructed to plateau.
// @early-stop
// /GX new+ctor wall: the placement-new lifetime + the teardown-on-failure path carry the EH
// frame whose state numbering + partial-object cleanup diverge from retail; the alloc/ctor/
// teardown shape is faithful. topic:wall topic:eh.
RVA(0x000798d0, 0x1b6)
i32 CTriggerMgr::DestroyGroup(i32 col, i32 row, i32 force) {
    static_cast<void>(force);
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov == 0) {
        m_overlay = new CActionOptionsMenuBar;
        if (this->Probe() == 0) {
            CActionOptionsMenuBar* o2 = m_overlay;
            if (o2 != 0) {
                o2->Dtor();
                operator delete(o2);
                m_overlay = 0;
            }
            g_gameReg->ReportError(0x800a, 0x3ff); // dual-view bridge; see SpawnPuddle
        }
        return 0;
    }
    if (ov->m_active != 0 || m_recList.GetCount() != 1) {
        return 0;
    }
    i32* rec = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
    char* cellp = reinterpret_cast<char*>(m_grid[rec[1] + rec[0] * TM_GRID_COLS]);
    if (cellp == 0 || *reinterpret_cast<i32*>((cellp + 0x1ec)) != g_curPlayer) {
        return 0;
    }
    if (this->PlaceCell(
            *reinterpret_cast<i32*>((cellp + 0x1f0)),
            *reinterpret_cast<i32*>((cellp + 0x1ec)),
            0
        )
        == 0) {
        return 0;
    }
    CGameLevel* view = m_world->m_level;
    CPlaneRender* pl = view->m_mainPlane;
    i32 ox = pl->m_originX - view->m_planeCtx.top + row;
    i32 oy = pl->m_originY - view->m_planeCtx.left + col;
    this->PlaceCell(oy, ox, 1);
    return 1;
}

RVA(0x00079b00, 0x15)
i32 CTriggerMgr::OverlayRelease() {
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov) {
        return ov->Render();
    }
    return 1;
}

// 0x79b30: ByteTableHas(b) - linear search the byte table (+0x264, count +0x268)
// for `b`; ret 1 on hit, 0 otherwise.
// @early-stop
// register-coloring wall (~85%): the un-peeled indexed loop now matches retail (plain for-loop,
// no explicit n<=0 guard so the count is tested once). The residual is pure regalloc - retail
// colors count in esi and the table in ecx (SIB `[eax+ecx]`, dl temp); our cl parks count in edx
// and the table in esi (SIB `[esi+eax]`, cl temp). Not source-steerable. topic:wall.
RVA(0x00079b30, 0x3e)
i32 CTriggerMgr::ByteTableHas(i32 b) {
    i32 n = m_byteArr.GetSize();
    u8* tbl = m_byteArr.GetData();
    for (i32 i = 0; i < n; i++) {
        if (b == tbl[i]) {
            return 1;
        }
    }
    return 0;
}

// 0x79b80: ReinitGroup(col, row) - when not already done (+0x284) and the level is active,
// Format a "Level%i" CString from the level index, read the WarpStone config color, hit-test
// the (col,row) target, lazily re-init the status-bar item (+0x2dc) and either flag it done or
// recycle the record node; mark +0x284 done. (__stdcall: ret 0x8.) Reconstructed to plateau.
// @early-stop
// /GX CString-temp wall: the Level%i Format temporary forces the EH frame whose state +
// cleanup diverge; the Format/GetColor/hit-test/status path is faithful. topic:wall topic:eh.
RVA(0x00079b80, 0x194)
i32 CTriggerMgr::ReinitGroup(i32 col, i32 row) {
    if (m_284 != 0) {
        return 0;
    }
    if (g_gameReg->m_134 != 1) {
        return 0;
    }
    CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
    CString name;
    name.Format("Level%i", lvl->m_levelIndex, 0);
    i32 color =
        g_buteMgr.GetIntDef(const_cast<char*>(static_cast<const char*>(name)), "WarpStone", 0);
    i32 hx = col;
    i32 hy = row;
    if (hy >= g_gameReg->m_viewOriginR || hy < g_gameReg->m_viewOriginL
        || hx >= g_gameReg->m_viewOriginB || hx < g_gameReg->m_viewOriginT) {
        lvl->ResetGoals(hy, hx);
    }
    // the main plane's coord wrap (thunk 0x295a -> ?WrapCoord@CDDrawWorkerHost@@ @0xa000;
    // receiver is level->m_mainPlane)
    CGameLevel* plane = g_gameReg->m_world->m_level;
    i32 outR = col;
    i32 outC = row;
    plane->m_mainPlane->WrapCoord(&outR, &outC);
    CStatusBarMgr* sbi = lvl->m_guts;
    if (sbi->m_hlBusy == 0) {
        if (sbi->m_position == 2) {
            sbi->Reset();
        }
        if (sbi->m_activeTab != 5) {
            sbi->Place(5, 3, 0);
        }
        sbi->Place(5, 1, 0);
        sbi->Run();
    }
    if (sbi->Place(color, outR, outC) != 0) {
        sbi->m_hlBusy = 1;
    } else {
        m_byteArr.InsertAt(m_byteArr.GetSize(), 0, 0);
    }
    m_284 = 1;
    return 1;
}

extern void __stdcall Eng_BuildNotifyA(i32 a); // 0x100930 (thunk 0x12fd); ret 4 = __stdcall

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (g_gameReg->m_134 != 1) {
        return;
    }
    if (m_284 == 0) {
        return;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    CStatusBarMgr* st = world->m_guts;
    if (st->m_retabNotify != 0) {
        operator delete(st->m_retabNotify);
        st->m_retabNotify = 0;
    }
    world->m_guts->m_hlBusy = 0;
    if (m_byteArr.GetSize() > 0) {
        m_byteArr.RemoveAt(m_byteArr.GetSize() - 1, 1);
        CStatusBarMgr* ctx = world->m_guts;
        if (*reinterpret_cast<i32*>(ctx) != 2 && ctx->m_activeTab == 5) {
            Eng_BuildNotifyA(0);
            world->m_guts->TryActivate();
        }
    }
    if (g_gameReg->m_134 == 1) {
        CTmCell* fx = m_pendingFx;
        if (fx != 0) {
            fx->ResolveDeathAnimation();
        }
    }
    this->RefreshB(6);
}

extern void __stdcall Eng_SpawnFx(i32 type, i32 x, i32 y, i32 a3, i32 a4, i32 a5); // 0x7c620

// 0x79ea0: SpawnTileFx(x, y, a3) - only when the active state is live
// (gameReg->m_134==1): read the tile at (x>>5, y>>5); if it carries neither the
// 0x40939 mask nor bit 0x2, spawn a type-0x14 fx centered on the tile. Otherwise
// (the bit path) map (a3-1) into the world's 4 fx anchors and spawn there. ret 1.
// __stdcall free function (cleans its own 3 args; retail ends in `ret 0xc`).
// @early-stop
// regalloc wall (~81%): body + offsets + the tile double-index byte-exact; retail homes
// gameReg in edi, the grid in esi and the width in ebx (3 callee-saved regs), our cl uses
// gameReg=esi/width=edi (2 regs). Not source-steerable. topic:wall topic:regalloc.
RVA(0x00079ea0, 0xc2)
i32 __stdcall SpawnTileFx(i32 x, i32 y, i32 a3) {
    if (g_gameReg->m_134 != 1) {
        return 0;
    }
    CGruntzMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = x >> 5;
    i32 ty = y >> 5;
    i32 tile;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        tile = 1;
    } else {
        tile = grid->m_rowInts[ty][tx * 8 - tx];
    }
    if ((tile & 0x40939) == 0 && (tile & 2) == 0) {
        Eng_SpawnFx(0x14, (tx << 5) + 0x10, (ty << 5) + 0x10, 0, a3, 0);
        return 1;
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    i32 idx = a3 - 1;
    CPlay::Anchor* rec = (static_cast<u32>(idx) < 4) ? &world->m_anchors[idx] : 0;
    if (rec != 0) {
        Eng_SpawnFx(0x14, rec->m_x, rec->m_y, 0, a3, 0);
    }
    return 1;
}

// 0x79fb0: NotifyCell(row, col, z) - the notify-cell hook CellDispatch tails into. If the
// cell is live and not yet notified, Recall it (when not recall-done), clear its tile-attr
// bit + reset the plane cell, null the grid slot, decrement the per-row count and, when z
// set, bump the per-row alt count and re-arm; mark the cell notified. (__stdcall: ret 0xc.)
// @early-stop
// 50.7->76.6 this audit: fixed the tile-attr column stride (*28 = 7 dwords, the SAME grid
// HitTestCell walks - the old *8 was a logic bug), RecallCell's real arg order (cell,x,y),
// tg cached once (only ->m_8 re-read: retail holds the CTileGrid ptr in edx across both
// stores), and the z!=0 path inline / z==0 far. Residual: retail homes the pos pair to a
// sub esp,8 frame with two DEAD stores while forwarding the regs (its spill pick; plain
// locals / whole-struct copy / inlined out-param getter all get DSE'd by our cl - measured
// 75.4/76.6/75.4), which cascades into the esi<->edi coloring + the late push ebp.
// topic:wall topic:regalloc.
RVA(0x00079fb0, 0x169)
void CTriggerMgr::NotifyCell(i32 row, i32 col, i32 z) {
    i32 idx = col * TM_GRID_COLS + row; // grid[col][row] base
    CTmCell* cell = m_grid[idx];
    if (cell == 0) {
        return;
    }
    if (cell->m_36c != 0) {
        return;
    }
    if (cell->m_arrivalPending == 0) {
        this->RecallCell(cell, cell->m_lastTilePxX, cell->m_lastTilePxY);
    }
    CTrigPoint pt;
    pt.m_x = cell->m_lastTilePxX;
    pt.m_y = cell->m_lastTilePxY;
    CGruntzMapMgr* tg = g_gameReg->m_tileGrid;
    i32 rowIdx = pt.m_y >> 5;
    i32 colByte = (pt.m_x >> 5) * 28; // 7-dword cell stride (the grid HitTestCell walks)
    (reinterpret_cast<char*>(tg->m_rows[rowIdx]))[colByte + 0x3] &= 0xdf;
    *reinterpret_cast<i32*>((reinterpret_cast<char*>(tg->m_rows[rowIdx]) + colByte + 0x4)) = -1;
    m_grid[idx] = 0;
    m_rowCount[col] -= 1;
    if (z != 0) {
        m_cellFlag[idx] = 1;
        m_rowStateB[col] += 1;
        i32 k = cell->m_entranceReason;
        if (k > 0x16) {
            k = cell->m_19c;
        }
        if (k == 0x14) {
            if (g_gameReg->m_134 == 1) {
                CTmCell* fx = m_pendingFx;
                if (fx != 0) {
                    fx->ResolveDeathAnimation();
                }
            }
            this->RefreshB(1);
        }
        cell->m_36c = 1;
        return;
    }
    i32 k = cell->m_entranceReason;
    if (k > 0x16) {
        k = cell->m_19c;
    }
    if (k == 0x14) {
        this->RefreshC();
    }
    m_rowStateC[col] += 1;
    cell->m_36c = 1;
}

RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
    if (sprite == 0) {
        // The *0x24556c singleton IS a CGruntzMgr; ReportError @0x08dc60 is ITS method.
        // Calling it through the CGameRegistry view emitted ?ReportError@CGameRegistry@@QAEXHH@Z,
        // a symbol NOTHING defines (unbound reloc -> link failure). Bridge-cast to the real
        // class (same object, same address) so the call binds to ?ReportError@CGruntzMgr@@.
        // The cast goes away when this TU's g_gameReg is retyped CGruntzMgr* outright, which is
        // blocked on ONE name conflict: the +0x70 field is `CTileGrid* m_tileGrid` in
        // GameRegistry.h (this TU reads it as a tile grid) but `CmdSinkV* m_cmdNotify` in
        // GruntzMgr.h -- one field, two names. Resolving it means renaming in GruntzMgr.h +
        // GruntzMgr.cpp, which a parallel lane owns.
        g_gameReg->ReportError(0x8009, 0x400);
        return 0;
    }
    sprite->m_7c->m_notify(sprite);
    sprite->m_124 = f124;
    sprite->m_114 = f114;
    sprite->m_118 = f118;
    return PlacePuddle(sprite, color);
}

// 0x7a240: PlacePuddle(sprite, color) - hand the sprite's placement params to its target's
// PlacePuddle; on failure flag the goal + ReportError(0x8009,0x401). On success walk the
// record list twice (full (x,y) match, then x-only when count>0x3b) flagging+unlinking each
// matching node, then RemoveAll the list. (__thiscall: ret 0x8.)
// @early-stop
// regalloc + dual-loop scheduling wall: the two record-walk loops + the found/unlinked flags
// spill to different stack slots than retail and the (x,y)==busy fast-path goto reorders.
// Logic + offsets + the RemoveAt/RemoveAll recycle byte-exact. topic:wall.
RVA(0x0007a240, 0x143)
i32 CTriggerMgr::PlacePuddle(CGameObject* sprite, i32 color) {
    CGruntPuddle* tgt = static_cast<CGruntPuddle*>(sprite->m_7c->m_logic);
    i32 d = sprite->m_118;
    if (d == 0) {
        d = 0x19;
    }
    if (tgt->Place(sprite->m_124, sprite->m_114, color, d) == 0) {
        tgt->m_38->m_flags |= 0x10000;
        g_gameReg->ReportError(0x8009, 0x401); // dual-view bridge; see SpawnPuddle
        return 0;
    }
    CTmRecNode* n = reinterpret_cast<CTmRecNode*>(m_baseList.GetHeadPosition());
    i32 manyFlag = (m_baseList.GetCount() > 0x3b) ? 1 : 0;
    i32 unlinked = 0;
    while (n != 0 && unlinked == 0) {
        CTmRecNode* cur = n;
        n = n->m_next;
        CGruntPuddle* o = cur->m_obj;
        if (o->m_tileX == tgt->m_tileX && o->m_tileY == tgt->m_tileY) {
            if (o->m_pending != 0) {
                tgt->m_38->m_flags |= 0x10000;
                return 0;
            }
            o->m_38->m_flags |= 0x10000;
            m_baseList.RemoveAt(reinterpret_cast<POSITION>(cur));
            unlinked = 1;
        }
    }
    if (manyFlag != 0 && unlinked == 0) {
        n = reinterpret_cast<CTmRecNode*>(m_baseList.GetHeadPosition());
        while (n != 0) {
            CTmRecNode* cur = n;
            n = n->m_next;
            CGruntPuddle* o = cur->m_obj;
            if (o->m_pending == 0) {
                o->m_38->m_flags |= 0x10000;
                m_baseList.RemoveAt(reinterpret_cast<POSITION>(cur));
            }
        }
    }
    m_baseList.AddTail(tgt);
    return 1;
}

RVA(0x0007a3f0, 0xd7)
i32 CTriggerMgr::LoadToyBoxIcon(i32 x, i32 y, i32 a3, i32 a4, i32 a5) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    i32 tx = x >> 5;
    i32 ty = y >> 5;

    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(fac->m_list.GetHeadPosition());
    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CGameObject* obj = cur->m_obj;
        void* init = static_cast<void*>(obj->m_7c->m_notify);
        if (init == static_cast<void*>(&IconClassInitA)
            || init == static_cast<void*>(&IconClassInitB)) {
            i32 ox = obj->m_screenX >> 5;
            i32 oy = obj->m_screenY >> 5;
            if (tx == ox && ty == oy) {
                return 0;
            }
        }
    }

    CWwdGameObjectA* spr = fac->CreateSprite(0, x, y, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        g_gameReg->ReportError(0x8009, 0x402);
        return 0;
    }
    spr->ApplyName("GAME_TOYBOX");
    spr->m_118 = a4;
    spr->m_130 = a5;
    spr->m_114 = a3;
    spr->m_stateFlags |= 1;
    return 1;
}

RVA(0x0007a510, 0x9e)
i32 CTriggerMgr::ClearRowAndRefresh(i32 startRow) {
    i32 row, last;
    if (startRow == 5) {
        row = 0;
        last = 3;
    } else {
        last = startRow;
        row = startRow;
    }
    if (row <= last) {
        CTmCell** cell = &m_grid[row * TM_GRID_COLS];
        i32 n = last - row + 1;
        do {
            i32 i = 15;
            do {
                CTmCell* c = *cell;
                if (c != 0 && c->m_deathAnimStarted == 0) {
                    (static_cast<CGrunt*>(c))
                        ->StartBombGruntRun(); // Recall==CGrunt::StartBombGruntRun @0x68520
                }
                cell++;
                i--;
            } while (i != 0);
            n--;
        } while (n != 0);
    }
    if (startRow == g_curPlayer) {
        m_groupFlag = 0;
    }
    // m_curState is the live CPlay state (Refresh==FlushPendingOps @0xda2d0,
    // SetStat==ArmSnapshot @0xd9240, m_2dc==CPlay::m_guts the SBI_RectOnly host @0x2dc).
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    world->FlushPendingOps();
    world->ArmSnapshot(0, 0xbb7);
    (static_cast<CStatusBarMgr*>(world->m_guts))->SetMode(1);
    return 1;
}

RVA(0x0007a5e0, 0x121)
i32 CTriggerMgr::RebuildOverlay(void* obj, i32 kind, i32 /*unusedC*/, i32 /*unusedD*/) {
    if (obj == 0) {
        return 0;
    }
    // Negated outer test (kind!=4 ... else kind==4): reproduces retail's block layout
    // where the kind==4 body is placed FAR (after the kind==7 body).
    // The kind-7/kind-4 self-probes ARE this manager's own (de)serialize entry
    // points reused as boolean probes: retail dispatches kind 7 -> Load (0x7abc0,
    // via ILT thunk 0x2644) and kind 4 -> ScanGroup (0x7a760, via thunk 0x2351),
    // both passing `obj` as the archive. (Were the fake Probe7/Probe4 decls; the
    // real callees are the in-unit round-trip pair.)
    if (kind != 4) {
        if (kind == 7) {
            if (this->Load(static_cast<CSerialArchive*>(obj)) == 0) {
                return 0;
            }
        }
    } else {
        if (this->ScanGroup(static_cast<CSerialArchive*>(obj)) == 0) {
            return 0;
        }
    }
    CSerialArchive* src = static_cast<CSerialArchive*>(
        obj
    ); // slots +0x2c/+0x30 ARE Read/Write (the ex-CTmOverlaySrc 13-slot view was the CSerialArchive scheme)
    // The three i64 timer pairs, snapshotted as raw 8-byte blocks (the GetA/GetB
    // getters copy bytes); (char*)& keeps retail's one-lea + biased-second-push shape.
    char* blk0 = reinterpret_cast<char*>(&m_timerBase);
    if (kind != 4) {
        if (kind == 7) {
            src->Read(blk0, 8);
            src->Read(blk0 + 8, 8);
        }
    } else {
        src->Write(blk0, 8);
        src->Write(blk0 + 8, 8);
    }
    char* blk1 = reinterpret_cast<char*>(&m_gooTimerBase);
    if (kind != 4) {
        if (kind == 7) {
            src->Read(blk1, 8);
            src->Read(blk1 + 8, 8);
        }
    } else {
        src->Write(blk1, 8);
        src->Write(blk1 + 8, 8);
    }
    char* blk2 = reinterpret_cast<char*>(&m_resourceTimerBase);
    if (kind != 4) {
        if (kind == 7) {
            src->Read(blk2, 8);
            src->Read(blk2 + 8, 8);
        }
    } else {
        src->Write(blk2, 8);
        src->Write(blk2 + 8, 8);
        return 1;
    }
    return 1;
}

void Ar_WriteId(void* id, i32 stride, void* archive); // 0x1b8760

// 0x7a760: ScanGroup(ar) - serialize the whole manager into archive `ar`: the 4x15 grid of
// cell ids, the four per-row arrays, the magic table + its bytes, the record + selection
// lists, the goal/overlay/state words. ret 0 when ar/level null or the overlay write fails.
// (__thiscall: ret 0x4.) [the manager's Serialize]
// @early-stop
// big serializer wall: 60+ archive Write calls; the grid/list write loops pin esi(ar)/ebx
// /edi differently than retail and the scratch slots differ. Logic + offsets byte-exact.
RVA(0x0007a760, 0x373)
i32 CTriggerMgr::ScanGroup(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = m_world;
    if (lvl == 0) {
        return 0;
    }
    CTmCell** cell = m_grid;
    i32 r = 4;
    do {
        i32 c = 15;
        do {
            CTmCell* g = *cell;
            i32 id = 0;
            if (g != 0) {
                id = g->m_object->m_188;
                Ar_WriteId(lvl->m_childGroup, id, ar);
            }
            ar->Write(&id, 4);
            cell++;
            c--;
        } while (c != 0);
        r--;
    } while (r != 0);
    ar->Write(m_rowCount, 0x10);
    ar->Write(m_cellFlag, 0xf0);
    ar->Write(m_rowStateB, 0x10);
    ar->Write(m_rowStateC, 0x10);
    i32 cnt = m_byteArr.GetSize();
    ar->Write(&cnt, 4);
    for (i32 i = 0; i < cnt; i++) {
        u8 b = m_byteArr.GetData()[i];
        ar->Write(&b, 1);
    }
    i32 flag24c = m_recList.GetCount();
    ar->Write(&flag24c, 4);
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    while (n != 0) {
        CTmNode* cur = n;
        n = n->m_next;
        ar->Write(cur->m_payload, 8);
    }
    CPtrList* list = m_selLists;
    i32 k = 10;
    do {
        i32 cnt2 = list->GetCount();
        ar->Write(&cnt2, 4);
        CTmNode* m = reinterpret_cast<CTmNode*>(list->GetHeadPosition());
        while (m != 0) {
            CTmNode* cur = m;
            m = m->m_next;
            ar->Write(cur->m_payload, 8);
        }
        list++;
        k--;
    } while (k != 0);
    CWwdGameObjectA* goal = m_goal;
    i32 goalId = 0;
    if (goal != 0) {
        goalId = goal->m_188;
    }
    ar->Write(&goalId, 4);
    CTmCell* ov = m_pendingFx; // the pending-fx grunt; its HUD carries the archive id
    i32 ovId = 0;
    if (ov != 0 && ov->m_object != 0) {
        ovId = ov->m_object->m_188;
    }
    ar->Write(&ovId, 4);
    ar->Write(m_274, 0x10);
    i32 cntC = m_baseList.GetCount();
    ar->Write(&cntC, 4);
    CTmRecNode* rn = reinterpret_cast<CTmRecNode*>(m_baseList.GetHeadPosition());
    while (rn != 0) {
        CTmRecNode* cur = rn;
        rn = rn->m_next;
        CGruntPuddle* obj = cur->m_obj;
        if (obj == 0) {
            return 0;
        }
        i32 oid = obj->m_object->m_188;
        Ar_WriteId(lvl->m_childGroup, oid, ar);
        ar->Write(&oid, 4);
    }
    i32 hasOv = (m_overlay != 0) ? 1 : 0;
    ar->Write(&hasOv, 4);
    if (m_overlay != 0) {
        if (this->SerializeOverlay(ar, 0, 0) == 0) { // overlay serialize self-call 0x7df8
            return 0;
        }
    } else {
        return 0;
    }
    ar->Write(&m_armed, 4);
    ar->Write(&m_284, 4);
    ar->Write(&m_phase, 4);
    ar->Write(&m_recX, 8);
    ar->Write(&m_countdownActive, 4);
    ar->Write(&m_3ec, 4);
    ar->Write(&m_groupFlag, 4);
    ar->Write(&g_curPlayer, 4);
    ar->Write(&g_groupSentinel, 4);
    ar->Write(&m_pendingFxKind, 4);
    ar->Write(&m_selSentinel, 4);
    return 1;
}

void RezFree(void* p); // 0x1b9b82 (__cdecl free used by the overlay teardown)

// 0x7abc0: Load(ar) - deserialize the whole trigger-mgr state (see the header). The
// grid + list loads resolve each stored key through the level's map, validating the
// found descriptor's type/sub-object; the overlay sub-object is rebuilt via new+Load.
// @early-stop
// /GX EH-state wall (same family as DestroyGroup / ApplySwitch in this TU): the
// full read/lookup/list-load body and the field offsets are faithful, but the
// overlay new-expression's partial-object cleanup states and the heavy stack-slot
// reuse (retail folds `this` and the lookup-out param into one slot) number/allocate
// differently than retail's __ehfuncinfo. topic:wall topic:eh.
RVA(0x0007abc0, 0x4b6)
i32 CTriggerMgr::Load(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    m_rollingballLoop = 0;
    m_teleportLoop = 0;
    m_rollingballWanted = 0;
    m_teleportWanted = 0;

    // The factory's embedded serialize map is the real MFC CMapPtrToPtr at +0x48
    // (Lookup @0x1b8760); documented embedded-member offset (see SpriteFactory.h).
    CMapPtrToPtr* map = &m_world->m_childGroup->m_map48;

    // the 4x15 placed-object grid (this[7..66], byte offsets +0x1c..+0x108)
    for (i32 base = 7; base < 0x43; base += 0xf) {
        for (i32 i = 0; i < 0xf; i++) {
            i32 key;
            ar->Read(&key, 4);
            void* cell = 0;
            if (key != 0) {
                void* found = 0;
                void* looked = map->Lookup(reinterpret_cast<void*>(key), found) ? found : 0;
                if (looked == 0) {
                    return 0;
                }
                cell = (static_cast<CGameObject*>(looked))->m_7c->m_logic;
                if (cell == 0) {
                    return 0;
                }
            }
            (reinterpret_cast<void**>(this))[base + i] = cell;
        }
    }

    // per-row state bands
    ar->Read(m_rowCount, 0x10);
    ar->Read(m_cellFlag, 0xf0);
    ar->Read(m_rowStateB, 0x10);
    ar->Read(m_rowStateC, 0x10);

    // the +0x260 byte table
    i32 count;
    u32 ci;
    ar->Read(&count, 4);
    CByteArray* arr = &m_byteArr;
    arr->SetSize(0, -1);
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        i32 b;
        ar->Read(&b, 1);
        arr->SetAtGrow(ci, b);
    }
    ClearRecords();

    // the +0x240 record list (nodes pulled off the shared free-list)
    ar->Read(&count, 4);
    CPtrList* rec = &m_recList;
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        CoordPoolNode* fl = g_coordPool.m_freeHead;
        void* node = 0;
        if (fl->m_next != 0) {
            node = &fl->m_coord;
            g_coordPool.m_freeHead = fl->m_next;
        }
        ar->Read(node, 8);
        rec->AddTail(node);
    }

    // the ten selection lists (+0x2d0, stride 0x1c)
    CPtrList* sel = m_selLists;
    i32 slot = 0xa;
    do {
        ar->Read(&count, 4);
        for (ci = 0; ci < static_cast<u32>(count); ci++) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            void* node = 0;
            if (fl->m_next != 0) {
                node = &fl->m_coord;
                g_coordPool.m_freeHead = fl->m_next;
            }
            ar->Read(node, 8);
            sel->AddTail(node);
        }
        sel++;
    } while (--slot != 0);

    // the type-5 singleton (+0x23c)
    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = map->Lookup(reinterpret_cast<void*>(key), found) ? found : 0;
            void* obj = (looked != 0
                         && (static_cast<CGameObject*>(looked))->GetClassId() == CLASSID_SERIALREF)
                            ? looked
                            : 0;
            m_goal = static_cast<CWwdGameObjectA*>(obj);
            if (obj == 0) {
                return 0;
            }
        }
    }

    // the pending-fx singleton (+0x2a0)
    {
        i32 key;
        ar->Read(&key, 4);
        if (key != 0) {
            void* found = 0;
            void* looked = map->Lookup(reinterpret_cast<void*>(key), found) ? found : 0;
            if (looked == 0) {
                return 0;
            }
            // the looked-up sprite's bound logic IS the pending-fx grunt (the creator
            // downcast every m_7c->m_logic consumer does)
            CTmCell* obj =
                static_cast<CTmCell*>((static_cast<CGameObject*>(looked))->m_7c->m_logic);
            m_pendingFx = obj;
            if (obj == 0) {
                return 0;
            }
        } else {
            m_pendingFx = 0;
        }
    }

    // the base object list (this+0): reload from count keys
    ar->Read(m_274, 0x10);
    m_baseList.RemoveAll();
    ar->Read(&count, 4);
    for (ci = 0; ci < static_cast<u32>(count); ci++) {
        i32 key;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        void* found = 0;
        void* looked = map->Lookup(reinterpret_cast<void*>(key), found) ? found : 0;
        if (looked == 0) {
            return 0;
        }
        void* obj = (static_cast<CGameObject*>(looked))->m_7c->m_logic;
        if (obj == 0) {
            return 0;
        }
        m_baseList.AddTail(obj);
    }

    // the overlay sub-object (+0x25c): tear down the old, rebuild + Load the new
    CActionOptionsMenuBar* old = m_overlay;
    if (old != 0) {
        old->Clear();
        RezFree(old);
        m_overlay = 0;
    }
    i32 hasOverlay;
    ar->Read(&hasOverlay, 4);
    if (hasOverlay != 0) {
        CActionOptionsMenuBar* ov = new CActionOptionsMenuBar;
        m_overlay = ov;
        if (ov->Deserialize(ar) == 0) {
            return 0;
        }
    }

    // tail scalars + two globals
    ar->Read(&m_armed, 4);
    ar->Read(&m_284, 4);
    ar->Read(&m_phase, 4);
    ar->Read(&m_recX, 8);
    ar->Read(&m_countdownActive, 4);
    ar->Read(&m_3ec, 4);
    ar->Read(&m_groupFlag, 4);
    ar->Read(&g_curPlayer, 4);
    ar->Read(&g_groupSentinel, 4);
    ar->Read(&m_pendingFxKind, 4);
    ar->Read(&m_selSentinel, 4);
    return 1;
}

// The stop-fx hook the TriggerCell driver uses (reloc-masked).
// 0x7b1b0: TriggerCell(x, y) - clear the pending-fx slot; only when the overlay sub-object
// (+0x25c) is live with its +0x2c set, look up the magic record cell, classify (x,y) and by
// the resulting kind spawn the matching fx sprite (kind 2 -> remapped 0x13, kind 3 -> alt
// 0x1e, else generic +0xc8 into +0x2a8), then Refresh + Record. ret 1. (ret 0x8.)
// @early-stop
// regalloc + switch-on-kind wall: the classify result drives a cmp/je ladder that pins ebx
// (world) and esi (cell) differently than retail, and the fx arg pushes spill. topic:wall.
RVA(0x0007b1b0, 0x12b)
i32 CTriggerMgr::TriggerCell(i32 x, i32 y) {
    CActionOptionsMenuBar* ov = m_overlay;
    m_pendingFxKind = 0;
    if (ov == 0 || ov->m_active == 0) {
        return 0;
    }
    CTmCell* cell;
    if (m_recList.GetCount() != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
        cell = m_grid[rec[1] + rec[0] * TM_GRID_COLS];
    }
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    i32 kind = this->Classify(x, y);
    if (kind == 2) {
        i32 alt = cell->m_entranceReason;
        if (alt > 0x16) {
            alt = cell->m_19c;
        }
        if (alt == 0x13) {
            g_gameReg->m_cmdGrid->Spawn(cell->m_lastTilePxX, cell->m_lastTilePxY, 0, 0, 0, 2, 1);
        }
    } else if (kind == 3) {
        if (cell->m_198 == 0x1e) {
            CGameObject* o = cell->m_object;
            g_gameReg->m_cmdGrid->Spawn(o->m_screenX, o->m_screenY, 0, 0, 0, 3, 1);
        }
    } else if (kind != 0) {
        i32 v = kind + kPendingFxIdBase;
        m_pendingFxKind = v;
        world->LoadCursorSprites(v, 0);
    }
    this->Refresh2();
    this->Record2(x, y);
    return 1;
}

RVA(0x0007b330, 0xc6)
i32 CTriggerMgr::LoadExplosionSprites(i32 geoB, i32 geoA, i32 variant, i32 dummy) {
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* spr = fac->CreateSprite(0, geoB, geoA, 0, "Explosion", 0x40003);
    if (spr) {
        i32 v = variant;
        if (v == 0) {
            v = (rand(), 1);
        }
        CString key;
        key.Format("GAME_EXPLOSION%d", v);
        spr->ApplyLookupGeometry(key, 0);
        spr->m_124 = v;
        spr->m_114 = 1;
    }
    return spr != 0;
}

void FormatStr(CString* out, const char* fmt, ...);

// @source: string-xref
// @early-stop
// regalloc/loop-strength-reduction wall: the logic - the M400c prep call, the
// radius rect, the doubly-nested tile scan, the col/row clamp, the cell-type
// dispatch (0x1e/0x1f rock-break + Particlez sprite + rate-limited sound,
// 0x21 giant-rock resurrect + "No giant rock logic found" CString diagnostic,
// 0x97/0x98/0x99 hazard reconcile), the PtInRect gate and the return-1/return-0
// tails - is byte-faithful (instruction selection, calls, constants, strings). The
// residual is the induction-variable set: retail carries pixelX/pixelY, the packed
// (tileX<<8) cell base and the loop bounds as spilled strength-reduced accumulators
// on a 0x1c frame, and pins the two loop counters (tileX->ebp, tileY->edi) with a
// specific spill coloring the /O2 recompile re-derives differently, cascading a
// +N [esp+N] shift across the body. Not source-steerable (see
// docs/patterns/loop-invariant-multiply-strength-reduce-vs-memreread.md +
// zero-register-pinning.md). Logic complete.
RVA(0x0007b440, 0x3f0)
i32 CTriggerMgr::BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4) {
    CombatCue(cx, cy, r, 6, a4);

    // The concrete game-state behind CGameRegistry::m_curState (the base CState is
    // proven <= 0x1c0, so a +0x2e4 read is a DERIVED state's field). Its shape here -
    // pad to +0x2e4, then the tile-trigger/cell container - is the SAME object
    // CBattlezMapConfig::LoadConfig reads as lvl->m_spawnInfo (also a +0x2c slot of its
    // owner, also +0x2e4 -> the container): <Gruntz/LevelInfo.h>'s CLevelSpawnInfo.
    // The downcast is the honest symptom of m_curState being typed as the base.
    CLevelSpawnInfo* root = reinterpret_cast<CLevelSpawnInfo*>(g_gameReg->m_curState);
    i32 tileCx = cx >> 5;
    i32 tileCy = cy >> 5;
    i32 hiX = tileCx + r;
    for (i32 tx = tileCx - r; tx <= hiX; tx++) {
        i32 pxX = (tx << 5) + 0x10;
        for (i32 ty = tileCy - r; ty <= tileCy + r; ty++) {
            i32 pxY = (ty << 5) + 0x10;
            if (pxX < 0x10 || pxY < 0x10) {
                continue;
            }
            CGameLevel* board = m_world->m_level; // the holder's CGameLevel
            CLevelPlane* grid = board->m_mainPlane;
            if (tx >= grid->m_wrapW || ty >= grid->m_wrapH) {
                continue;
            }
            i32 col;
            if (pxX < 0x10) {
                col = 0;
            } else if (tx >= grid->m_gridW) {
                col = grid->m_gridW - 1;
            } else {
                col = tx;
            }
            i32 row = (ty >= grid->m_gridH) ? grid->m_gridH - 1 : ty;
            i32 cell = grid->m_tileGrid[grid->m_colOffsets[row] + col];
            i32 type;
            if (cell == static_cast<i32>(0xeeeeeeee) || cell == -1) {
                type = 0;
            } else {
                CTileImageSet* o =
                    static_cast<CTileImageSet*>(board->m_imageSets.GetAt(cell & 0xffff));
                type = o->GetCollisionAt(0, 0);
            }

            if (type != 0x1e && type != 0x1f) {
                if (type == 0x21) {
                    CGiantRockLogic* gr = root->m_2e4->ScanNeighborhood(tx, ty);
                    if (gr == 0) {
                        CString msg;
                        FormatStr(&msg, "No giant rock logic found around: x=%d, y=%d", cx, cy);
                        g_gameReg->EnterModalUI(msg);
                        g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_ROCK_SCAN_MISS);
                        return 0;
                    }
                    gr->BuildRockBreakInGameText();
                    root->m_2e4->DelFromList1(gr);
                    continue;
                }
                if (type != 0x97 && type != 0x98 && type != 0x99) {
                    continue;
                }
                CTileActionEvent* o = root->m_2e4->FindByField0C(ty + (tx << 8));
                if (o->Process(0)) {
                    root->m_2e4->DelFromList3(o);
                }
                continue;
            }

            // type == 0x1e || type == 0x1f: rock-break marker + particle
            CTileTriggerSwitchLogic* lo = reinterpret_cast<CTileTriggerSwitchLogic*>(
                (static_cast<CTileTriggerContainer*>(root->m_2e4))
                    ->FindInLists12(ty + (tx << 8), 0x1a)
            );
            if (lo != 0) {
                (reinterpret_cast<CTileTriggerLogic*>(lo))->ApplyMove(type);
                (static_cast<CTileTriggerContainer*>(root->m_2e4))
                    ->DelFromList1(static_cast<void*>(lo));
            } else {
                CLevelPlane* wg = g_gameReg->m_world->m_level->m_mainPlane;
                i32 off = wg->m_colOffsets[ty];
                if (type == 0x1e) {
                    wg->m_tileGrid[off + tx] = 0x5a;
                    (g_gameReg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5a);
                } else {
                    wg->m_tileGrid[off + tx] = 0x5b;
                    (g_gameReg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5b);
                }
            }

            POINT pt;
            pt.x = pxX;
            pt.y = pxY;
            if (!PtInRect(reinterpret_cast<const RECT*>(&g_gameReg->m_viewOriginL), pt)) {
                continue;
            }
            CWwdGameObjectA* spr =
                m_world->m_childGroup->CreateSprite(0, pxX, pxY, 0xcf84f, "Particlez", 0x40003);
            if (spr == 0) {
                continue;
            }
            spr->ApplyName("LEVEL_ROCKBREAK");
            spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);

            CSndHost* set = m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                // CSndHost's name map is an MFC CMapStringToPtr (RTTI-proven), so its
                // Lookup out-param is a void*& - the payload is the cue itself.
                void* e_ob = 0;
                set->m_10.Lookup("LEVEL_ROCKBREAK", e_ob);
                LeafCue* e = static_cast<LeafCue*>(e_ob);
                if (e != 0 && g_sndEnabled != 0) {
                    u32 now = g_killCueClock;
                    if (now - e->m_14 >= e->m_18) {
                        e->m_14 = now;
                        e->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

static const char s_LightFx[] = "LightFx";
static const char s_GAME_FLASH[] = "GAME_FLASH";
static const char s_GAME_LIGHTING_FLASH[] = "GAME_LIGHTING_FLASH";
static char s_Grunt[] = "Grunt";
static char s_CombatTimeout[] = "CombatTimeout";

// @early-stop
// prologue-scheduling + grid-pointer-register regalloc wall (~82%): the body is
// byte-correct in shape/offsets/symbols/CFG (the 4x15 grid scan, the +-((r<<5)+7)
// AABB, the tail-merged tier-1/6/7 ApplyCellEffect cases, the tier-2 teleport
// re-roll loop, the tier-3/5/4 health/toyz/freeze LightFx flashes + shared Activate
// tail all match instruction-for-instruction). Residual is the documented prologue
// coin-flip: retail spills `this` to esp+0x14 and materialises the running grid
// pointer with `lea 0x1c(ecx),eax` (this preserved, pointer in eax/memory), while cl
// spills `this` to esp+0x10 and reuses ecx via `add 0x1c,ecx` (pointer in ecx) - a
// one-slot shift that cascades displacement bytes through the whole prologue + loop
// control. Source-invariant; deferred to the final sweep.
RVA(0x0007b930, 0x3b7)
i32 CTriggerMgr::CombatCue(i32 x, i32 y, i32 radius, i32 tier, i32 flag) {
    i32 r = radius << 5;
    i32 xLo = x - r - 7;
    i32 yLo = y - r - 7;
    i32 xHi = x + r + 7;
    i32 yHi = y + r + 7;
    i32 rangeA = m_world->m_level->m_mainPlane->m_gridW - 2; // plane grid dims
    i32 rangeB = m_world->m_level->m_mainPlane->m_gridH - 2;

    CGrunt** p = m_grid; // the flat 4x15 board
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++, p++) {
            CGrunt* g = *p;
            if (g == 0) {
                continue;
            }
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            if (g->m_entranceDropActive != 0) {
                continue;
            }
            i32 gx = g->m_object->m_screenX;
            i32 gy = g->m_object->m_screenY;
            i32 lx = gx - 7;
            i32 ly = gy - 7;
            i32 hx = lx + 14;
            i32 hy = ly + 14;
            if (xLo <= hx && xHi >= lx && yLo <= hy && yHi >= ly) {
                switch (tier) {
                    case 1:
                        if (g->m_gruntKind != 0x38) {
                            CellDispatch(i, j, 0, flag);
                        }
                        break;
                    case 6:
                        if (g->m_gruntKind != 0x38) {
                            CellDispatch(i, j, 0xb, flag);
                        }
                        break;
                    case 7:
                        if (g->m_gruntKind != 0x38) {
                            CellDispatch(i, j, 2, flag);
                        }
                        break;
                    case 2: { // teleport-scatter
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 done = 0;
                        do {
                            i32 dx = rangeA ? GruntRand() % rangeA + 1 : GruntRand() & 1;
                            i32 dy = rangeB ? GruntRand() % rangeB + 1 : GruntRand() & 1;
                            if (g->TeleportMove(dx, dy, 0, 1)) {
                                CGameObject* spr =
                                    g_gameReg->m_world->m_childGroup
                                        ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                                done = 1;
                                spr->m_7c->m_notify(spr);
                                (static_cast<CLightFx*>(spr->m_7c->m_logic))
                                    ->Activate(
                                        reinterpret_cast<i32>(s_GAME_LIGHTING_FLASH),
                                        reinterpret_cast<i32>(s_GAME_FLASH),
                                        3,
                                        1
                                    );
                            }
                        } while (done == 0);
                        break;
                    }
                    case 3: { // health
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->m_health = 0x64;
                        g->UpdateCombatTimer();
                        g->m_combatTimeoutLo =
                            g_buteMgr.GetIntDef(s_Grunt, s_CombatTimeout, 0x1388);
                        g->m_combatTimeoutHi = 0;
                        g->m_combatClockLo = g_frameTime;
                        g->m_combatClockHi = 0;
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_7c->m_logic))
                            ->Activate(
                                reinterpret_cast<i32>(s_GAME_LIGHTING_FLASH),
                                reinterpret_cast<i32>(s_GAME_FLASH),
                                2,
                                1
                            );
                        break;
                    }
                    case 5: { // toyz
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 toy = GruntRand() % 9 + 0x17;
                        if (toy == 0x1e) {
                            toy = 0x20;
                        }
                        g->SetMoveStateA(toy, 1, 0, 0);
                        CGameObject* spr =
                            g_gameReg->m_world->m_childGroup
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_7c->m_logic))
                            ->Activate(
                                reinterpret_cast<i32>(s_GAME_LIGHTING_FLASH),
                                reinterpret_cast<i32>(s_GAME_FLASH),
                                7,
                                1
                            );
                        break;
                    }
                    case 4: { // freeze
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->FreezeApply();
                        CGameObject* h = g->m_object;
                        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                            0,
                            h->m_screenX,
                            h->m_screenY,
                            0xf4240,
                            s_LightFx,
                            0x40003
                        );
                        spr->m_7c->m_notify(spr);
                        (static_cast<CLightFx*>(spr->m_7c->m_logic))
                            ->Activate(
                                reinterpret_cast<i32>(s_GAME_LIGHTING_FLASH),
                                reinterpret_cast<i32>(s_GAME_FLASH),
                                9,
                                1
                            );
                        break;
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x0007be10, 0x34)
void CTriggerMgr::StopPendingFx() {
    CPlay* world = static_cast<CPlay*>(g_gameReg->m_curState);
    if (m_pendingFxKind == 0 && world->m_dragEndNotify == 0) { // m_504 == CPlay::m_dragEndNotify
        return;
    }
    world->LoadCursorSprites(0, 0); // CPlay::LoadCursorSprites @0xd0120
    m_pendingFxKind = 0;
}

// @early-stop
// regalloc/frame-layout wall (~65%): instruction selection, calls, constants,
// strings + the rect/loop/spawn structure are byte-faithful, but retail
// frame-allocates the `node` loop variable (a dedicated 4-byte slot at [esp+0x14]
// inside a 0x18 frame) while this /O2 recompile reuses an incoming-arg slot, yielding
// a 0x14 frame and a +4 cascade across every [esp+N] operand. Not source-steerable
// (the slot-vs-frame choice is the allocator's). Logic complete. See
// docs/patterns/zero-register-pinning.md + const-materialize-into-reg-vs-immediate.md.
RVA(0x0007be60, 0x21e)
i32 CTriggerMgr::LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r) {
    RECT rect;
    i32 hx = cx >> 5;
    i32 hy = cy >> 5;
    rect.left = hx - r;
    rect.right = hx + r;
    rect.top = hy - r;
    rect.bottom = hy + r;

    for (CTmRecNode* node = reinterpret_cast<CTmRecNode*>(m_baseList.GetHeadPosition()); node != 0;
         node = node->m_next) {
        CGruntPuddle* g = node->m_obj;
        if (g->m_pending != 0) {
            continue;
        }
        POINT pt;
        pt.x = g->m_tileX;
        pt.y = g->m_tileY;
        if (!PtInRect(&rect, pt)) {
            continue;
        }

        i32 type = g->m_gruntType;
        GruntzPlayer* cfg = &g_gameReg->m_options[type];
        i32 aiType = 0;
        i32 ok = 0;
        i32 px = (g->m_tileX << 5) + 0x10;
        i32 py = (g->m_tileY << 5) + 0x10;

        if (g_gameReg->m_134 == 1) {
            i32 radius = 0;
            if (cfg->m_014 == 0) {
                aiType = g_buteMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_buteMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (PlaceObject(
                    type,
                    px,
                    py,
                    0x186a0,
                    3,
                    g->m_placeIndex,
                    0,
                    0,
                    aiType,
                    radius,
                    0,
                    0,
                    0
                )
                != -1) {
                ok = 1;
            }
        } else if (cfg->m_liveGate != 0 && cfg->m_doneFlag == 0 && cfg->m_clearedRound == 0) {
            if (cfg->m_014 != 0) {
                if (PlaceObject(type, px, py, 0x186a0, 3, g->m_placeIndex, 0, 0, 0, 0, 0, 0, 0)
                    != -1) {
                    ok = 1;
                }
            } else if (cfg->m_038.Method_030990(g->m_tileX, g->m_tileY) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->m_38->m_flags |= 0x10000;
            m_baseList.RemoveAt(
                reinterpret_cast<POSITION>(node)
            ); // 0x1b4ac7 (retail then reads node->m_next)
            CGameObject* spr = g_gameReg->m_world->m_childGroup
                                   ->CreateSprite(0, px, py, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_notify(spr);
            (static_cast<CLightFx*>(spr->m_7c->m_logic))
                ->Activate(
                    reinterpret_cast<i32>("GAME_LIGHTING_FLASH"),
                    reinterpret_cast<i32>("GAME_FLASH"),
                    8,
                    1
                );
        }
    }
    return 1;
}

// 0x7c110: SpawnGrunt(col, row, a18, a1c) - find the first free column of grid row `row`;
// if full ret 0. Snap the source cell[col]'s display pos to a tile, run a prep self-call,
// create a "Grunt" sprite at that pos, Init it, place it via its userlogic; on placement
// failure flag the goal and ret 0, else stash the cell + bump the per-row counters. ret 1.
// (__stdcall: ret 0x10.)
// @early-stop
// regalloc + free-column-scan wall: the inner "first free col" loop and the snap arithmetic
// pin ebp/edi differently than retail; the placement-failure goal-flag path tail-merges.
RVA(0x0007c110, 0x166)
i32 CTriggerMgr::SpawnGrunt(i32 col, i32 row, i32 a18, i32 a1c) {
    CTmCell* src = m_grid[col * TM_GRID_COLS + a1c];
    i32 free = 0;
    CTmCell** rowBase = &m_grid[row * TM_GRID_COLS];
    if (*rowBase != 0) {
        CTmCell** p = rowBase;
        while (free < 15 && *p != 0) {
            p++;
            free++;
        }
    }
    if (free >= 15) {
        return 0;
    }
    CGameObject* o = src->m_object;
    i32 sx = (o->m_screenX & ~0x1f) + 0x10;
    i32 sy = (o->m_screenY & ~0x1f) + 0x10;
    i32 k = src->m_entranceReason;
    if (k > 0x16) {
        k = src->m_19c;
    }
    i32 vis = src->m_198;
    this->Reset3(col, k, vis); // prep self-call 0x7ec96
    CDDrawChildGroup* fac = m_world->m_childGroup;
    CWwdGameObjectA* sprite = fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == 0) {
        return 0;
    }
    sprite->m_7c->m_notify(sprite);
    // SETTLED FROM THE BINARY (the contradiction the deleted CTmCell view was hiding). Retail:
    //   mov ecx,[edi+0x7c] ; push edi ; call [ecx+0x10]   <- aux->Init(sprite)
    //   mov edx,[edi+0x7c] ; mov edi,[edx+0x18]           <- edi := aux->m_logic  (REASSIGNED)
    //   ... mov ecx,edi ; call <Place>
    //   mov DWORD PTR [esi+ebp*4+0x1c],edi                <- m_grid[..] := THE LOGIC
    // The grid holds the LOGIC (this grunt), not the CreateSprite result - the earlier
    // reconstruction stored the sprite, which is why the cell offsets never lined up with the
    // sprite's. The downcast to the concrete leaf is the authentic one every creator does.
    CGrunt* logic = static_cast<CGrunt*>(sprite->m_7c->m_logic);
    if (logic->Place(col, row, vis, k, 0, 0, 0, 0, 0, 0, 0, 0) == 0) {
        logic->m_38->m_flags |= 0x10000;
        return 0;
    }
    m_grid[row * TM_GRID_COLS + free] = logic;
    m_rowCount[row] += 1;
    m_cellFlag[(row * TM_GRID_COLS + free)] = 0;
    return 1;
}

// 0x7c2e0: CycleMoveIcons(skipRow, enable) - for grid rows 0..3 except `skipRow`, either
// roll a random move-icon onto each live cell (stashing the prior +0x1f8 when -1) and tick
// the world's region-4, or restore each cell's stashed icon. ret 1.
// @early-stop
// scheduling residual (~96%): logic + offsets + externs byte-exact. Declaring `r`
// before `grid` recovered retail's `xor eax,eax; lea edi` order (93->96). The residual
// is a spill-slot-OFFSET assignment: retail puts the row counter at [esp+0x10] and the
// grid ptr at [esp+0x14]; cl assigns them the opposite offsets (register roles r->eax,
// grid->edi already match). Not source-steerable. topic:wall topic:regalloc.
RVA(0x0007c2e0, 0xb5)
i32 CTriggerMgr::CycleMoveIcons(i32 skipRow, i32 enable) {
    i32 r = 0;
    CTmCell** grid = m_grid;
    for (; r < 4; r++) {
        if (r != skipRow) {
            CTmCell** cell = grid;
            i32 i = 15;
            do {
                CTmCell* g = *cell;
                if (g != 0) {
                    if (enable != 0) {
                        i32 t = rand() % 0x11;
                        if (g->m_1f8 == -1) {
                            g->m_1f8 = g->m_1f4_moveIcon;
                        }
                        (static_cast<CGrunt*>(g))
                            ->SelectMoveIcon(t); // -> ?SelectMoveIcon@CGrunt@@ (0x57800)
                        (static_cast<CPlay*>(g_gameReg->m_curState))->OnRegion4(1);
                    } else if (g->m_1f8 != -1) {
                        (static_cast<CGrunt*>(g))->SelectMoveIcon(g->m_1f8); // -> CGrunt (0x57800)
                        g->m_1f8 = -1;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        grid += 15;
    }
    return 1;
}

// @early-stop
// Dense 6-case switch (~61%). Collapsing the two separate address-taken locals
// (void* p_ob + LeafCue* p) into ONE reused LeafCue* p removed a spurious `push ecx`
// stack-slot alloc (retail colors the single local into the dead `state` arg slot):
// 55->61. Remaining residuals, all confirmed by llvm-objdump -dr base vs target:
// (1) the jump-table artifact - MSVC emits the table as a separate $L COMDAT, the
// delinker inlines it - documented ~79% ceiling, docs/patterns/
// switch-jumptable-separate-comdat.md. (2) case-1 /O2 scheduling: MSVC re-materializes
// edi=0 with a redundant `xor edi,edi` inside case 1 and reorders the p=0 store vs the
// m_soundRegistry member load; the window check `(clock-m_14) >= m_18` folds the
// memory operands into sub/cmp in retail but loads them to regs here. (3) case-block
// physical ordering (case 2/4/6 tails) differs. Not source-steerable.
RVA(0x0007c3d0, 0x1ae)
void CTriggerMgr::LoadFinishLevelSprite(i32 state) {
    switch (state) {
        case 1:
            if (m_phase != 2) {
                LeafCue* p = 0;
                m_world->m_soundRegistry->m_10.Lookup(
                    "GAME\\FINISHLEVEL",
                    reinterpret_cast<void*&>(p)
                );
                m_timerWindow = static_cast<u32>((p->m_10->m_durationMs + 500));
                m_timerBase = g_frameTime;
                CSndHost* h28 = m_world->m_soundRegistry;
                if (h28->m_emitGate == 0) {
                    p = 0;
                    h28->m_10.Lookup("GAME\\FINISHLEVEL", reinterpret_cast<void*&>(p));
                    if (p != 0 && g_sndEnabled != 0
                        && static_cast<u32>((g_killCueClock - p->m_14))
                               >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                    }
                }
                m_phase = 1;
                m_groupFlag = 0;
                m_3ec = state;
                return;
            }
            goto Lab_56b;
        case 2:
            m_phase = 1;
            break;
        case 3:
            if (m_phase == 0) {
                m_phase = 2;
                if (m_pendingFx != 0) {
                    m_pendingFx->ResolveDeathAnimation();
                }
            }
            goto Lab_522;
        case 4:
            m_phase = 2;
            m_timerWindow = 3000;
            m_timerBase = g_frameTime;
            goto Lab_56b;
        case 5:
            m_phase = 2;
            break;
        case 6:
            m_phase = 2;
        Lab_522:
            m_timerWindow = 3000;
            m_timerBase = g_frameTime;
            goto Lab_56b;
        default:
            return;
    }
    m_timerWindow = 3000;
    m_timerBase = g_frameTime;
Lab_56b:
    m_groupFlag = 0;
    m_3ec = state;
}

RVA(0x0007c620, 0x3c5)
i32 CTriggerMgr::LoadPowerupIconSprites(
    i32 type,
    i32 geoB,
    i32 geoA,
    i32 m130,
    i32 warpIdx,
    i32 m120
) {
    if (type == 0) {
        return 0;
    }

    CString name;
    switch (type) {
        case PICKUP_BOMB:
            name = "GAME_INGAMEICONZ_TOOLZ_BOMBZ";
            break;
        case PICKUP_BOOMERANG:
            name = "GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ";
            break;
        case PICKUP_BRICK:
            name = "GAME_INGAMEICONZ_TOOLZ_BRICKZ";
            break;
        case PICKUP_CLUB:
            name = "GAME_INGAMEICONZ_TOOLZ_CLUBZ";
            break;
        case PICKUP_GAUNTLETZ:
            name = "GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ";
            break;
        case PICKUP_GLOVEZ:
            name = "GAME_INGAMEICONZ_TOOLZ_GLOVEZ";
            break;
        case PICKUP_GOOBER:
            name = "GAME_INGAMEICONZ_TOOLZ_GOOBERZ";
            break;
        case PICKUP_GRAVITYBOOTZ:
            name = "GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ";
            break;
        case PICKUP_GUNHAT:
            name = "GAME_INGAMEICONZ_TOOLZ_GUNHATZ";
            break;
        case PICKUP_NERFGUN:
            name = "GAME_INGAMEICONZ_TOOLZ_NERFGUNZ";
            break;
        case PICKUP_ROCK:
            name = "GAME_INGAMEICONZ_TOOLZ_ROCKZ";
            break;
        case PICKUP_SHIELD:
            name = "GAME_INGAMEICONZ_TOOLZ_SHIELDZ";
            break;
        case PICKUP_SHOVEL:
            name = "GAME_INGAMEICONZ_TOOLZ_SHOVELZ";
            break;
        case PICKUP_SPRING:
            name = "GAME_INGAMEICONZ_TOOLZ_SPRINGZ";
            break;
        case PICKUP_SPY:
            name = "GAME_INGAMEICONZ_TOOLZ_SPYZ";
            break;
        case PICKUP_SWORD:
            name = "GAME_INGAMEICONZ_TOOLZ_SWORDZ";
            break;
        case PICKUP_TIMEBOMB:
            name = "GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ";
            break;
        case PICKUP_TOOB:
            name = "GAME_INGAMEICONZ_TOOLZ_TOOBZ";
            break;
        case PICKUP_WAND:
            name = "GAME_INGAMEICONZ_TOOLZ_WANDZ";
            break;
        case PICKUP_WARPSTONE:
            if (g_gameReg->m_134 == 1) {
                CString lvl;
                lvl.Format("Level%i", g_gameReg->m_curState->m_levelIndex);
                name.Format(
                    "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i",
                    g_buteMgr.GetInt("WarpStone", static_cast<const char*>(lvl))
                );
            } else {
                name.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", warpIdx);
            }
            break;
        case PICKUP_WELDER:
            name = "GAME_INGAMEICONZ_TOOLZ_WELDERZ";
            break;
        case PICKUP_WINGZ:
            name = "GAME_INGAMEICONZ_TOOLZ_WINGZ";
            break;
        case PICKUP_BABYWALKER:
            name = "GAME_INGAMEICONZ_TOYZ_BABYWALKERZ";
            break;
        case PICKUP_BEACHBALL:
            name = "GAME_INGAMEICONZ_TOYZ_BEACHBALLZ";
            break;
        case PICKUP_BIGWHEEL:
            name = "GAME_INGAMEICONZ_TOYZ_BIGWHEELZ";
            break;
        case PICKUP_GOKART:
            name = "GAME_INGAMEICONZ_TOYZ_GOKARTZ";
            break;
        case PICKUP_JACKINTHEBOX:
            name = "GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ";
            break;
        case PICKUP_JUMPROPE:
            name = "GAME_INGAMEICONZ_TOYZ_JUMPROPEZ";
            break;
        case PICKUP_POGOSTICK:
            name = "GAME_INGAMEICONZ_TOYZ_POGOSTICKZ";
            break;
        case PICKUP_SCROLL:
            name = "GAME_INGAMEICONZ_TOYZ_SCROLLZ";
            break;
        case PICKUP_SQUEAKTOY:
            name = "GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ";
            break;
        case PICKUP_YOYO:
            name = "GAME_INGAMEICONZ_TOYZ_YOYOZ";
            break;
        case PICKUP_MEGAPHONE:
            name = "GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ";
            break;
        case PICKUP_HEALTH1:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH1";
            break;
        case PICKUP_HEALTH2:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH2";
            break;
        case PICKUP_HEALTH3:
            name = "GAME_INGAMEICONZ_POWERUPZ_HEALTH3";
            break;
        case PICKUP_CONVERSION:
            name = "GAME_INGAMEICONZ_POWERUPZ_CONVERSION";
            break;
        case PICKUP_DEATHTOUCH:
            name = "GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH";
            break;
        case PICKUP_GHOST:
            name = "GAME_INGAMEICONZ_POWERUPZ_GHOST";
            break;
        case PICKUP_REACTIVEARMOR:
            name = "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR";
            break;
        case PICKUP_ROIDZ:
            name = "GAME_INGAMEICONZ_POWERUPZ_ROIDZ";
            break;
        case PICKUP_INVULNERABILITY:
            name = "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY";
            break;
        case PICKUP_SUPERSPEED:
            name = "GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED";
            break;
        case PICKUP_W:
            name = "GAME_INGAMEICONZ_SECRETW";
            break;
        case PICKUP_A:
            name = "GAME_INGAMEICONZ_SECRETA";
            break;
        case PICKUP_R:
            name = "GAME_INGAMEICONZ_SECRETR";
            break;
        case PICKUP_P:
            name = "GAME_INGAMEICONZ_SECRETP";
            break;
        case PICKUP_STOPWATCH:
            name = "GAME_INGAMEICONZ_POWERUPZ_STOPWATCH";
            break;
        case PICKUP_COIN:
            name = "GAME_INGAMEICONZ_POWERUPZ_COIN";
            break;
        case PICKUP_COVEREDTIMEBOMB: {
            CGameObject* tb = g_gameReg->m_world->m_childGroup
                                  ->CreateSprite(0, geoB, geoA, 0xf, "TimeBomb", 0x40003);
            if (tb) {
                tb->m_120 = g_buteMgr.GetDwordDef("Powerupz", "CoveredTimeBombTime", 0x7d0);
            }
            return tb != 0;
        }
        default:
            return 0;
    }

    CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup
                               ->CreateSprite(0, geoB, geoA, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        return 0;
    }
    spr->ApplyName(name);
    spr->m_120 = m120;
    spr->m_114 = 0;
    spr->m_118 = 0;
    spr->m_124 = 0;
    spr->m_11c = 0;
    spr->m_placeMode = 0;
    spr->m_12c = 0;
    spr->m_130 = m130;
    return 1;
}

// 0x7cc60: RebuildSelectionList(idx) - recycle selection list `idx` (+0x2d4) to the free
// list, RemoveAll it (+0x2d0), then allocate a fresh node per record-list entry (+0x244)
// copying its (x,y) payload; reset +0x3e8. ret 1.
// @early-stop
// regalloc wall (~86%): the free-list recycle + node-alloc bodies are byte-exact; retail
// pins this/idx-base differently across the two list walks. topic:wall.
RVA(0x0007cc60, 0xa7)
i32 CTriggerMgr::RebuildSelectionList(i32 idx) {
    CPtrList* sel = &m_selLists[idx];
    CTmNode* n = reinterpret_cast<CTmNode*>(m_selLists[idx].GetHeadPosition());
    if (n != 0) {
        void* head = g_coordPool.m_freeHead;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload != 0) {
                CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                slot->m_next = static_cast<CoordPoolNode*>(head);
                head = slot;
                g_coordPool.m_freeHead = static_cast<CoordPoolNode*>(head);
            }
        } while (n != 0);
    }
    sel->RemoveAll();
    CTmNode* rec = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    while (rec != 0) {
        CTmNode* cur = rec;
        rec = rec->m_next;
        i32* src = cur->m_payload;
        void** fh = reinterpret_cast<void**>(g_coordPool.m_freeHead);
        CoordPoolNode* fhNode = reinterpret_cast<CoordPoolNode*>(fh);
        i32* dst = 0;
        if (fhNode->m_next != 0) {
            dst = reinterpret_cast<i32*>(&fhNode->m_coord);
            g_coordPool.m_freeHead = fhNode->m_next;
        }
        dst[0] = src[0];
        dst[1] = src[1];
        sel->AddTail(dst);
    }
    m_selSentinel = -1;
    return 1;
}

// 0x7cd40: CenterSelectionGroup(slot) - ResetAll, tick the live overlay, then walk the
// slot's selection list (+0x2d4[slot*0x1c]). For each node look up grid[15*x+y]: if the
// cell is live, ResetCell(x,y,1,0) and (on the second pass for the same slot, m_3e8==slot)
// fold its display pos into a running bbox seeded from the level grid dims; if dead, recycle
// the node back to the free list and RemoveAt it from the slot list. On the centering pass
// scroll the world to the bbox centre and clear m_3e8; else latch m_3e8=slot. ret 1 (0 when
// the slot list is empty).
// @early-stop
// regalloc wall (~87%): logic + offsets + all reloc-masked externs (ResetAll/OverlayTick/
// ResetCell/RemoveAt/Center/g_coordPool.m_freeHead*/g_gameReg) byte-exact, but retail pins this=ebp,
// node=esi, y=edi (a perfect 5-reg fit); our cl swaps this/node into esi/ebp and spills
// `this` to the stack, reusing esi for y. Same systematic esi<->ebp swap the rest of this
// TU exhibits; not source-steerable. topic:wall.
RVA(0x0007cd40, 0x18f)
i32 CTriggerMgr::CenterSelectionGroup(i32 slot) {
    ResetAll();
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0 && ov->m_active != 0) {
        OverlayTick();
    }
    CTmNode* n = reinterpret_cast<CTmNode*>(m_selLists[slot].GetHeadPosition());
    if (n == 0) {
        m_selSentinel = -1;
        return 0;
    }
    i32 maxX = 0;
    i32 maxY = 0;
    CPlaneRender* grid = g_gameReg->m_world->m_level->m_mainPlane;
    i32 minX = grid->m_wrapW - 1;
    i32 minY = grid->m_wrapH - 1;
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* payload = cur->m_payload;
        i32 idx = payload[1] + TM_GRID_COLS * payload[0];
        CTmCell* cell = m_grid[idx];
        if (cell != 0) {
            ResetCell(payload[0], payload[1], 1, 0);
            if (m_selSentinel == slot) {
                CGameObject* disp = cell->m_object;
                i32 x = disp->m_screenX;
                i32 y = disp->m_screenY;
                if (x < minX) {
                    minX = x;
                }
                if (x > maxX) {
                    maxX = x;
                }
                if (y < minY) {
                    minY = y;
                }
                if (y > maxY) {
                    maxY = y;
                }
            }
        } else {
            CoordPoolNode* node = g_coordPool.NodeOf(payload);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_selLists[slot].RemoveAt(reinterpret_cast<POSITION>(cur));
        }
    } while (n != 0);
    if (m_selSentinel == slot) {
        (static_cast<CPlay*>(g_gameReg->m_curState))
            ->ResetGoals(
                minX + (maxX - minX) / 2,
                minY + (maxY - minY) / 2
            ); // 0xd5f00
        m_selSentinel = -1;
        return 1;
    }
    m_selSentinel = slot;
    return 1;
}

// @early-stop
// 83% - regalloc wall: the list walk, grid-hash (x*15 + y), bounding-box min/max
// fold, midpoint centre call and single-selection latch are byte-faithful; the
// residual is min/max register colouring + the doubled grid-lookup spill.  No EH.
RVA(0x0007cf40, 0x12e)
i32 CTriggerMgr::CenterOnGroup(i32 doSelect) {
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    if (n == 0) {
        return 0;
    }
    CPlaneRender* dims = g_gameReg->m_world->m_level->m_mainPlane;
    i32 minX = dims->m_wrapW - 1;
    i32 minY = dims->m_wrapH - 1;
    i32 maxX = 0;
    i32 maxY = 0;
    i32 count = 0;
    do {
        i32* k = n->m_payload; // the (x,y) record pair
        n = n->m_next;
        CTmCell* cell = m_grid[k[0] * TM_GRID_COLS + k[1]];
        if (cell != 0) {
            count++;
            CGameObject* g = cell->m_object;
            i32 gx = g->m_screenX;
            i32 gy = g->m_screenY;
            if (gx < minX) {
                minX = gx;
            }
            if (gx > maxX) {
                maxX = gx;
            }
            if (gy < minY) {
                minY = gy;
            }
            if (gy > maxY) {
                maxY = gy;
            }
        }
    } while (n != 0);
    i32 cy = minY + (maxY - minY) / 2;
    i32 cx = minX + (maxX - minX) / 2;
    i32 r = (static_cast<CPlay*>(g_gameReg->m_curState))->ResetGoals(cx, cy);
    if (r != 0 && count == 1 && m_recList.GetCount() == 1) {
        i32* head = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
        CTmCell* cell2 = m_grid[head[0] * TM_GRID_COLS + head[1]];
        if (cell2 != 0) {
            i32 v1f0 = cell2->m_tileOwnerLo;
            i32 v1ec = cell2->m_tileOwnerHi;
            if (RecordListHas(v1ec, v1f0)) {
                m_recX = v1ec;
                m_recY = v1f0;
                m_armed = 1;
                LoadCameraSprite();
            }
        }
    }
    return 1;
}

RVA(0x0007d0c0, 0x57)
void CTriggerMgr::ClearSelections() {
    CPtrList* list = m_selLists;
    i32 k = 10;
    do {
        CTmNode* n = reinterpret_cast<CTmNode*>(list->GetHeadPosition());
        if (n != 0) {
            void* head = g_coordPool.m_freeHead;
            do {
                CTmNode* cur = n;
                n = n->m_next;
                i32* payload = cur->m_payload;
                if (payload != 0) {
                    CoordPoolNode* slot = g_coordPool.NodeOf(payload);
                    slot->m_next = static_cast<CoordPoolNode*>(head);
                    head = slot;
                    g_coordPool.m_freeHead = static_cast<CoordPoolNode*>(head);
                }
            } while (n != 0);
        }
        list->RemoveAll();
        list++;
        k--;
    } while (k != 0);
    m_selSentinel = -1;
}

RVA(0x0007d140, 0x61)
i32 CTriggerMgr::ClearRow(i32 row) {
    CTmCell** cell = &m_grid[row * TM_GRID_COLS];
    i32 i = 15;
    do {
        CTmCell* c = *cell;
        if (c != 0 && c->m_deathAnimStarted == 0) {
            (static_cast<CGrunt*>(c))
                ->BuildGruntExitAnimation(); // ExitGrid==CGrunt::BuildGruntExitAnimation @0x641b0
        }
        cell++;
        i--;
    } while (i != 0);
    if (row == g_curPlayer) {
        m_groupFlag = 0;
    }
    (static_cast<CPlay*>(g_gameReg->m_curState))
        ->FlushPendingOps(); // Refresh==CPlay::FlushPendingOps @0xda2d0
    return 1;
}

RVA(0x0007d1d0, 0x9d)
i32 CTriggerMgr::NearestCellDist(i32 skipRow, i32 px, i32 py) {
    i32 tx = px >> 5;
    i32 ty = py >> 5;
    i32 best = 0x7fffffff;
    i32 r = 0;
    CTmCell** row = m_grid;
    do {
        if (r != skipRow) {
            i32 i = 15;
            CTmCell** cell = row;
            do {
                CTmCell* g = *cell;
                if (g != 0 && g->m_entranceCommitted != 0) {
                    CGameObject* o = g->m_object;
                    i32 dx = (o->m_screenX >> 5) - tx;
                    i32 dy = (o->m_screenY >> 5) - ty;
                    i32 d = abs(dx * dx + dy * dy);
                    if (d < best) {
                        best = d;
                    }
                }
                cell++;
                i--;
            } while (i != 0);
        }
        r++;
        row += 15;
    } while (r < 4);
    return best;
}

// 0x7d2a0: SelectionListFind(key, y) - only when key == g_curPlayer, scan the 10
// selection lists (+0x2d4, stride 0x1c) for a node whose payload (x,y) matches
// (key,y); ret the first matching list index, 0xa on a second match, else 0.
// @early-stop
// loop-rotation + interleaved-epilogue wall: the list-walk body is byte-exact, but
// retail loops with `jl top` (fall-through exit) and interleaves `mov eax,0xa` between
// the pops; our cl emits `jge end; jmp top` + a clean epilogue. topic:wall.
RVA(0x0007d2a0, 0x64)
i32 CTriggerMgr::SelectionListFind(i32 key, i32 y) {
    if (key != g_curPlayer) {
        return 0;
    }
    i32 result = 0;
    i32 i = 0;
    CPtrList* list = m_selLists;
    do {
        CTmNode* n = reinterpret_cast<CTmNode*>(list->GetHeadPosition());
        while (n != 0) {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload[0] == key && payload[1] == y) {
                if (result != 0) {
                    return 10;
                }
                result = i;
            }
        }
        i++;
        list++;
    } while (i < 10);
    return result;
}

// 0x7d330: DestroyAllAnims - DestroyAnims on every live grid cell (4 rows x 15), then
// walk the level's display-object list clearing the +0x200 marker on each object whose
// type-descriptor (obj+0x7c) slot-4 matches the switch tag, then stop the three sound
// channels (+0x3f0, +0x3f4, and the active grunt's +0x618).
// @early-stop
// regalloc wall: retail homes the row counter in ebp and the zero-compare const in ebx;
// our cl swaps them (ebx counter, ebp zero). Code bytes + offsets byte-exact, externs
// reloc-masked (DirectSoundMgr::StopAndRewind, ReadConfigFromButeMgr tag). topic:wall.
RVA(0x0007d330, 0xd3)
void CTriggerMgr::DestroyAllAnims() {
    CTmCell** cell = m_grid;
    i32 r = 4;
    do {
        i32 i = 15;
        do {
            CTmCell* g = *cell;
            if (g != 0) {
                (static_cast<CGrunt*>(g))
                    ->DestroyAnims(); // -> ?DestroyAnims@CGrunt@@QAEXXZ (0x57d80)
            }
            cell++;
            i--;
        } while (i != 0);
        r--;
    } while (r != 0);

    CDDrawGroupNode* node =
        reinterpret_cast<CDDrawGroupNode*>(m_world->m_childGroup->m_list.GetHeadPosition());
    while (node != 0) {
        CGameObject* obj = node->m_obj;
        node = node->m_next;
        if (obj != 0) {
            AnimWorkerObj* desc = obj->m_7c;
            // the grunt-notify stamp: workers bound to grunt logic carry
            // CGrunt::ReadConfigFromButeMgr as their raw notify fn (bit-compare)
            void (CTmCell::*tag)() = &CTmCell::ReadConfigFromButeMgr;
            if (*reinterpret_cast<void**>(&desc->m_notify) == *reinterpret_cast<void**>(&tag)) {
                (static_cast<CGrunt*>(desc->m_logic))->m_neighborCol = 0;
            }
        }
    }

    DirectSoundMgr* ch0 = m_rollingballLoop;
    if (ch0 != 0) {
        ch0->StopAndRewind();
        m_rollingballLoop = 0;
    }
    DirectSoundMgr* ch1 = m_teleportLoop;
    if (ch1 != 0) {
        ch1->StopAndRewind();
        m_teleportLoop = 0;
    }
    CState* state = g_gameReg->PickPausedThenPlayState();
    if (state != 0) {
        CStatusBarMgr* sub = (static_cast<CPlay*>(state))->m_guts;
        if (sub != 0) {
            DirectSoundMgr* ch2 = sub->m_destructButton;
            if (ch2 != 0) {
                ch2->StopAndRewind();
                sub->m_destructButton = 0;
            }
        }
    }
}

// 0x7d450: ToggleRegionA - clear a live pending-fx (LoadCursorSprites(0,0), ret 0); else,
// for the active record cell of the magic group, gate on CanShowStamina and dispatch by its
// logic kind (+0x170/+0x19c): kind 0x13 => ResetGroup, else set a pending fx (+0x2a8). ret 1.
// @early-stop
// 58.8->76.6 this audit: the cell decode is the negated-far if/else (`count!=1 -> cell=0`
// inline, lookup FAR - retail's je/xor/jmp layout) and the v==0x13 ResetGroup body sits
// INLINE with the pending-fx tail far. Residual: the pos-pair dead-store frame (retail
// homes cell->m_pos into a sub esp,8 frame while forwarding the regs into the ResetGroup
// pushes; struct copy / unused copy / i32[2] array spellings all DSE'd by our cl - same
// wall as NotifyCell) + the m_2a8=0 per-branch stores our cl hoists above the test.
// topic:wall topic:regalloc.
RVA(0x0007d450, 0x112)
i32 CTriggerMgr::ToggleRegionA() {
    if (m_pendingFxKind != 0) {
        m_pendingFxKind = 0;
        (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    // negated-condition-far-block: the lookup body lands FAR, cell=0 inline (retail layout)
    CTmCell* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }
    if ((static_cast<CGrunt*>(cell))->CanShowStamina()
        == 0) { // -> ?CanShowStamina@CGrunt@@ (0x514a0)
        OverlayTick();
        return 1;
    }
    i32 v = cell->m_entranceReason;
    if (v > 0x16) {
        v = cell->m_19c;
    }
    if (v == 0x13) {
        CTrigPoint pt;
        pt.m_x = cell->m_lastTilePxX;
        pt.m_y = cell->m_lastTilePxY;
        g_gameReg->m_cmdGrid->ResetGroup(pt.m_x, pt.m_y, 0, 0, 0, 2, 1);
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = v + kPendingFxIdBase;
    (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(v + kPendingFxIdBase, 0);
    OverlayTick();
    return 1;
}

// 0x7d5c0: ToggleRegionB - the sibling of ToggleRegionA: clear a live pending-fx; else, for
// the active record cell, gate on +0x170<0x17 and dispatch by +0x198: 0x1e => ResetGroup on
// the cell's display pos, 0 => just tick, else set a pending fx (+0x2a8). ret 1.
// @early-stop
// regalloc wall (~82%): logic + offsets + externs byte-exact; retail pins this->esi and the
// magic const into edi across the dispatch ladder. topic:wall.
RVA(0x0007d5c0, 0xdc)
i32 CTriggerMgr::ToggleRegionB() {
    if (m_pendingFxKind != 0) {
        m_pendingFxKind = 0;
        (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    CTmCell* cell;
    if (m_recList.GetCount() != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = (reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition()))->m_payload;
        cell = m_grid[rec[0] * TM_GRID_COLS + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }
    if (cell->m_entranceReason >= 0x17) {
        OverlayTick();
        return 1;
    }
    i32 kind = cell->m_198;
    if (kind == 0x1e) {
        CGameObject* o = cell->m_object;
        g_gameReg->m_cmdGrid->ResetGroup(o->m_screenX, o->m_screenY, 0, 0, 0, 3, 1);
        OverlayTick();
        return 1;
    }
    if (kind == 0) {
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = kind + kPendingFxIdBase;
    (static_cast<CPlay*>(g_gameReg->m_curState))->LoadCursorSprites(kind + kPendingFxIdBase, 0);
    OverlayTick();
    return 1;
}

// 0x7d6e0: EnqueueGroupCells - when armed (+0x400), collect the y-byte of every magic-group
// record cell with a clear +0x1e4 flag, then post the group to the command mgr (+0x6c):
// EnqueueSingle when exactly one, else EnqueueMulti with the y-byte buffer. ret 1.
// @early-stop
// stack-frame-size wall (~90%): the record scan (now with the matched byte counter, so the
// cl/byte-store/dword-reload sequence matches) + the two CGruntzCmdMgr enqueue calls are
// byte-exact; retail's frame is 0x88 vs our 0x6c (extra scratch slots) so every esp-relative
// displacement shifts by a constant. topic:wall.
RVA(0x0007d6e0, 0xea)
i32 CTriggerMgr::EnqueueGroupCells() {
    if (m_groupFlag == 0) {
        return 0;
    }
    u8 buf[0x68];
    u8 count = 0;
    char x = 0;
    CTmNode* n = reinterpret_cast<CTmNode*>(m_recList.GetHeadPosition());
    if (n != 0) {
        i32 magic = g_curPlayer;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* p = cur->m_payload;
            x = *reinterpret_cast<char*>(p);
            CTmCell* cell = m_grid[p[0] * TM_GRID_COLS + p[1]];
            if (cell->m_tileOwnerHi == magic && cell->m_entranceActive == 0) {
                buf[count] = (reinterpret_cast<u8*>(p))[4];
                count++;
            }
        } while (n != 0);
    }
    if (count == 1) {
        g_gameReg->m_cmdSubMgr->EnqueueSingle(1, x, static_cast<char>(buf[0]), 5, 0, 0, 0, 0);
    } else {
        g_gameReg->m_cmdSubMgr->EnqueueMulti(1, x, count, reinterpret_cast<u8*>(buf), 5, 0, 0, 0);
    }
    return 1;
}

// 0x85c50: ~CTriggerMgr - the /GX destructor: Cleanup (drain the lists), then the compiler
// auto-emits the reverse-order member teardown - the 10 selection lists (+0x2d0, EH state 2),
// the +0x260 byte array (state 1), the +0x240 record list (state 0) and the embedded base
// list (state -1) - from the real MFC CPtrList / CByteArray members. destructors. (__thiscall.)
// @early-stop
// /GX member-array dtor wall: the compiler-emitted member destructors + vector-dtor helper
// number their __ehfuncinfo states differently than retail; the teardown sequence is faithful.
// topic:wall topic:eh.
RVA(0x00085c50, 0x83)
CTriggerMgr::~CTriggerMgr() {
    Cleanup();
}


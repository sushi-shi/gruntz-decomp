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
#include <Gruntz/RockBreakMgr.h> // canonical CRockBreakMgr (body below)

#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // canonical CUserLogic (switch/trigger logic virtuals)
#include <Gruntz/TileGrid.h>      // canonical CTileGrid (the registry's +0x70 tile grid)
#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Wwd/WwdFile.h>          // CPlaneRender - the canonical plane (dims here)
#include <stdlib.h>               // rand (0x11fee0, reloc-masked)
#include <Globals.h>

// The *0x24556c singleton, typed as the REAL class (CGruntzMgr). This TU is MFC, so it
// sees it. Retyping kills BOTH the ((CGruntzMgr*)g_gameReg) dual-view bridge casts and the
// ((CTmGameReg*)g_gameReg) fake-view casts: every member this TU reads (m_curState/m_world/
// m_cmdGrid/m_cmdSubMgr/m_tileGrid/m_134/m_modeW/m_modeH/m_viewOriginL) is a real CGruntzMgr
// member at the same offset. The +0x70 board is now the REAL CGruntzMapMgr and the +0x6c
// sub-manager the REAL CGruntzCmdMgr, so those derefs are cast-free too.
extern "C" CGruntzMgr* g_gameReg;

// Merged-donor headers (the dossier-10b one-TU merge):
#include <Gruntz/Play.h>              // CWorld::WorldTimeline (HudRect @0x78060) + CPlay
#include <Gruntz/GameLevel.h>         // CLevelPlane (PositionUpdate @0x788d0 tail call)
#include <Gruntz/BoundaryTailViews.h> // CSnd788d0 (the 0x788d0 emitter view)
#include <Gruntz/GameRegistry.h>      // canonical singleton view (icon/selection donors)
#include <Gruntz/Grunt.h>             // CGruntTileMgr (CombatCue @0x7b930) + g_gameReg
#include <Gruntz/String.h>
#include <Gruntz/PickupType.h>      // the shared pickup/toy/tool id space (0x7c620)
#include <Gruntz/IconLoaderViews.h> // EngineLabelBacklog (the four icon loaders)
#include <Gruntz/Brickz.h>          // CBrickzGrid (rock-break ComputeCellFlags)
#include <Gruntz/SoundCueMgr.h>     // canonical CSoundCueMgr (ConfigureItem @0x1360d0)
#include <Gruntz/SoundCue.h>        // CSndHost (the finish-level cue holder)
#include <Gruntz/LeafCue.h>         // LeafCue (the finish-level looked-up cue)
#include <Gruntz/LightFx.h>         // CLightFx (resurrect flash Activate)
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/LevelInfo.h>              // CLevelSpawnInfo - the concrete state behind m_curState
#include <Gruntz/TileTriggerContainer.h>   // canonical CTileTriggerContainer (rock-break)
#include <Gruntz/TileActionEvent.h>        // canonical CTileActionEvent (rock-break)
#include <Gruntz/TileTriggerSwitchLogic.h> // canonical CTileTriggerSwitchLogic (rock-break)
#include <Gruntz/TileGridCommand.h>        // canonical CTileTriggerLogic (rock-break)

#include <Gruntz/TriggerMgrViews.h> // the shared CTm* views + singleton externs

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
    CTmCell** cell = &m_grid[rowIdx * 15];
    i32 ty = g->m_lastTilePxY >> 5;
    CTmCell* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 i = 15;
    do {
        CTmCell* c = *cell;
        if (c != 0) {
            CGruntHud* o = c->m_10;
            i32 dx = (o->m_5c >> 5) - tx;
            i32 dy = (o->m_60 >> 5) - ty;
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
// CWorld::WorldTimeline::HudRect (0x78060) - the combat-region scan
// CPlay::PostHudRect / CPlay::DispatchHudClick invoke on m_4w()->m_68 (decl in
// <Gruntz/Play.h>'s WorldTimeline view). Merged from Play.cpp per dossier 10b
// (embedded singleton, this TU by retail position; @identity-TODO - the
// WorldTimeline view's +0x1c grunt grid / +0x23c goal ARE CTriggerMgr's
// m_grid/m_goal, the +0x68 registry slot GameRegistry.h already resolves to
// CTriggerMgr): screen-transform the world rect via the active viewport, then
// for each occupied grunt slot whose 30x30 screen box hits the rect, either
// re-arm the local player's grunt (Method_36ed/ResetCell29cd on g_curPlayer)
// or arm a foe's combat state (health sprite + CombatTimeout clock).

DATA(0x00244c54)
extern "C" i32 g_curPlayer; // 0x644c54  local-player index
// @early-stop
// regalloc/CSE wall (~80% - and 0x78060 is not play's .obj, so the frame is re-scored):
// logic + instruction selection match, but cl pins `this`->ebx (retail ebp) and CSEs
// view->m_viewport once where retail reloads it per rect pair (a symmetric ebx<->ebp swap).
RVA(0x00078060, 0x18d)
void CWorld::WorldTimeline::HudRect(RECT r, i32 flag) {
    CombatView* view = m_viewHost->m_view;
    r.left += view->m_viewport->m_rect.left - view->m_originX;
    r.top += view->m_viewport->m_rect.top - view->m_originY;
    r.right += view->m_viewport->m_rect.left - view->m_originX;
    r.bottom += view->m_viewport->m_rect.top - view->m_originY;
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++) {
            CombatGrunt* g = m_grunts[j];
            if (g) {
                i32 cx = g->m_pos->m_screenX;
                i32 cy = g->m_pos->m_screenY;
                RECT box;
                SetRect(&box, cx - 0xf, cy - 0xf, cx + 0xf, cy + 0xf);
                if (r.left <= box.right && r.right >= box.left && r.top <= box.bottom
                    && r.bottom >= box.top) {
                    if (i == g_curPlayer) {
                        if (flag == 0 && g->m_1fc != 0) {
                            Method_36ed();
                            flag = 1;
                        }
                        ResetCell29cd(g_curPlayer, j, 1, 1);
                    } else {
                        g->CreateHealthSprite();
                        g->m_combatTimeout =
                            g_buteMgr.GetDwordDef("Grunt", "CombatTimeout", 0x1388);
                        g->m_88c = 0;
                        g->m_880 = g_645588;
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
            CTmNode* n = (CTmNode*)list->GetHeadPosition();
            while (n != 0) {
                CTmNode* cur = n;
                n = n->m_next;
                i32* p = cur->m_payload;
                if (p[0] == x && p[1] == y) {
                    void** slot = (void**)((char*)p - g_coordPool.m_linkOffset);
                    *slot = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = slot;
                    list->RemoveAt((POSITION)cur);
                }
            }
            list++;
            k--;
        } while (k != 0);
    }
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
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
    CTmCell* cell = m_grid[y + x * 15];
    if (cell != 0) {
        ((CGrunt*)cell)->ClearAllSprites(); // CTmCell IS CGrunt (0x4b240); bridge-cast, see note
    }
    if (m_recX == p[0] && m_recY == p[1]) {
        CTmGoal* goal = m_goal;
        if (goal != 0) {
            goal->m_8 |= 0x10000;
            m_goal = 0;
        }
        m_230 = 0;
    }
    CActionOptionsMenuBar* ov = m_overlay;
    if (ov != 0 && ov->m_gridX == p[0] && ov->m_gridY == p[1]) {
        OverlayTick();
    }
    void** slot = (void**)((char*)p - g_coordPool.m_linkOffset);
    *slot = g_coordPool.m_freeHead;
    g_coordPool.m_freeHead = slot;
    m_recList.RemoveAt((POSITION)cur);
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
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
    if (n != 0) {
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            i32 idx = payload[1] + 15 * payload[0];
            CTmCell* cell = m_grid[idx];
            if (cell != 0) {
                ((CGrunt*)cell)
                    ->ClearAllSprites(); // CTmCell IS CGrunt (0x4b240); bridge-cast, see note
                void** slot = (void**)((char*)payload - g_coordPool.m_linkOffset);
                *slot = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        } while (n != 0);
    }
    m_recList.RemoveAll();
    StopPendingFx();
    CTmGoal* goal = m_goal;
    if (goal != 0) {
        goal->m_8 |= 0x10000;
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
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
    if (n == 0) {
        return 0;
    }
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* p = cur->m_payload;
        if (p[0] == x && p[1] == y) {
            return 1;
        }
    } while (n != 0);
    return 0;
}

// 0x78520: ReportRecordsA(a14, a18, a1c, a20, a24) - when the level flag (+0x400) is set,
// scan the record list (+0x244) collecting the byte of each magic-group, un-triggered cell;
// if exactly one matched, hand it to the world's single-record reporter, else hand the whole
// collected array to the manager's multi-record reporter. (__stdcall: ret 0xc.)
// @early-stop
// reporter-dispatch arg-shape wall (~72%): the record scan is now byte-exact (u8 count +
// `bytes[count]=payload[4]` collected byte, size matches retail 0x106). The residual is the
// trailing count==1/else dispatch: retail calls two 8-arg reporter methods on g_gameReg->m_cmdSubMgr
// passing a per-iter firstByte dword slot (`*(u8*)payload` stored beside count) + the count/
// array as separate args; our 7-arg self-call ReportN/Report1 shape approximates it. topic:wall.
RVA(0x00078520, 0x106)
void CTriggerMgr::ReportRecordsA(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 bytes[0x88];
    u8 count = 0;
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
    while (n != 0) {
        CTmNode* next = n->m_next;
        u8* payload = (u8*)n->m_payload;
        CTmCell* cell = m_grid[*(i32*)(payload + 4) + *(i32*)payload * 15];
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = payload[4];
            count++;
        }
        n = next;
    }
    if (count == 1) {
        g_gameReg->m_cmdSubMgr->Report1(2, bytes[0], a14, a18, 0, a1c, 0);
    } else {
        this->ReportN(2, a14, bytes, a18, a1c, a20, a24);
    }
}

// 0x78680: ReportRecordsB(a14, a18, a1c, a20, a24, a28) - the four-way variant of
// ReportRecordsA: same magic-group byte scan, then dispatch by (count==1, a14!=0) to one of
// four (single/multi x kind 3/9) report calls. (__stdcall: ret 0x10.)
// @early-stop
// reporter-dispatch arg-shape wall (~62%): same fixed record scan as ReportRecordsA (u8 count +
// payload[4] collected byte). The residual is the 4-way (count==1 x a28) dispatch to the two
// 8-arg g_gameReg->m_cmdSubMgr reporter methods with the firstByte dword slot; our self-call shape
// approximates it. topic:wall.
RVA(0x00078680, 0x189)
void CTriggerMgr::ReportRecordsB(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28) {
    if (m_groupFlag == 0) {
        return;
    }
    u8 bytes[0x88];
    u8 count = 0;
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
    while (n != 0) {
        CTmNode* next = n->m_next;
        u8* payload = (u8*)n->m_payload;
        CTmCell* cell = m_grid[*(i32*)(payload + 4) + *(i32*)payload * 15];
        if (cell->m_tileOwnerHi == g_curPlayer && cell->m_entranceActive == 0) {
            bytes[count] = payload[4];
            count++;
        }
        n = next;
    }
    CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
    if (count == 1) {
        if (a28 != 0) {
            rep->Report1(9, bytes[0], a14, a18, 0, 0, 0);
        } else {
            rep->Report1(3, bytes[0], a14, a18, 0, 0, 0);
        }
    } else {
        if (a28 != 0) {
            this->ReportN(9, a14, bytes, a18, a1c, a20, a24);
        } else {
            this->ReportN(3, a14, bytes, a18, a1c, a20, a24);
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
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
    if (n != 0) {
        i32 bias = g_coordPool.m_linkOffset;
        void* head = g_coordPool.m_freeHead;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            void** slot = (void**)((char*)cur->m_payload - bias);
            *slot = head;
            head = slot;
            g_coordPool.m_freeHead = head;
        } while (n != 0);
    }
    m_recList.RemoveAll();
}

// 0x788d0: sound-emitter screen-position update, called by CMulti::PumpB/Tick
// (and CPlay::Render). Merged from Multi.cpp per dossier 10b (embedded
// singleton, this TU by retail position; the private-state oracle agrees: it
// reads NO globals, and its field walk is CTriggerMgr-shaped - m_1c[m_234*15+
// m_238] is m_grid[recX*15+recY], m_22c->m_24->m_5c the m_level->view->plane
// chain). @identity-TODO: kept the placeholder CSnd788d0 view
// (<Gruntz/BoundaryTailViews.h>) - no class renames in the migration.

// @early-stop
// /O2 x87 scheduling wall (~63%): logic byte-for-byte identical, but retail
// materialises m_5c/m_60 in GP regs and spills them to stack temps for the int->float
// `fild` (register pressure from the m_22c->m_24->m_5c walk reusing edx), then uses
// `fmul mem`+fxch; our /O2 emits the shorter `fild [struct]` direct + `fmulp`.
// Confirmed NOT /O1 (o1 profile 45%). Pure instruction scheduling/regalloc.
RVA(0x000788d0, 0x64)
i32 CSnd788d0::PositionUpdate() {
    ElemSrc788d0* src = m_1c[m_234 * 15 + m_238]->m_10;
    i32 v60 = src->m_60;
    i32 v5c = src->m_5c;
    Emitter788d0* t = m_22c->m_24->m_5c;
    float f60 = (float)v60;
    float f5c = (float)v5c;
    if (!(t->m_8 & 1)) {
        f5c *= t->m_18;
        f60 *= t->m_1c;
    }
    t->m_10 = f5c;
    t->m_14 = f60;
    ((CLevelPlane*)t)->RecomputePlaneCoords();
    return 1;
}

// ===========================================================================
// The four EngineLabelBacklog icon/sprite loaders (0x78960 / 0x7a3f0 / 0x7b330
// / 0x7c620) - the iconloaders unit, merged wholesale per dossier 10b (its 4
// fns are embedded between the CTriggerMgr runs; the unit's other two fns
// 0x1c070/0x1e720 stay in IconLoaders.cpp). Holder view + factory chain in
// <Gruntz/IconLoaderViews.h>; the singleton is the canonical CGameRegistry
// via g_gameReg (<Gruntz/Grunt.h>).
// ===========================================================================
// Two icon-class init-slot fns the toybox de-dup test compares an existing
// icon's m_7c->Init against (the in-game-icon / in-game-text classes).
// Declared extern so each `cmp esi, OFFSET fn` immediate carries a DIR32 reloc
// that pairs with retail's reloc at 0x40288d / 0x402bad (names are reloc-masked).
extern "C" void IconClassInitA(); // 0x40288d
extern "C" void IconClassInitB(); // 0x402bad

// ===========================================================================
// LoadCameraSprite @0x078960
// ===========================================================================
//
// Lazily creates the "DoNothing" camera sprite once (gated on this+0x23c being
// empty). It positions the sprite from the viewport (g_gameReg->m_viewportX/m_viewportY) offset
// by a per-tile bias selected from the current map's first travel count, runs the
// sprite's init virtual (vtbl slot +0x10), then caches its first frame.
// __thiscall (this @ esi). Returns 1 on (re)creation, 0 if already present.

// HOMED onto ::CTriggerMgr (was the placeholder host `EngineLabelBacklog`). PROVEN:
//  - retail calls 0x78960 with ecx = the game-mgr's +0x68 object, i.e. a CTriggerMgr
//    (CPlay::PositionBridgeToggle @0x0d5b20's tail: mov ecx,[esi+4]; mov ecx,[ecx+0x68];
//    ... mov [ecx+0x23c],0 ; call 0x3d1e -> 0x78960). That tail is the "GoalTail" call
//    LevelTileValidation.cpp used to route through a fake LvWorld::LvTimeline view.
//  - the host view's fields ARE CTriggerMgr's at the same offsets: m_factoryHolder
//    (+0x22c) == m_level (the same `m_level->m_8` factory hop SpawnPuddle makes), and
//    m_cameraSprite (+0x23c) == m_goal (the very slot the caller had just cleared).
//  - <Gruntz/TriggerMgr.h> ALREADY declared `i32 LoadCameraSprite(); // 0x78960` on
//    CTriggerMgr and SBI_RectOnly.cpp already CALLS it - so before this, the call emitted
//    ?LoadCameraSprite@CTriggerMgr@@QAEHXZ, a symbol nothing defined (unbound -> link fail).
// The two (CGameObject*)m_goal casts below are the residual +0x23c type split (CTmGoal vs
// CGameObject - one object, two views); reconciling those two classes is follow-up work.
RVA(0x00078960, 0x9b)
i32 CTriggerMgr::LoadCameraSprite() {
    if (m_goal != 0) {
        return 0;
    }

    i32 vx = g_gameReg->m_modeW;
    i32 vy = g_gameReg->m_modeH;
    i32 count = *(*(i32**)((char*)g_gameReg->m_curState + 0x2dc));

    i32 ax, cx;
    if (count == 0) {
        ax = vx - 0xc8;
        cx = vy - 0x28;
    } else if (count > 0 && count <= 2) {
        ax = vx - 0x28;
        cx = vy - 0x28;
    }

    CSpriteFactory* fac = m_level->m_8;
    CGameObject* spr = fac->CreateSprite(0, ax, cx, 0xf4240, "DoNothing", 1);
    m_goal = (CTmGoal*)spr;
    spr->m_7c->Init(spr);
    ((CGameObject*)m_goal)->ApplyName("GAME_CAMERASPRITE");
    return 1;
}

// 0x78a30: OverlayTick - dispatch the overlay sub-object's Tick when present.
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
        i32* rec = ((CTmNode*)m_recList.GetHeadPosition())->m_payload;
        cell = m_grid[rec[0] * 15 + rec[1]];
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
    CPlay* world = (CPlay*)g_gameReg->m_curState;
    if (m_pendingFxKind == 0) {
        // bridge-cast (as ClearAllSprites above): CTmCell IS CGrunt, so this binds to the
        // REAL ?CanShowStamina@CGrunt@@QAEHXZ (0x514a0). Declared on CTmCell it mangled to
        // ?CanShowStamina@CTmCell@@QAEHXZ - a PHANTOM no obj and no .LIB defines.
        if (((CGrunt*)cell)->CanShowStamina() == 0) {
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
    CTmLevelView* view = m_level->m_24;
    CPlaneRender* grid = view->m_5c;
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
    if (cval != (i32)0xeeeeeeee && cval != -1) {
        void* tc = view->m_4c[cval & 0xffff];
        (*(i32(**)(void*, i32, i32))(*(void***)tc + 8))(tc, 0, 0);
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
        if ((u32)tx >= (u32)plane->m_c || (u32)ty >= (u32)plane->m_10) {
            attr = 1;
        } else {
            attr = plane->m_8[ty][tx * 7];
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
    (void)hitFlag;
    return 1;
}

// 0x79520: ResetGroup(a14, a18, ...) - when the level flag (+0x400) is set, hit-test the
// magic group, classify the (a14,a18) target into one of three branches and place/report the
// matching cursor / lightfx / warpstone sprite; on factory success Init it and report it.
// ret 1 (0 on flag-clear / placement failure). (__stdcall: ret 0x1c.)
// @early-stop
// big switch-driver wall (0x2e3 B): the three-way classify ladder + the three CreateSprite
// /Init/Report stanzas pin esi(sprite)/ebx/ebp differently than retail and the string-arg
// pushes spill. Logic + offsets + the reloc-masked externs byte-exact. topic:wall.
RVA(0x00079520, 0x2e3)
i32 CTriggerMgr::ResetGroup(i32 a14, i32 a18, i32 a1c, i32 a20, i32 a24, i32 a28, i32 a2c) {
    (void)a1c;
    (void)a20;
    (void)a24;
    (void)a2c;
    if (m_groupFlag == 0) {
        return 0;
    }
    CTmCell* hit = this->Hit5(a14, a18, 0, 0, 5);
    CTmCell* cell;
    if (m_recList.GetCount() != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = ((CTmNode*)m_recList.GetHeadPosition())->m_payload;
        cell = m_grid[rec[1] + rec[0] * 15];
    }
    i32 sel;
    if (cell != 0 && cell->m_tileOwnerHi == g_curPlayer) {
        if (a28 != 0) {
            sel = 0;
        } else if (hit == 0) {
            sel = 1;
        } else if (hit == cell) {
            // toggle off the pending-fx and rewind
            m_pendingFxKind = 0;
            ((CPlay*)g_gameReg->m_curState)
                ->LoadCursorSprites(0, 0); // ILT 0x35da (was the StopFx2 phantom)
            CGruntHud* o = hit->m_10;
            this->PlaceA(o->m_5c, o->m_60, a18, a14);
            return 1;
        } else {
            sel = 2;
        }
    } else {
        sel = (hit != 0) ? 2 : 1;
    }

    CGameObject* sprite = 0;
    i32 kindArg = 0;
    i32 logicArg = 0;
    if (sel == 0) {
        // place-on-self path
        this->PlaceB(a14, a18, 1);
        return 1;
    } else if (sel == 1) {
        // spawn the cursor target sprite
        CGruntzCmdMgr* rep = g_gameReg->m_cmdSubMgr;
        if (cell != 0) {
            rep->Report1(1, cell->m_tileOwnerHi, cell->m_tileOwnerLo, a18, a14, 0, 0);
        } else {
            rep->Report1(1, a14, a18, 0, 0, 0, 0);
        }
        if (*(i32*)((char*)this + 0x2c) == 0) { // placeholder gate (see raw)
            return 0;
        }
        CSpriteFactory* fac = m_level->m_8;
        sprite = fac->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
        kindArg = 3;
        logicArg = 1;
    } else {
        // sel==2: place-and-report variant -> WarpStone factory
        this->PlaceB(a14, a18, 1);
        CSpriteFactory* fac = m_level->m_8;
        sprite = fac->CreateSprite(0, a14, a18, 0xf4240, "LightFx", 0x40003);
        kindArg = 2;
        logicArg = 1;
    }
    if (sprite == 0) {
        return 0;
    }
    sprite->m_7c->Init(sprite);
    void* logic = sprite->m_7c->m_logic;
    ((CUserLogic*)logic)->Arm("GAME_LIGHTING_TARGETCURSOR", "GAME_TARGETCURSOR", kindArg, logicArg);
    return 1;
}

// ---------------------------------------------------------------------------
// The /GX (eh) CTriggerMgr methods below (DestroyGroup 0x798d0, ReinitGroup
// 0x79b80, Load 0x7abc0, and ~CTriggerMgr 0x85c50 at end-of-file) own
// destructible locals / new+ctor lifetimes (CString temporaries, the overlay
// rebuild); the whole unit compiles /GX (the former TriggerMgrEh split is
// folded back - the real dev TU was one /GX unit, see ResetAll's note).
// LAYOUT NOTE: these methods touch `this` by raw offset in places (the
// opaque-shell convention); only offsets + reloc-masked helpers are modelled.

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
    (void)force;
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
    i32* rec = ((CTmNode*)m_recList.GetHeadPosition())->m_payload;
    char* cellp = (char*)m_grid[rec[1] + rec[0] * 15];
    if (cellp == 0 || *(i32*)(cellp + 0x1ec) != g_curPlayer) {
        return 0;
    }
    if (this->PlaceCell(*(i32*)(cellp + 0x1f0), *(i32*)(cellp + 0x1ec), 0) == 0) {
        return 0;
    }
    char* view = *(char**)((char*)m_level + 0x24);
    char* sc = *(char**)(view + 0x5c) + 0x40;
    i32 ox = *(i32*)(sc) - *(i32*)(view + 0x14) + row;
    i32 oy = *(i32*)(sc + 0x4) - *(i32*)(view + 0x10) + col;
    this->PlaceCell(oy, ox, 1);
    return 1;
}

// 0x79b00: OverlayRelease - release the overlay sub-object when present; ret 1.
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
    if (*(i32*)((char*)g_gameReg + 0x134) != 1) {
        return 0;
    }
    char* lvl = (char*)g_gameReg->m_curState;
    CString name;
    name.Format("Level%i", *(i32*)(lvl + 0x1c), 0);
    i32 color = g_buteMgr.GetIntDef((char*)(const char*)name, "WarpStone", 0);
    i32 hx = col;
    i32 hy = row;
    if (hy >= *(i32*)((char*)g_gameReg + 0x144) || hy < *(i32*)((char*)g_gameReg + 0x13c)
        || hx >= *(i32*)((char*)g_gameReg + 0x148) || hx < *(i32*)((char*)g_gameReg + 0x140)) {
        ((CPlay*)lvl)->ResetGoals(hy, hx); // ILT 0x2e28 (was the 3-arg Place2 phantom)
    }
    CTmGridHolder* plane = (CTmGridHolder*)*(char**)(*(char**)((char*)g_gameReg + 0x30) + 0x24);
    i32 outR = col;
    i32 outC = row;
    plane->Snap(&outR, &outC);
    CStatusBarMgr* sbi = (CStatusBarMgr*)*(char**)((char*)lvl + 0x2dc);
    if (sbi->m_hlBusy == 0) {
        if (*(i32*)sbi == 2) {
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

// ===========================================================================
// CTriggerMgr::ResetSpawnState  (0x79d90)
// ===========================================================================

// The +0x260 byte table is the real MFC CByteArray member m_byteArr (see
// <Gruntz/TriggerMgr.h>): RemoveAt (ResetSpawnState) + SetSize/SetAtGrow (Load) called
// directly. Plus the two build-state notifiers (0x100930 / 0x104d60).
extern void Eng_BuildNotifyA(i32 a); // 0x100930 (thunk 0x12fd)
extern void __cdecl operator delete(void*);

RVA(0x00079d90, 0xc5)
void CTriggerMgr::ResetSpawnState() {
    if (g_gameReg->m_134 != 1) {
        return;
    }
    if (m_284 == 0) {
        return;
    }
    CTmWorld* world = (CTmWorld*)g_gameReg->m_curState;
    CStatusBarMgr* st = world->m_2dc;
    if (st->m_retabNotify != 0) {
        operator delete(st->m_retabNotify);
        st->m_retabNotify = 0;
    }
    world->m_2dc->m_hlBusy = 0;
    if (m_byteArr.GetSize() > 0) {
        m_byteArr.RemoveAt(m_byteArr.GetSize() - 1, 1);
        CStatusBarMgr* ctx = world->m_2dc;
        if (*(i32*)ctx != 2 && ctx->m_activeTab == 5) {
            Eng_BuildNotifyA(0);
            world->m_2dc->TryActivate();
        }
    }
    if (g_gameReg->m_134 == 1) {
        CTmPendingFx* fx = m_pendingFx;
        if (fx != 0) {
            fx->Pulse();
        }
    }
    this->RefreshB(6);
}

// The world fx-spawner (0x7c620, thunk 0x152d): a __stdcall sprite spawner.
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
    if ((u32)tx >= (u32)grid->m_c || (u32)ty >= (u32)grid->m_10) {
        tile = 1;
    } else {
        tile = grid->m_8[ty][tx * 8 - tx];
    }
    if ((tile & 0x40939) == 0 && (tile & 2) == 0) {
        Eng_SpawnFx(0x14, (tx << 5) + 0x10, (ty << 5) + 0x10, 0, a3, 0);
        return 1;
    }
    CTmWorld* world = (CTmWorld*)g_gameReg->m_curState;
    i32 idx = a3 - 1;
    CTmWorld::Anchor* rec = ((u32)idx < 4) ? &world->m_anchors[idx] : 0;
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
    i32 idx = col * 15 + row; // grid[col][row] base
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
    pt.x = cell->m_lastTilePxX;
    pt.y = cell->m_lastTilePxY;
    CGruntzMapMgr* tg = g_gameReg->m_tileGrid;
    i32 rowIdx = pt.y >> 5;
    i32 colByte = (pt.x >> 5) * 28; // 7-dword cell stride (the grid HitTestCell walks)
    ((char*)tg->m_8[rowIdx])[colByte + 0x3] &= 0xdf;
    *(i32*)((char*)tg->m_8[rowIdx] + colByte + 0x4) = -1;
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
                CTmPendingFx* fx = m_pendingFx;
                if (fx != 0) {
                    fx->Pulse();
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

// 0x7a180: SpawnPuddle(x, y, f124, dir, color, f118) - create a "GruntPuddle" sprite from
// the level's factory; on failure ReportError and ret 0. On success run its Init hook,
// stash the placement fields (+0x124/+0x114/+0x118) and tail into PlacePuddle. (ret 0x18.)
RVA(0x0007a180, 0x86)
i32 CTriggerMgr::SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118) {
    CSpriteFactory* fac = m_level->m_8;
    CGameObject* sprite = fac->CreateSprite(0, x, y, 0xa, "GruntPuddle", 0x40003);
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
    sprite->m_7c->Init(sprite);
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
    CTmPuddleTarget* tgt = (CTmPuddleTarget*)sprite->m_7c->m_logic;
    i32 d = sprite->m_118;
    if (d == 0) {
        d = 0x19;
    }
    if (tgt->Place(sprite->m_124, sprite->m_114, color, d) == 0) {
        tgt->m_38->m_8 |= 0x10000;
        g_gameReg->ReportError(0x8009, 0x401); // dual-view bridge; see SpawnPuddle
        return 0;
    }
    CTmRecNode* n = (CTmRecNode*)m_baseList.GetHeadPosition();
    i32 manyFlag = (m_baseList.GetCount() > 0x3b) ? 1 : 0;
    i32 unlinked = 0;
    while (n != 0 && unlinked == 0) {
        CTmRecNode* cur = n;
        n = n->m_next;
        CTmPuddleTarget* o = cur->m_obj;
        if (o->m_54 == tgt->m_54 && o->m_58 == tgt->m_58) {
            if (o->m_5c != 0) {
                tgt->m_38->m_8 |= 0x10000;
                return 0;
            }
            o->m_38->m_8 |= 0x10000;
            m_baseList.RemoveAt((POSITION)cur);
            unlinked = 1;
        }
    }
    if (manyFlag != 0 && unlinked == 0) {
        n = (CTmRecNode*)m_baseList.GetHeadPosition();
        while (n != 0) {
            CTmRecNode* cur = n;
            n = n->m_next;
            CTmPuddleTarget* o = cur->m_obj;
            if (o->m_5c == 0) {
                o->m_38->m_8 |= 0x10000;
                m_baseList.RemoveAt((POSITION)cur);
            }
        }
    }
    m_baseList.AddTail(tgt);
    return 1;
}

// ===========================================================================
// LoadToyBoxIcon @0x07a3f0
// ===========================================================================
//
// Lazily creates the "GAME_TOYBOX" in-game icon at tile (x>>5, y>>5): first walks
// the factory's live-icon list (m_factoryHolder->m_8->m_liveObjects) and bails (return 0) if an
// existing icon of one of the two icon classes already sits on that tile;
// otherwise CreateSprite("InGameIcon"), cache its frame, stamp the four config
// slots and the +0x40 visible bit.  __thiscall, ret 0x14 (5 args).
//
// Byte-identical (99.94%): the only residual is the reloc-typing artifact on the
// two `cmp esi, OFFSET fn` de-dup comparisons (base names IconClassInitA/B vs
// retail thunk_FUN_004bf150 / CSingleAnimation; code bytes match).

RVA(0x0007a3f0, 0xd7)
i32 EngineLabelBacklog::LoadToyBoxIcon(i32 x, i32 y, i32 a3, i32 a4, i32 a5) {
    CSpriteFactory* fac = m_factoryHolder->m_8;
    i32 tx = x >> 5;
    i32 ty = y >> 5;

    CSpriteListNode* node = fac->m_liveObjects;
    while (node != 0) {
        CSpriteListNode* cur = node;
        node = node->next;
        CGameObject* obj = cur->m_sprite;
        void* init = (void*)obj->m_7c->Init;
        if (init == (void*)&IconClassInitA || init == (void*)&IconClassInitB) {
            i32 ox = obj->m_screenX >> 5;
            i32 oy = obj->m_screenY >> 5;
            if (tx == ox && ty == oy) {
                return 0;
            }
        }
    }

    CGameObject* spr = fac->CreateSprite(0, x, y, 0x17318, "InGameIcon", 0x40003);
    if (!spr) {
        g_gameReg->ReportError(0x8009, 0x402); // was the fake CGameRegistry::Report
        return 0;
    }
    spr->ApplyName("GAME_TOYBOX");
    spr->m_118 = a4;
    spr->m_130 = a5;
    spr->m_114 = a3;
    spr->m_stateFlags |= 1;
    return 1;
}

// 0x7a510: ClearRowAndRefresh(startRow) - Recall every live, hook-less cell of rows
// startRow..3 (5 = all); clear +0x400 when startRow is the magic group; then refresh
// the world, bump a stat, and re-arm the status item. ret 1.
// CRACKED (72->100): the "5 = all rows" decode as an explicit if/else (row=0/last=3 vs
// row=last=startRow) matches retail's jmp-merge (the default-then-override form reserves an
// extra frame slot + spills last); assign `last` before `row` in the else; declare `cell`
// (row*15) BEFORE `n` so the two lea chains interleave into edx. Cluster idiom for all the
// "startRow==5" range decoders (ClearGridRange/HitTestCell/CellHitTest).
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
        CTmCell** cell = &m_grid[row * 15];
        i32 n = last - row + 1;
        do {
            i32 i = 15;
            do {
                CTmCell* c = *cell;
                if (c != 0 && c->m_deathAnimStarted == 0) {
                    ((CGrunt*)c)->StartBombGruntRun(); // Recall==CGrunt::StartBombGruntRun @0x68520
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
    CPlay* world = (CPlay*)g_gameReg->m_curState;
    world->FlushPendingOps();
    world->ArmSnapshot(0, 0xbb7);
    ((CStatusBarMgr*)world->m_guts)->SetMode(1);
    return 1;
}

// 0x7a5e0: RebuildOverlay(obj, kind, ., .) - copy the source object's two pose getters into
// the manager's three 0x8-byte pose blocks (+0x290/+0x2b0/+0x2c0), selecting getter +0x30 for
// kind 4 and +0x2c for kind 7; ret 0 when obj null, 1 otherwise. Early-out when the kind-4/7
// self-probe is non-zero. (__thiscall: ret 0x10 => 4 args; Ghidra mis-derived the 2-arg
// `...PAXH@Z` prototype - only obj/kind are used, the trailing two slots are dead.)
// BYTE-EXACT after: (1) the 4-arg prototype, (2) the probe polarity (retail continues
// when Probe!=0, bails to return 0 when Probe==0), (3) virtual GetA/GetB dispatch (slots
// +0x2c/+0x30 -> a real 13-slot vtable view), and (4) the negated-outer-condition idiom
// (docs/patterns/negated-condition-far-block.md): writing each kind dispatch as
// `if (kind != 4) { if (kind == 7) {..7..} } else {..4..}` places the kind==4 body FAR,
// matching retail's block layout (the natural `if(kind==4)else if(kind==7)` lays it inline).
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
            if (this->Load((CSerialArchive*)obj) == 0) {
                return 0;
            }
        }
    } else {
        if (this->ScanGroup((CSerialArchive*)obj) == 0) {
            return 0;
        }
    }
    CTmOverlaySrc* src = (CTmOverlaySrc*)obj;
    char* blk0 = m_overlayDescA;
    if (kind != 4) {
        if (kind == 7) {
            src->GetA(blk0, 8);
            src->GetA(blk0 + 8, 8);
        }
    } else {
        src->GetB(blk0, 8);
        src->GetB(blk0 + 8, 8);
    }
    char* blk1 = m_overlayDescB;
    if (kind != 4) {
        if (kind == 7) {
            src->GetA(blk1, 8);
            src->GetA(blk1 + 8, 8);
        }
    } else {
        src->GetB(blk1, 8);
        src->GetB(blk1 + 8, 8);
    }
    char* blk2 = m_overlayDescC;
    if (kind != 4) {
        if (kind == 7) {
            src->GetA(blk2, 8);
            src->GetA(blk2 + 8, 8);
        }
    } else {
        src->GetB(blk2, 8);
        src->GetB(blk2 + 8, 8);
        return 1;
    }
    return 1;
}

// The serialization archive ScanGroup writes through is the shared CSerialArchive
// (<Gruntz/SerialArchive.h>, pulled via TriggerMgr.h): its Write @ vtable slot +0x30
// (real virtual dispatch). Plus the per-object id-write helper @0x5b8760.
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
    CTmLevel* lvl = m_level;
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
                id = g->m_10->m_188;
                Ar_WriteId(lvl->m_8, id, ar);
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
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
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
        CTmNode* m = (CTmNode*)list->GetHeadPosition();
        while (m != 0) {
            CTmNode* cur = m;
            m = m->m_next;
            ar->Write(cur->m_payload, 8);
        }
        list++;
        k--;
    } while (k != 0);
    void* goal = m_goal;
    i32 goalId = 0;
    if (goal != 0) {
        goalId = *(i32*)((char*)goal + 0x188);
    }
    ar->Write(&goalId, 4);
    void* ov = m_pendingFx;
    i32 ovId = 0;
    if (ov != 0 && *(void**)((char*)ov + 0x10) != 0) {
        ovId = *(i32*)(*(char**)((char*)ov + 0x10) + 0x188);
    }
    ar->Write(&ovId, 4);
    ar->Write(m_274, 0x10);
    i32 cntC = m_baseList.GetCount();
    ar->Write(&cntC, 4);
    CTmNode* rn = (CTmNode*)m_baseList.GetHeadPosition();
    while (rn != 0) {
        CTmNode* cur = rn;
        rn = rn->m_next;
        void* obj = cur->m_payload;
        if (obj == 0) {
            return 0;
        }
        i32 oid = *(i32*)(*(char**)((char*)obj + 0x10) + 0x188);
        Ar_WriteId(lvl->m_8, oid, ar);
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
    ar->Write(&m_230, 4);
    ar->Write(&m_284, 4);
    ar->Write(&m_288, 4);
    ar->Write(&m_recX, 8);
    ar->Write(&m_2a4, 4);
    ar->Write(&m_3ec, 4);
    ar->Write(&m_groupFlag, 4);
    ar->Write(&g_curPlayer, 4);
    ar->Write(&g_644ca4, 4);
    ar->Write(&m_pendingFxKind, 4);
    ar->Write(&m_selSentinel, 4);
    return 1;
}

// ---------------------------------------------------------------------------
// Load (0x7abc0) collaborators. The manager reads its state through the archive
// reader `ar` (vtable slot 0x2c = Read(dst, size)); the map values it resolves
// carry a type-id virtual (slot 8 = vtbl+0x20) and a +0x7c sub-object whose +0x18
// is the real placed game-object.
// ---------------------------------------------------------------------------
// The archive reader `ar` is the shared CSerialArchive (Read @ vtable slot 11 =
// +0x2c); see <Gruntz/SerialArchive.h> (pulled via TriggerMgr.h).
// The map-resolved placed object is the canonical CGameObject (<Gruntz/UserLogic.h>): its
// slot-8 virtual GetTypeId (+0x20) yields the serialize type-id, its +0x7c CGameObjAux holds
// the bound logic (aux->m_logic @+0x18). (Former CTmSerMapObj/CTmSerMapObjVtbl PMF-vtable +
// CTmSerAux views folded onto the real class + real virtual.)
// The serialize key->object map is the CSpriteFactory's embedded m_objMap (@factory+0x48,
// see <Gruntz/SpriteFactory.h>); reached through the typed member, no this+offset cast.
// The manager's embedded list nodes (base list @this+0, record @+0x240, the ten
// selection lists @+0x2d0) are the real MFC CPtrList members; the +0x260 byte array is the
// real MFC CByteArray member; the +0x25c overlay sub-object reuses CTmOverlay (all above).
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
    if (m_level == 0) {
        return 0;
    }
    m_soundChanA = 0;
    m_soundChanB = 0;
    m_3f8 = 0;
    m_3fc = 0;

    // The factory's embedded serialize map is the real MFC CMapPtrToPtr at +0x48
    // (Lookup @0x1b8760); documented embedded-member offset (see SpriteFactory.h).
    CMapPtrToPtr* map = (CMapPtrToPtr*)((char*)m_level->m_8 + 0x48);

    // the 4x15 placed-object grid (this[7..66], byte offsets +0x1c..+0x108)
    for (i32 base = 7; base < 0x43; base += 0xf) {
        for (i32 i = 0; i < 0xf; i++) {
            i32 key;
            ar->Read(&key, 4);
            void* cell = 0;
            if (key != 0) {
                void* found = 0;
                void* looked = map->Lookup((void*)key, found) ? found : 0;
                if (looked == 0) {
                    return 0;
                }
                cell = ((CGameObject*)looked)->m_7c->m_logic;
                if (cell == 0) {
                    return 0;
                }
            }
            ((void**)this)[base + i] = cell;
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
    for (ci = 0; ci < (u32)count; ci++) {
        i32 b;
        ar->Read(&b, 1);
        arr->SetAtGrow(ci, b);
    }
    ClearRecords();

    // the +0x240 record list (nodes pulled off the shared free-list)
    ar->Read(&count, 4);
    CPtrList* rec = &m_recList;
    for (ci = 0; ci < (u32)count; ci++) {
        char* fl = (char*)g_coordPool.m_freeHead;
        void* node = 0;
        if (*(void**)fl != 0) {
            node = fl + 4;
            g_coordPool.m_freeHead = *(void**)fl;
        }
        ar->Read(node, 8);
        rec->AddTail(node);
    }

    // the ten selection lists (+0x2d0, stride 0x1c)
    CPtrList* sel = m_selLists;
    i32 slot = 0xa;
    do {
        ar->Read(&count, 4);
        for (ci = 0; ci < (u32)count; ci++) {
            char* fl = (char*)g_coordPool.m_freeHead;
            void* node = 0;
            if (*(void**)fl != 0) {
                node = fl + 4;
                g_coordPool.m_freeHead = *(void**)fl;
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
            void* looked = map->Lookup((void*)key, found) ? found : 0;
            void* obj = (looked != 0 && ((CGameObject*)looked)->GetTypeId() == 5) ? looked : 0;
            m_goal = (CTmGoal*)obj; // Eh's serialize-view reinterpret of the goal slot
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
            void* looked = map->Lookup((void*)key, found) ? found : 0;
            if (looked == 0) {
                return 0;
            }
            void* obj = ((CGameObject*)looked)->m_7c->m_logic;
            m_pendingFx = (CTmPendingFx*)obj; // Eh's serialize-view reinterpret
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
    for (ci = 0; ci < (u32)count; ci++) {
        i32 key;
        ar->Read(&key, 4);
        if (key == 0) {
            return 0;
        }
        void* found = 0;
        void* looked = map->Lookup((void*)key, found) ? found : 0;
        if (looked == 0) {
            return 0;
        }
        void* obj = ((CGameObject*)looked)->m_7c->m_logic;
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
    ar->Read(&m_230, 4);
    ar->Read(&m_284, 4);
    ar->Read(&m_288, 4);
    ar->Read(&m_recX, 8);
    ar->Read(&m_2a4, 4);
    ar->Read(&m_3ec, 4);
    ar->Read(&m_groupFlag, 4);
    ar->Read(&g_curPlayer, 4);
    ar->Read(&g_renderCtx, 4);
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
        i32* rec = ((CTmNode*)m_recList.GetHeadPosition())->m_payload;
        cell = m_grid[rec[1] + rec[0] * 15];
    }
    CPlay* world = (CPlay*)g_gameReg->m_curState;
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
            CGruntHud* o = cell->m_10;
            g_gameReg->m_cmdGrid->Spawn(o->m_5c, o->m_60, 0, 0, 0, 3, 1);
        }
    } else if (kind != 0) {
        i32 v = kind + kPendingFxIdBase;
        m_pendingFxKind = v;
        world->LoadCursorSprites(v, 0); // ILT 0x35da @0x7b2f4 (was the StopFx2 phantom)
    }
    this->Refresh2();
    this->Record2(x, y);
    return 1;
}

// ===========================================================================
// LoadExplosionSprites @0x07b330
// ===========================================================================
//
// Creates a random "GAME_EXPLOSION%d" explosion sprite via the "Explosion" class
// template, resolves its geometry, and records the chosen variant index at
// this+0x124 with the loaded flag at this+0x114. __thiscall ret 0x10 (4 args).

RVA(0x0007b330, 0xc6)
i32 EngineLabelBacklog::LoadExplosionSprites(i32 geoB, i32 geoA, i32 variant, i32 dummy) {
    CSpriteFactory* fac = m_factoryHolder->m_8;
    CGameObject* spr = fac->CreateSprite(0, geoB, geoA, 0, "Explosion", 0x40003);
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

// ===========================================================================
// CRockBreakMgr::BuildRockBreakParticles (0x7b440) - merged from
// RockBreakParticles.cpp per dossier 10b (rock-break FX spawned by triggers;
// embedded singleton, this TU by retail position). @identity-TODO: the
// CRockBreakMgr/Rock* identities are placeholders; the Rock* views below are
// the donor's (RockMgr is the 0x64556c singleton under its extern-"C" csv name
// g_gameReg - the name every unit's reloc matches).
// ===========================================================================
// FUN_001b2cf5 __cdecl: format into a CString (the LoadBootyCheatState FormatStr).
void FormatStr(CString* out, const char* fmt, ...);

// The eye-candy sprite the factory returns is the shared CGameObject (ApplyName
// @0x150540 / ApplyLookupGeometry @0x1505b0); the factory is the canonical
// CSpriteFactory (m_22c->m_8; <Gruntz/SpriteFactory.h>).

// Every object the rock-break driver walks is a REAL class already in the tree; the
// eleven Rock* views were a second model of them (offsets identical field-for-field):
//   RockSndPlayer    -> ::CSoundCueMgr   (Play @0x1360d0 IS its ConfigureItem)
//   RockSndEntry     -> ::LeafCue        (m_10 player, m_14 last clock, m_18 interval)
//   RockSndSet       -> ::CSndHost       (m_10 CMapStringToOb name map, m_30 emit gate)
//   RockGrid         -> ::BrickzGridDesc (m_20 flat id table, m_24 row-base table,
//                                         m_28/m_2c storage dims, m_30/m_34 active dims)
//   RockCellObj      -> ::BrickzButeObj  (8 filler slots then GetTypeCode @+0x20)
//   RockBoard        -> ::BrickzAttrMgr  (m_4c bute-object table, m_5c the grid desc)
//   RockMapHost      -> ::CSpriteFactoryHolder (m_8 sprite factory, m_28 the CSndHost)
//   RockSettingsRoot -> ::CState         (g_gameReg->m_curState)
//   RockMgr          -> ::CGameRegistry  (g_gameReg itself)
//   RockLogicObj / RockLogicMgr -> nothing: both were EMPTY comment-only shells whose
//                                  calls already bridge-cast to the real classes.
//
// @identity-TODO, left as a cast rather than invented: the +0x24 slot of the world
// holder is claimed by TWO incompatible shapes - CGameRegistry.h's CGameViewport
// (viewport RECT @+0x10, camera geom @+0x5c) and the terrain BrickzAttrMgr this driver
// walks (bute table @+0x4c, grid desc @+0x5c). One of the two is wrong; settling it
// needs the world-holder ctor, so the rock-break sites reach the terrain manager
// through an explicit cast and CSpriteFactoryHolder::m_24 is left untouched.

DATA(0x0021ab20)
extern i32 g_sndEnabled; // ?g_sndEnabled@@3HA
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // _g_killCueClock (wrap-safe draw clock)

// (CRockBreakMgr is the canonical <Gruntz/RockBreakMgr.h> class - was a .cpp-local view here.)

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
i32 CRockBreakMgr::BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 a4) {
    Prepare(cx, cy, r, 6, a4);

    // The concrete game-state behind CGameRegistry::m_curState (the base CState is
    // proven <= 0x1c0, so a +0x2e4 read is a DERIVED state's field). Its shape here -
    // pad to +0x2e4, then the tile-trigger/cell container - is the SAME object
    // CBattlezMapConfig::LoadConfig reads as lvl->m_spawnInfo (also a +0x2c slot of its
    // owner, also +0x2e4 -> the container): <Gruntz/LevelInfo.h>'s CLevelSpawnInfo.
    // The downcast is the honest symptom of m_curState being typed as the base.
    CLevelSpawnInfo* root = (CLevelSpawnInfo*)g_gameReg->m_curState;
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
            BrickzAttrMgr* board = (BrickzAttrMgr*)m_22c->m_24;
            BrickzGridDesc* grid = board->m_5c;
            if (tx >= grid->m_30 || ty >= grid->m_34) {
                continue;
            }
            i32 col;
            if (pxX < 0x10) {
                col = 0;
            } else if (tx >= grid->m_28) {
                col = grid->m_28 - 1;
            } else {
                col = tx;
            }
            i32 row = (ty >= grid->m_2c) ? grid->m_2c - 1 : ty;
            i32 cell = grid->m_20[grid->m_24[row] + col];
            i32 type;
            if (cell == (i32)0xeeeeeeee || cell == -1) {
                type = 0;
            } else {
                BrickzButeObj* o = board->m_4c[cell & 0xffff];
                type = o->GetTypeCode(0, 0);
            }

            if (type != 0x1e && type != 0x1f) {
                if (type == 0x21) {
                    CTileTriggerSwitchLogic* gr =
                        (CTileTriggerSwitchLogic*)((CTileTriggerSwitchLogic*)root->m_2e4)
                            ->ScanNeighborhood(tx, ty);
                    if (gr == 0) {
                        CString msg;
                        FormatStr(&msg, "No giant rock logic found around: x=%d, y=%d", cx, cy);
                        g_gameReg->EnterModalUI(msg);
                        g_gameReg->ReportError(0x80dd, 0x403);
                        return 0;
                    }
                    gr->BuildRockBreakInGameText();
                    ((CTileTriggerContainer*)root->m_2e4)->DelFromList1((void*)gr);
                    continue;
                }
                if (type != 0x97 && type != 0x98 && type != 0x99) {
                    continue;
                }
                CTileTriggerSwitchLogic* o =
                    (CTileTriggerSwitchLogic*)((CTileTriggerSwitchLogic*)root->m_2e4)
                        ->FindByField0C(ty + (tx << 8));
                if (((CTileActionEvent*)o)->Process(0)) {
                    ((CTileTriggerContainer*)root->m_2e4)->DelFromList3((void*)o);
                }
                continue;
            }

            // type == 0x1e || type == 0x1f: rock-break marker + particle
            CTileTriggerSwitchLogic* lo =
                (CTileTriggerSwitchLogic*)((CTileTriggerContainer*)root->m_2e4)
                    ->FindInLists12(ty + (tx << 8), 0x1a);
            if (lo != 0) {
                ((CTileTriggerLogic*)lo)->ApplyMove(type);
                ((CTileTriggerContainer*)root->m_2e4)->DelFromList1((void*)lo);
            } else {
                BrickzGridDesc* wg = (BrickzGridDesc*)g_gameReg->m_world->m_24->m_mainPlane;
                i32 off = wg->m_24[ty];
                if (type == 0x1e) {
                    wg->m_20[off + tx] = 0x5a;
                    ((CBrickzGrid*)g_gameReg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5a);
                } else {
                    wg->m_20[off + tx] = 0x5b;
                    ((CBrickzGrid*)g_gameReg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5b);
                }
            }

            POINT pt;
            pt.x = pxX;
            pt.y = pxY;
            if (!PtInRect((const RECT*)&g_gameReg->m_viewOriginL, pt)) {
                continue;
            }
            CGameObject* spr = m_22c->m_8->CreateSprite(0, pxX, pxY, 0xcf84f, "Particlez", 0x40003);
            if (spr == 0) {
                continue;
            }
            spr->ApplyName("LEVEL_ROCKBREAK");
            spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);

            CSndHost* set = (CSndHost*)m_22c->m_28;
            if (set->m_emitGate == 0) {
                // CSndHost's name map is an MFC CMapStringToPtr (RTTI-proven), so its
                // Lookup out-param is a void*& - the payload is the cue itself.
                void* e_ob = 0;
                set->m_10.Lookup("LEVEL_ROCKBREAK", e_ob);
                LeafCue* e = (LeafCue*)e_ob;
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
SIZE_UNKNOWN(CRockBreakMgr);
SIZE_UNKNOWN(CMapStringToOb);

// ===========================================================================
// CGruntTileMgr::CombatCue (0x7b930) - merged from GruntTileMgr.cpp per
// dossier 10b (the grunt tile-mgr spell-area cue; embedded singleton, this TU
// by retail position). CGruntTileMgr + g_gameReg come from
// <Gruntz/Grunt.h>; the LightFx/flash key statics are the donor's.
// ===========================================================================
// The LightFx / flash key strings (reloc-masked .rodata literals).
static const char s_LightFx[] = "LightFx";
static const char s_GAME_FLASH[] = "GAME_FLASH";
static const char s_GAME_LIGHTING_FLASH[] = "GAME_LIGHTING_FLASH";
// The combat-timeout bute keys (GetIntDef takes char*).
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
i32 CGruntTileMgr::CombatCue(i32 x, i32 y, i32 radius, i32 tier, i32 flag) {
    i32 r = radius << 5;
    i32 xLo = x - r - 7;
    i32 yLo = y - r - 7;
    i32 xHi = x + r + 7;
    i32 yHi = y + r + 7;
    i32 rangeA = m_22c->m_24->m_5c->m_28 - 2;
    i32 rangeB = m_22c->m_24->m_5c->m_2c - 2;

    CGrunt** p = &m_grid[0][0];
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
            i32 gx = g->m_10->m_5c;
            i32 gy = g->m_10->m_60;
            i32 lx = gx - 7;
            i32 ly = gy - 7;
            i32 hx = lx + 14;
            i32 hy = ly + 14;
            if (xLo <= hx && xHi >= lx && yLo <= hy && yHi >= ly) {
                switch (tier) {
                    case 1:
                        if (g->m_gruntKind != 0x38) {
                            ApplyCellEffect(i, j, 0, flag);
                        }
                        break;
                    case 6:
                        if (g->m_gruntKind != 0x38) {
                            ApplyCellEffect(i, j, 0xb, flag);
                        }
                        break;
                    case 7:
                        if (g->m_gruntKind != 0x38) {
                            ApplyCellEffect(i, j, 2, flag);
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
                                    g_gameReg->m_world->m_8
                                        ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                                done = 1;
                                spr->m_7c->Init(spr);
                                ((CLightFx*)spr->m_7c->m_logic)
                                    ->Activate((i32)s_GAME_LIGHTING_FLASH, (i32)s_GAME_FLASH, 3, 1);
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
                        g->m_combatClockLo = g_645588;
                        g->m_combatClockHi = 0;
                        CGameObject* spr =
                            g_gameReg->m_world->m_8
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->Init(spr);
                        ((CLightFx*)spr->m_7c->m_logic)
                            ->Activate((i32)s_GAME_LIGHTING_FLASH, (i32)s_GAME_FLASH, 2, 1);
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
                            g_gameReg->m_world->m_8
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->Init(spr);
                        ((CLightFx*)spr->m_7c->m_logic)
                            ->Activate((i32)s_GAME_LIGHTING_FLASH, (i32)s_GAME_FLASH, 7, 1);
                        break;
                    }
                    case 4: { // freeze
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->FreezeApply();
                        CGruntHud* h = g->m_10;
                        CGameObject* spr =
                            g_gameReg->m_world->m_8
                                ->CreateSprite(0, h->m_5c, h->m_60, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->Init(spr);
                        ((CLightFx*)spr->m_7c->m_logic)
                            ->Activate((i32)s_GAME_LIGHTING_FLASH, (i32)s_GAME_FLASH, 9, 1);
                        break;
                    }
                }
            }
        }
    }
    return 1;
}

// 0x7be10: StopPendingFx - when our overlay-fx (+0x2a8) is live, or the world has a
// pending flag (world+0x504), stop the world's fx and clear +0x2a8.
RVA(0x0007be10, 0x34)
void CTriggerMgr::StopPendingFx() {
    CPlay* world = (CPlay*)g_gameReg->m_curState;
    if (m_pendingFxKind == 0 && world->m_dragEndNotify == 0) { // m_504 == CPlay::m_dragEndNotify
        return;
    }
    world->LoadCursorSprites(0, 0); // CPlay::LoadCursorSprites @0xd0120
    m_pendingFxKind = 0;
}

// ===========================================================================
// CGruntResurrector::LoadGruntResurrectTuning (0x7be60) - merged from
// GruntResurrectRadius.cpp per dossier 10b (the resurrect-radius pass, sibling
// of the rock-break scan; called by CGrunt::LoadGruntAbilityTuning @0x572db).
// @identity-TODO: CGruntResurrector/Res* identities are placeholders (the
// owning `this` is the grunt-list manager; g_gameReg/g_resButeMgr are the
// donor's names for the 0x64556c / 0x2453d8 singletons).
// ===========================================================================
SIZE_UNKNOWN(ResGruntLogic);
struct ResGruntLogic { // grunt->m_38
    char m_pad00[0x8];
    u32 m_8; // +0x08  flags (|= 0x10000 on resurrect)
};
struct ResHost; // grunt->m_6c (opaque; passed through to Resurrect)
SIZE_UNKNOWN(ResGrunt);
struct ResGrunt {
    char m_pad00[0x38];
    ResGruntLogic* m_38; // +0x38
    char m_pad3c[0x54 - 0x3c];
    i32 m_54; // +0x54  x tile
    i32 m_58; // +0x58  y tile
    i32 m_5c; // +0x5c  busy/skip gate
    char m_pad60[0x68 - 0x60];
    i32 m_68;      // +0x68  grunt type index
    ResHost* m_6c; // +0x6c
};
SIZE_UNKNOWN(ResNode);
struct ResNode {
    ResNode* m_next; // +0x00
    char m_pad04[0x4];
    ResGrunt* m_grunt; // +0x08
};
SIZE_UNKNOWN(ResMgrCfgEntry);
struct ResMgrCfgEntry { // g_gameReg + 0x150 + type*0x238
    char m_pad00[0x14];
    i32 m_14; // +0x14
    char m_pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
    char m_pad28[0x2c - 0x28];
    i32 m_2c; // +0x2c
    char m_pad30[0x38 - 0x30];
    CBattlezMapConfig m_38; // +0x38
    char m_pad3c[0x238 - 0x3c];
};
// The factory (m_world->m_8) is the canonical CSpriteFactory (<Gruntz/SpriteFactory.h>);
// the created "LightFx" eye-candy sprite is the shared CGameObject whose +0x7c
// CGameObjAux carries the Init driver (+0x10) and the per-class setup slot m_18
// (+0x18) - here the LightFx flash config below, downcast at the site.
SIZE_UNKNOWN(ResFactoryHost);
struct ResFactoryHost {
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08
};
SIZE_UNKNOWN(ResSettings);
struct ResSettings {
    char m_pad00[0x30];
    ResFactoryHost* m_world; // +0x30
    char m_pad34[0x134 - 0x34];
    i32 m_134; // +0x134  resurrect mode
    char m_pad138[0x150 - 0x138];
    ResMgrCfgEntry m_150[1]; // +0x150  per-type config (stride 0x238)
};
// The resource-config manager @0x2453d8 IS the canonical CButeMgr singleton g_buteMgr
// (?g_buteMgr@@3VCButeMgr@@A, DATA-bound tree-wide; the former ResButeMgr {} view + its
// (CButeMgr*) cross-casts were a fake facet - dissolved onto the real class).
extern CButeMgr g_buteMgr;
SIZE_UNKNOWN(CGruntResurrector);
struct CGruntResurrector {
    char m_pad00[0x4];
    ResNode* m_4; // +0x04  grunt list head
    // FUN_000040bb __thiscall: spawn/resurrect one grunt (ret != -1 on success).
    i32 Resurrect(
        i32 type,
        i32 px,
        i32 py,
        i32 a3,
        i32 a4,
        ResHost* host,
        i32 a6,
        i32 a7,
        i32 aiType,
        i32 radius,
        i32 a10,
        i32 a11,
        i32 a12
    );
    void Notify(ResNode* node); // FUN_001b4ac7 __thiscall
    i32 LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r);
};

// @early-stop
// regalloc/frame-layout wall (~65%): instruction selection, calls, constants,
// strings + the rect/loop/spawn structure are byte-faithful, but retail
// frame-allocates the `node` loop variable (a dedicated 4-byte slot at [esp+0x14]
// inside a 0x18 frame) while this /O2 recompile reuses an incoming-arg slot, yielding
// a 0x14 frame and a +4 cascade across every [esp+N] operand. Not source-steerable
// (the slot-vs-frame choice is the allocator's). Logic complete. See
// docs/patterns/zero-register-pinning.md + const-materialize-into-reg-vs-immediate.md.
RVA(0x0007be60, 0x21e)
i32 CGruntResurrector::LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r) {
    RECT rect;
    i32 hx = cx >> 5;
    i32 hy = cy >> 5;
    rect.left = hx - r;
    rect.right = hx + r;
    rect.top = hy - r;
    rect.bottom = hy + r;

    for (ResNode* node = m_4; node != 0; node = node->m_next) {
        ResGrunt* g = node->m_grunt;
        if (g->m_5c != 0) {
            continue;
        }
        POINT pt;
        pt.x = g->m_54;
        pt.y = g->m_58;
        if (!PtInRect(&rect, pt)) {
            continue;
        }

        i32 type = g->m_68;
        ResSettings* s = (ResSettings*)g_gameReg;
        ResMgrCfgEntry* cfg = &s->m_150[type];
        i32 aiType = 0;
        i32 ok = 0;
        i32 px = (g->m_54 << 5) + 0x10;
        i32 py = (g->m_58 << 5) + 0x10;

        if (s->m_134 == 1) {
            i32 radius = 0;
            if (cfg->m_14 == 0) {
                aiType = g_buteMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_buteMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (Resurrect(type, px, py, 0x186a0, 3, g->m_6c, 0, 0, aiType, radius, 0, 0, 0) != -1) {
                ok = 1;
            }
        } else if (cfg->m_20 != 0 && cfg->m_2c == 0 && cfg->m_24 == 0) {
            if (cfg->m_14 != 0) {
                if (Resurrect(type, px, py, 0x186a0, 3, g->m_6c, 0, 0, 0, 0, 0, 0, 0) != -1) {
                    ok = 1;
                }
            } else if (cfg->m_38.Method_030990(g->m_54, g->m_58) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->m_38->m_8 |= 0x10000;
            Notify(node);
            CGameObject* spr =
                g_gameReg->m_world->m_8->CreateSprite(0, px, py, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->Init(spr);
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 8, 1);
        }
    }
    return 1;
}

// The sprite's bound logic (sprite desc +0x18) is the canonical CUserLogic
// (<Gruntz/UserLogic.h>): its multi-arg Place driver (@0x4c1c4) and target-cursor Arm
// (@0x4e517) are reached through the base pointer (former CTmUserLogic view folded away).

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
    CTmCell* src = m_grid[col * 15 + a1c];
    i32 free = 0;
    CTmCell** rowBase = &m_grid[row * 15];
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
    CGruntHud* o = src->m_10;
    i32 sx = (o->m_5c & ~0x1f) + 0x10;
    i32 sy = (o->m_60 & ~0x1f) + 0x10;
    i32 k = src->m_entranceReason;
    if (k > 0x16) {
        k = src->m_19c;
    }
    i32 vis = src->m_198;
    this->Reset3(col, k, vis); // prep self-call 0x7ec96
    CSpriteFactory* fac = m_level->m_8;
    CGameObject* sprite = fac->CreateSprite(0, sx, sy, 0x186a0, "Grunt", 0x40003);
    if (sprite == 0) {
        return 0;
    }
    sprite->m_7c->Init(sprite);
    // SETTLED FROM THE BINARY (the contradiction the deleted CTmCell view was hiding). Retail:
    //   mov ecx,[edi+0x7c] ; push edi ; call [ecx+0x10]   <- aux->Init(sprite)
    //   mov edx,[edi+0x7c] ; mov edi,[edx+0x18]           <- edi := aux->m_logic  (REASSIGNED)
    //   ... mov ecx,edi ; call <Place>
    //   mov DWORD PTR [esi+ebp*4+0x1c],edi                <- m_grid[..] := THE LOGIC
    // The grid holds the LOGIC (this grunt), not the CreateSprite result - the earlier
    // reconstruction stored the sprite, which is why the cell offsets never lined up with the
    // sprite's. The downcast to the concrete leaf is the authentic one every creator does.
    CGrunt* logic = (CGrunt*)sprite->m_7c->m_logic;
    if (logic->Place(col, row, vis, k, 0, 0, 0, 0, 0, 0, 0, 0) == 0) {
        logic->m_154->m_8 |= 0x10000;
        return 0;
    }
    m_grid[row * 15 + free] = logic;
    m_rowCount[row] += 1;
    m_cellFlag[(row * 15 + free)] = 0;
    return 1;
}

// 0x7c2e0: CycleMoveIcons(skipRow, enable) - for grid rows 0..3 except `skipRow`, either
// roll a random move-icon onto each live cell (stashing the prior +0x1f8 when -1) and tick
// the world's region-4, or restore each cell's stashed icon. ret 1.
// @early-stop
// scheduling residual (~93%): logic + offsets + externs byte-exact; retail hoists the -1
// sentinel into ebp and schedules the rand()/idiv differently. topic:wall.
RVA(0x0007c2e0, 0xb5)
i32 CTriggerMgr::CycleMoveIcons(i32 skipRow, i32 enable) {
    CTmCell** grid = m_grid;
    for (i32 r = 0; r < 4; r++) {
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
                        ((CGrunt*)g)->SelectMoveIcon(t); // -> ?SelectMoveIcon@CGrunt@@ (0x57800)
                        ((CPlay*)g_gameReg->m_curState)->OnRegion4(1);
                    } else if (g->m_1f8 != -1) {
                        ((CGrunt*)g)->SelectMoveIcon(g->m_1f8); // -> CGrunt (0x57800)
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

// ===========================================================================
// CFinishLevelState::LoadFinishLevelSprite (0x7c3d0) - merged from
// FinishLevelSprite.cpp per dossier 10b (the finish-level overlay state
// driver; embedded singleton, this TU by retail position). @identity-TODO:
// CFinishLevelState is a placeholder identity. The 6-way state transition:
// entering state 1 looks up the GAME\FINISHLEVEL sprite, latches its
// completion timer and (when the live surface is free) plays its sound cue;
// the other states reseat the timer or run the grunt death animation.
// g_sndEnabled/g_sndCueTag/g_killCueClock are declared in the rock-break
// section above; g_645588 in <Gruntz/TriggerMgrViews.h>.
// ===========================================================================
// CSoundCueMgr - ConfigureItem pushes a cue; +0x28 carries the cue duration (both
// modeled in <Gruntz/SoundCueMgr.h>). The +0x28 cue holder (name->cue map @+0x10,
// emit gate @+0x30) is the canonical CSndHost, its looked-up cue the LeafCue -
// both in <Gruntz/SoundCue.h> (the former CueObj / CStatusBarHolder folded onto them).

SIZE_UNKNOWN(FinishLevelMgr);
struct FinishLevelMgr {
    char m_pad00[0x28];
    CSndHost* m_28; // +0x28  the +0x28 cue/status holder (canonical CSndHost)
};

SIZE_UNKNOWN(CFinishLevelState);
class CFinishLevelState {
public:
    void LoadFinishLevelSprite(i32 state);
    char m_pad000[0x22c];
    FinishLevelMgr* m_22c; // +0x22c
    char m_pad230[0x288 - 0x230];
    i32 m_288; // +0x288 phase
    char m_pad28c[0x290 - 0x28c];
    i32 m_290;     // +0x290 timer base
    i32 m_294;     // +0x294
    i32 m_298;     // +0x298 timer duration
    i32 m_29c;     // +0x29c
    CGrunt* m_2a0; // +0x2a0
    char m_pad2a4[0x3ec - 0x2a4];
    i32 m_3ec; // +0x3ec last state
    char m_pad3f0[0x400 - 0x3f0];
    i32 m_400; // +0x400
};

// @early-stop
// Dense 6-case switch (~61%). Two stacked residuals, both confirmed by llvm-objdump
// -dr base vs target: (1) the jump-table artifact - MSVC emits the table as a
// separate $L COMDAT (jmp reloc -> $L19166), the delinker inlines it at fn+0x1b0
// (jmp reloc -> fn) - documented ~79% ceiling, docs/patterns/
// switch-jumptable-separate-comdat.md. (2) case-1 /O2 scheduling below that ceiling:
// caching CSndHost* h28 = m_22c->m_28 recovered the m_28 share between the
// m_30 check and the 2nd Lookup (58->61), but MSVC still (a) re-materializes edi=0
// with a redundant `xor edi,edi` (the header zero doesn't propagate through the
// indirect switch jmp in this build) and (b) HOISTS the m_22c reload for h28 up into
// the m_298 computation, which reorders the independent m_298/m_29c stores. Cases
// 2-6 are byte-identical to retail; only case-1 instruction scheduling diverges,
// not source-steerable.
RVA(0x0007c3d0, 0x1ae)
void CFinishLevelState::LoadFinishLevelSprite(i32 state) {
    switch (state) {
        case 1:
            if (m_288 != 2) {
                void* p_ob = 0;
                m_22c->m_28->m_10.Lookup("GAME\\FINISHLEVEL", p_ob);
                LeafCue* p = (LeafCue*)p_ob;
                m_298 = p->m_10->m_28 + 500;
                m_29c = 0;
                m_290 = g_645588;
                m_294 = 0;
                CSndHost* h28 = m_22c->m_28;
                if (h28->m_emitGate == 0) {
                    p = 0;
                    h28->m_10.Lookup("GAME\\FINISHLEVEL", (void*&)p);
                    if (p != 0 && g_sndEnabled != 0
                        && (u32)(g_killCueClock - p->m_14) >= (u32)p->m_18) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                    }
                }
                m_288 = 1;
                m_400 = 0;
                m_3ec = state;
                return;
            }
            goto Lab_56b;
        case 2:
            m_288 = 1;
            break;
        case 3:
            if (m_288 == 0) {
                m_288 = 2;
                if (m_2a0 != 0) {
                    m_2a0->ResolveDeathAnimation();
                }
            }
            goto Lab_522;
        case 4:
            m_288 = 2;
            m_298 = 3000;
            m_29c = 0;
            m_290 = g_645588;
            goto Lab_565;
        case 5:
            m_288 = 2;
            break;
        case 6:
            m_288 = 2;
        Lab_522:
            m_298 = 3000;
            m_29c = 0;
            m_290 = g_645588;
            goto Lab_565;
        default:
            return;
    }
    m_298 = 3000;
    m_29c = 0;
    m_290 = g_645588;
Lab_565:
    m_294 = 0;
Lab_56b:
    m_400 = 0;
    m_3ec = state;
}

// ===========================================================================
// LoadPowerupIconSprites @0x07c620
// ===========================================================================
//
// The big in-game-icon loader: a switch on the powerup/toy/tool type id selects
// the sprite-set key, then the common path CreateSprite("InGameIcon", ...) +
// CacheFirstFrame + zeroes the +0x114-block. Two special cases: WARPSTONE (a
// per-level key resolved through g_buteMgr) and the closing TIMEBOMB type that
// runs its own CreateSprite("TimeBomb", ...) + a CoveredTimeBombTime default.
// __thiscall-free ret 0x18 (6 args). Returns 1 on success.

// The in-game-icon type id LoadPowerupIconSprites dispatches on (type) is PickupType,
// the shared object/pickup/grunt-kind id space in <Gruntz/PickupType.h>. Each name is
// confirmed by its case's GAME_INGAMEICONZ_* sprite key + verified against the retail
// jump table (VA 0x47c9e8/0x47cab4); the ids AGREE with CGrunt's LoadPickupSprites (both
// traced to the retail switch). Same immediates as the bare labels -> matching-neutral.
// (The warp-letter cases are PICKUP_W/A/R/P; the icon-only covered timebomb is
// PICKUP_COVEREDTIMEBOMB = 99.)

RVA(0x0007c620, 0x3c5)
i32 EngineLabelBacklog::LoadPowerupIconSprites(
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
                CResourceTracker* rt = (CResourceTracker*)g_gameReg->m_curState;
                CString lvl;
                lvl.Format("Level%i", rt->m_levelNumber);
                name.Format(
                    "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i",
                    g_buteMgr.GetInt("WarpStone", (const char*)lvl)
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
            CGameObject* tb =
                g_gameReg->m_world->m_8->CreateSprite(0, geoB, geoA, 0xf, "TimeBomb", 0x40003);
            if (tb) {
                tb->m_120 = g_buteMgr.GetDwordDef("Powerupz", "CoveredTimeBombTime", 0x7d0);
            }
            return tb != 0;
        }
        default:
            return 0;
    }

    CGameObject* spr =
        g_gameReg->m_world->m_8->CreateSprite(0, geoB, geoA, 0x17318, "InGameIcon", 0x40003);
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
    CTmNode* n = (CTmNode*)m_selLists[idx].GetHeadPosition();
    if (n != 0) {
        void* head = g_coordPool.m_freeHead;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* payload = cur->m_payload;
            if (payload != 0) {
                void** slot = (void**)((char*)payload - g_coordPool.m_linkOffset);
                *slot = head;
                head = slot;
                g_coordPool.m_freeHead = head;
            }
        } while (n != 0);
    }
    sel->RemoveAll();
    CTmNode* rec = (CTmNode*)m_recList.GetHeadPosition();
    while (rec != 0) {
        CTmNode* cur = rec;
        rec = rec->m_next;
        i32* src = cur->m_payload;
        void** fh = (void**)g_coordPool.m_freeHead;
        void* nextFree = *fh;
        i32* dst = 0;
        if (nextFree != 0) {
            dst = (i32*)((char*)fh + 4);
            g_coordPool.m_freeHead = nextFree;
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
    CTmNode* n = (CTmNode*)m_selLists[slot].GetHeadPosition();
    if (n == 0) {
        m_selSentinel = -1;
        return 0;
    }
    i32 maxX = 0;
    i32 maxY = 0;
    CPlaneRender* grid = (CPlaneRender*)g_gameReg->m_world->m_24->m_mainPlane;
    i32 minX = grid->m_wrapW - 1;
    i32 minY = grid->m_wrapH - 1;
    do {
        CTmNode* cur = n;
        n = n->m_next;
        i32* payload = cur->m_payload;
        i32 idx = payload[1] + 15 * payload[0];
        CTmCell* cell = m_grid[idx];
        if (cell != 0) {
            ResetCell(payload[0], payload[1], 1, 0);
            if (m_selSentinel == slot) {
                CGruntHud* disp = cell->m_10;
                i32 x = disp->m_5c;
                i32 y = disp->m_60;
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
            void** node = (void**)((char*)payload - g_coordPool.m_linkOffset);
            *node = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_selLists[slot].RemoveAt((POSITION)cur);
        }
    } while (n != 0);
    if (m_selSentinel == slot) {
        ((CPlay*)g_gameReg->m_curState)
            ->ResetGoals(
                minX + (maxX - minX) / 2,
                minY + (maxY - minY) / 2
            ); // 0xd5f00 (was Center)
        m_selSentinel = -1;
        return 1;
    }
    m_selSentinel = slot;
    return 1;
}

// ===========================================================================
// CGroupSel::CenterOnGroup (0x7cf40) - merged from GroupOps.cpp per dossier
// 10b (directly abuts ?CenterSelectionGroup@CTriggerMgr @0x7cd40, same
// selection feature). @identity-TODO: CGroupSel is a placeholder view - its
// +0x1c grid / +0x230..+0x238 latch / +0x244 list head ARE CTriggerMgr's
// m_grid / m_230-m_recX-m_recY / m_recList.m_head; the fold onto CTriggerMgr
// is deferred (the m_grid[1] flexible-index idiom + CSel* sub-views diverge
// from the CTm* shapes). GroupOps.cpp keeps CGroupBroadcast::Broadcast
// (0x112080, a different interval).
// ===========================================================================
// ===========================================================================
// CenterOnGroup (0x7cf40)
// ===========================================================================
// The view-centre helper reached as ((CTmWorld*)g_gameReg->m_curState)->Center(x, y) (0x2e28 thunk).
// The map dimensions grid (gameReg->m_world->m_24->m_5c) is the shared
// CPlaneRender (<Wwd/WwdFile.h>); only its m_wrapW/m_wrapH are read here.
struct CMapHolderB {
    char m_pad00[0x5c];
    CPlaneRender* m_5c; // +0x5c
};
struct CMapHolderA {
    char m_pad00[0x24];
    CMapHolderB* m_24; // +0x24
};

// A selected cell's grunt (cell->m_10) carries its tile position at +0x5c/+0x60.
struct CSelGrunt {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c x
    i32 m_60; // +0x60 y
};
// A grid cell: +0x10 the grunt, +0x1ec/+0x1f0 the latch values.
struct CSelGridCell {
    char m_pad00[0x10];
    CSelGrunt* m_10; // +0x10
    char m_pad14[0x1ec - 0x14];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
};
// A selection list node: next @0, the cell-key object @0x08.
struct CSelKey {
    i32 m_0; // +0x00 grid x-ish
    i32 m_4; // +0x04 grid y-ish
};
struct CSelNode {
    CSelNode* m_next; // +0x00
    void* m_pad04;
    CSelKey* m_8; // +0x08
};

struct CGroupSel {
    char m_pad00[0x1c];
    CSelGridCell* m_grid[1]; // +0x1c  grid cell pointer table (indexed by x*15 + y)
    // The latch / state fields below live well past the grid; the modeled grid
    // stays size-1 (indexed past its end) so these keep their true offsets.
    char m_pad20[0x230 - 0x20];
    i32 m_230; // +0x230  select-latch gate
    i32 m_234; // +0x234  latched x
    i32 m_238; // +0x238  latched y
    char m_pad23c[0x244 - 0x23c];
    CSelNode* m_244; // +0x244  selection list head
    char m_pad248[0x24c - 0x248];
    i32 m_24c;                       // +0x24c  single-selection flag
    i32 CenterOnGroup(i32 doSelect); // 0x7cf40
    i32 TrySelect(i32 a, i32 b);     // 0x33aa
    void Commit();                   // 0x3d1e
};

// @early-stop
// 83% - regalloc wall: the list walk, grid-hash (x*15 + y), bounding-box min/max
// fold, midpoint centre call and single-selection latch are byte-faithful; the
// residual is min/max register colouring + the doubled grid-lookup spill.  No EH.
RVA(0x0007cf40, 0x12e)
i32 CGroupSel::CenterOnGroup(i32 doSelect) {
    CSelNode* n = m_244;
    if (n == 0) {
        return 0;
    }
    CPlaneRender* dims = (CPlaneRender*)g_gameReg->m_world->m_24->m_mainPlane;
    i32 minX = dims->m_wrapW - 1;
    i32 minY = dims->m_wrapH - 1;
    i32 maxX = 0;
    i32 maxY = 0;
    i32 count = 0;
    do {
        CSelKey* k = n->m_8;
        n = n->m_next;
        CSelGridCell* cell = m_grid[k->m_0 * 15 + k->m_4];
        if (cell != 0) {
            count++;
            CSelGrunt* g = cell->m_10;
            i32 gx = g->m_5c;
            i32 gy = g->m_60;
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
    i32 r = ((CPlay*)g_gameReg->m_curState)->ResetGoals(cx, cy);
    if (r != 0 && count == 1 && m_24c == 1) {
        CSelKey* head = m_244->m_8;
        CSelGridCell* cell2 = m_grid[head->m_0 * 15 + head->m_4];
        if (cell2 != 0) {
            i32 v1f0 = cell2->m_1f0;
            i32 v1ec = cell2->m_1ec;
            if (TrySelect(v1ec, v1f0)) {
                m_234 = v1ec;
                m_238 = v1f0;
                m_230 = 1;
                Commit();
            }
        }
    }
    return 1;
}

// 0x7d0c0: ClearSelections - drain all 10 selection lists (+0x2d0, stride 0x1c) back
// to the free list (skipping null-payload nodes), RemoveAll each list, reset +0x3e8.
RVA(0x0007d0c0, 0x57)
void CTriggerMgr::ClearSelections() {
    CPtrList* list = m_selLists;
    i32 k = 10;
    do {
        CTmNode* n = (CTmNode*)list->GetHeadPosition();
        if (n != 0) {
            void* head = g_coordPool.m_freeHead;
            do {
                CTmNode* cur = n;
                n = n->m_next;
                i32* payload = cur->m_payload;
                if (payload != 0) {
                    void** slot = (void**)((char*)payload - g_coordPool.m_linkOffset);
                    *slot = head;
                    head = slot;
                    g_coordPool.m_freeHead = head;
                }
            } while (n != 0);
        }
        list->RemoveAll();
        list++;
        k--;
    } while (k != 0);
    m_selSentinel = -1;
}

// 0x7d140: ClearRow(row) - run ExitGrid on the 15 live, hook-less cells of grid
// row `row` (+0x1c); clear +0x400 when row is the magic group, then refresh world.
RVA(0x0007d140, 0x61)
i32 CTriggerMgr::ClearRow(i32 row) {
    CTmCell** cell = &m_grid[row * 15];
    i32 i = 15;
    do {
        CTmCell* c = *cell;
        if (c != 0 && c->m_deathAnimStarted == 0) {
            ((CGrunt*)c)
                ->BuildGruntExitAnimation(); // ExitGrid==CGrunt::BuildGruntExitAnimation @0x641b0
        }
        cell++;
        i--;
    } while (i != 0);
    if (row == g_curPlayer) {
        m_groupFlag = 0;
    }
    ((CPlay*)g_gameReg->m_curState)->FlushPendingOps(); // Refresh==CPlay::FlushPendingOps @0xda2d0
    return 1;
}

// 0x7d1d0: NearestCellDist - the minimum squared (>>5 tile) distance from (px,py) to
// any live, clickable grid cell, scanning rows 0..3 but skipping row `skipRow`.
// CRACKED (77->100): (1) branchless abs() on the squared sum -> cdq;xor;sub (the
// `if(d<0)d=-d` branch form emits jns;neg); (2) the m_5c distance term declared/evaluated
// BEFORE m_60 (load-order); (3) `r` declared before `row` + `r++` before `row+=15` so the
// loop counter takes the [esp+0x20] slot. docs/patterns/signed-modulo-pow2-abs-restore.md.
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
                    CGruntHud* o = g->m_10;
                    i32 dx = (o->m_5c >> 5) - tx;
                    i32 dy = (o->m_60 >> 5) - ty;
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
        CTmNode* n = (CTmNode*)list->GetHeadPosition();
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
                ((CGrunt*)g)->DestroyAnims(); // -> ?DestroyAnims@CGrunt@@QAEXXZ (0x57d80)
            }
            cell++;
            i--;
        } while (i != 0);
        r--;
    } while (r != 0);

    CSpriteListNode* node = m_level->m_8->m_liveObjects;
    while (node != 0) {
        CTmCell* obj = (CTmCell*)node->m_sprite;
        node = node->next;
        if (obj != 0) {
            char* desc = *(char**)((char*)obj + 0x7c);
            void (CTmCell::*tag)() = &CTmCell::ReadConfigFromButeMgr;
            if (*(void**)(desc + 0x10) == *(void**)&tag) {
                char* tgt = *(char**)(desc + 0x18);
                *(i32*)(tgt + 0x200) = 0;
            }
        }
    }

    DirectSoundMgr* ch0 = m_soundChanA;
    if (ch0 != 0) {
        ch0->StopAndRewind();
        m_soundChanA = 0;
    }
    DirectSoundMgr* ch1 = m_soundChanB;
    if (ch1 != 0) {
        ch1->StopAndRewind();
        m_soundChanB = 0;
    }
    void* state = g_gameReg->PickPausedThenPlayState();
    if (state != 0) {
        char* sub = *(char**)((char*)state + 0x2dc);
        if (sub != 0) {
            DirectSoundMgr* ch2 = *(DirectSoundMgr**)(sub + 0x618);
            if (ch2 != 0) {
                ch2->StopAndRewind();
                *(DirectSoundMgr**)(sub + 0x618) = 0;
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
        ((CPlay*)g_gameReg->m_curState)->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    // negated-condition-far-block: the lookup body lands FAR, cell=0 inline (retail layout)
    CTmCell* cell;
    if (m_recList.GetCount() != 1) {
        cell = 0;
    } else {
        i32* rec = ((CTmNode*)m_recList.GetHeadPosition())->m_payload;
        cell = m_grid[rec[0] * 15 + rec[1]];
    }
    if (cell == 0) {
        return 1;
    }
    if (cell->m_tileOwnerHi != g_curPlayer) {
        return 1;
    }
    if (((CGrunt*)cell)->CanShowStamina() == 0) { // -> ?CanShowStamina@CGrunt@@ (0x514a0)
        OverlayTick();
        return 1;
    }
    i32 v = cell->m_entranceReason;
    if (v > 0x16) {
        v = cell->m_19c;
    }
    if (v == 0x13) {
        CTrigPoint pt;
        pt.x = cell->m_lastTilePxX;
        pt.y = cell->m_lastTilePxY;
        g_gameReg->m_cmdGrid->ResetGroup(pt.x, pt.y, 0, 0, 0, 2, 1);
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = v + kPendingFxIdBase;
    ((CPlay*)g_gameReg->m_curState)->LoadCursorSprites(v + kPendingFxIdBase, 0);
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
        ((CPlay*)g_gameReg->m_curState)->LoadCursorSprites(0, 0);
        return 0;
    }
    m_pendingFxKind = 0;
    CTmCell* cell;
    if (m_recList.GetCount() != 1) { // negated-far cell decode (see ToggleRegionA)
        cell = 0;
    } else {
        i32* rec = ((CTmNode*)m_recList.GetHeadPosition())->m_payload;
        cell = m_grid[rec[0] * 15 + rec[1]];
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
        CGruntHud* o = cell->m_10;
        g_gameReg->m_cmdGrid->ResetGroup(o->m_5c, o->m_60, 0, 0, 0, 3, 1);
        OverlayTick();
        return 1;
    }
    if (kind == 0) {
        OverlayTick();
        return 1;
    }
    m_pendingFxKind = kind + kPendingFxIdBase;
    ((CPlay*)g_gameReg->m_curState)->LoadCursorSprites(kind + kPendingFxIdBase, 0);
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
    CTmNode* n = (CTmNode*)m_recList.GetHeadPosition();
    if (n != 0) {
        i32 magic = g_curPlayer;
        do {
            CTmNode* cur = n;
            n = n->m_next;
            i32* p = cur->m_payload;
            x = *(char*)p;
            CTmCell* cell = m_grid[p[0] * 15 + p[1]];
            if (cell->m_tileOwnerHi == magic && cell->m_entranceActive == 0) {
                buf[count] = ((u8*)p)[4];
                count++;
            }
        } while (n != 0);
    }
    if (count == 1) {
        g_gameReg->m_cmdSubMgr->EnqueueSingle(1, x, (char)buf[0], 5, 0, 0, 0, 0);
    } else {
        g_gameReg->m_cmdSubMgr->EnqueueMulti(1, x, count, (u8*)buf, 5, 0, 0, 0);
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

// Donor-view size annotations (the moved singletons' placeholder shapes).
SIZE_UNKNOWN(CMapHolderB);
SIZE_UNKNOWN(CMapHolderA);
SIZE_UNKNOWN(CSelGrunt);
SIZE_UNKNOWN(CSelGridCell);
SIZE_UNKNOWN(CSelKey);
SIZE_UNKNOWN(CSelNode);
SIZE_UNKNOWN(CGroupSel);

// --- vtable catalog ---

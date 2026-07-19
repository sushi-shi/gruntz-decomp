// ObjectTracker.cpp - CGrunt::StepPeerTracking (0x0f7d90), the per-tick
// peer-tracking behavior step: publish the grunt's own defended position, and if
// its target gate (m_198) is armed, find the nearest enemy on the board and either
// relay its screen position to the trigger grid or (dwell-throttled) re-place at
// its tile and fire the on-screen cue 0x366.
//
// IS ::CGrunt - the one retail caller (thunk 0x2806 from LoadGruntTuningConstants
// @0x5d210, a method data-ref'd inside ??_7CGrunt@@6B@) dispatches it as
// `mov ecx,esi` on its own grunt `this` (0x5dd4e), and every field lands on a
// CGrunt member at the identical offset: m_display==m_10, m_posX/Y==
// m_lastTilePxX/Y (0x17c/0x180), m_target==m_198, m_areaId/m_subId==
// m_tileOwnerHi/Lo (0x1ec/0x1f0), m_active==m_entranceCommitted (0x1fc),
// m_placeKind==m_arrivalFlags (0x248), m_peerSource==m_tileMgr (0x260),
// m_idleState/m_idleFlag==m_arrivalState/m_defenderState (0x2d0/0x2d4),
// m_reportTimer==m_dwell (0x2ec), m_lastX/Y==m_defenderX/Y (0x300/0x304),
// m_cueArmed==m_390. Its helper thunks are the already-reconstructed bodies:
// GetPeer 0x253b -> CTriggerMgr::FindNearestEnemy (0x77df0), CheckScreenPos
// 0x1e97 -> CGrunt::RectContainsGated (0x51a20), CheckOwnerCell 0x1014 ->
// CGrunt::GruntInRadius (0x67b00), PlaceAtTile 0x1640 -> CGrunt::TileSwitch
// (0x4b320), ReportObjectAt 0x3030 -> CTriggerMgr::ApplyTriggerB (0x6e120).
#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <rva.h>
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (m_tileMgr / registry m_cmdGrid)
#include <Gruntz/Grunt.h>      // canonical CGrunt (the ex-CObjectTracker identity) + CGameRegistry
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_world->m_level) + CLevelPlane (m_mainPlane +0x5c)

// The CGameRegistry view of the 0x24556c singleton (the SAME object as WwdGameReg* g_gameReg,
// the 0x24556c dual-view): typed CGameRegistry so its m_cmdGrid (CTriggerMgr) / m_cueSink
// (CGruntCueSink) slots are reached cast-free. Bound here (Win32 dual-view convention).

// The registry's viewport-bounds path is the canonical chain g_gameReg->m_world
// (CDDrawSurfaceMgr) ->m_24 (CGameLevel) ->m_mainPlane (CLevelPlane, +0x5c); the
// on-screen clip bounds are the plane's +0x40..+0x4c origin/extent block
// (m_originX/m_originY/m_extentX/m_extentY).

// @early-stop
// ~95%: logic byte-exact except (1) the TileSwitch (0x1640) arg-setup: retail
// loads ecx=this before the call (a thiscall-shaped dispatch whose body ignores ecx)
// while the shared free-__stdcall model emits no receiver load - one dead mov; and
// (2) the on-screen clip-rect check's addressing mode (reading the CLevelPlane
// +0x40 origin/extent fields directly vs a materialized rect pointer). Both accepted
// regalloc/addressing ripple (structure phase).
RVA(0x000f7d90, 0x171)
i32 CGrunt::StepPeerTracking() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    if (m_198 == 0) {
        m_arrivalState = 5;
        m_defenderState = 0;
        m_dwell = 0;
        return 1;
    }
    CGrunt* p = m_tileMgr->FindNearestEnemy(this);
    if (p == 0) {
        return 1;
    }
    if (p->m_entranceCommitted == 0) {
        return 1;
    }
    CGameObject* a = p->m_10;
    if (a->m_screenX == p->m_lastTilePxX && a->m_screenY == p->m_lastTilePxY
        && RectContainsGated(a->m_screenX, a->m_screenY)) {
        CGameObject* b = p->m_10;
        g_gameReg->m_cmdGrid
            ->ApplyTriggerB(m_tileOwnerHi, m_tileOwnerLo, b->m_screenX, b->m_screenY);
        return 1;
    }
    if (static_cast<u32>(m_dwell) <= 0x3e8) {
        return 1;
    }
    if (GruntInRadius(p->m_tileOwnerHi, p->m_tileOwnerLo)) {
        CGameObject* b = p->m_10;
        TileSwitch(b->m_screenX >> 5, b->m_screenY >> 5, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        if (m_390 == 0) {
            return 1;
        }
        CGameObject* c = m_10;
        CGameRegistry* g = g_gameReg;
        i32 y = c->m_screenY;
        i32 x = c->m_screenX;
        CLevelPlane* r = g->m_world->m_level->m_mainPlane;
        if (x < r->m_extentX && x >= r->m_originX && y < r->m_extentY && y >= r->m_originY) {
            g->m_cueSink->CueEvent(this, 0x366, -1, 0, -1, -1);
        }
    }
    m_390 = 0;
    return 1;
}

#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (m_tileMgr / registry m_cmdGrid)
#include <Gruntz/Grunt.h>      // canonical CGrunt (the ex-CObjectTracker identity) + CGameRegistry
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_world->m_level) + CDDrawWorkerHost (m_mainPlane +0x5c)

// @early-stop
// ~95%: logic byte-exact except (1) the TileSwitch (0x1640) arg-setup: retail
// loads ecx=this before the call (a thiscall-shaped dispatch whose body ignores ecx)
// while the shared free-__stdcall model emits no receiver load - one dead mov; and
// (2) the on-screen clip-rect check's addressing mode (reading the CDDrawWorkerHost
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
    CGameObject* a = p->m_object;
    if (a->m_screenX == p->m_lastTilePxX && a->m_screenY == p->m_lastTilePxY
        && RectContainsGated(a->m_screenX, a->m_screenY)) {
        CGameObject* b = p->m_object;
        g_gameReg->m_cmdGrid
            ->ApplyTriggerB(m_tileOwnerHi, m_tileOwnerLo, b->m_screenX, b->m_screenY);
        return 1;
    }
    if (static_cast<u32>(m_dwell) <= 0x3e8) {
        return 1;
    }
    if (GruntInRadius(p->m_tileOwnerHi, p->m_tileOwnerLo)) {
        CGameObject* b = p->m_object;
        TileSwitch(b->m_screenX >> 5, b->m_screenY >> 5, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        if (m_390 == 0) {
            return 1;
        }
        CWwdGameObjectA* c = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 y = c->m_screenY;
        i32 x = c->m_screenX;
        CDDrawWorkerHost* r = g->m_world->m_level->m_mainPlane;
        if (x < r->m_extentX && x >= r->m_originX && y < r->m_extentY && y >= r->m_originY) {
            g->m_cueSink->CueEvent(this, 0x366, -1, 0, -1, -1);
        }
    }
    m_390 = 0;
    return 1;
}

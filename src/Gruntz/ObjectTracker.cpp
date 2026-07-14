// ObjectTracker.cpp - CObjectTracker::Update (0x0f7d90), the per-tick step of a
// game-object logic leaf that tracks a peer game-object (class + members in
// <Gruntz/ObjectTracker.h>). It mirrors its own screen position, relays the tracked
// peer's (area,sub) trigger-grid id + screen position to the registry's command grid,
// and (throttled) re-places at the peer's tile and fires an on-screen cue.
//
// XREF: Update (0xf7d90) is reached (via ILT 0x2806) from
// CUserLogic::LoadGruntTuningConstants (0x5d210, userlogic), itself reached from
// RegisterGameObjectTypes (0xa3b0) - so this is one of the registered game-object types.
// It reads the game registry singleton (0x64556c). Offsets are load-bearing; engine
// callees are external (reloc-masked).
#include <rva.h>
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (registry m_cmdGrid, +0x68)
#include <Gruntz/Grunt.h> // canonical CGruntCueSink (registry m_cueSink, +0x60) + CGameRegistry + CGameObject
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_world->m_24) + CLevelPlane (m_mainPlane +0x5c)
#include <Gruntz/ObjectTracker.h>

// The CGameRegistry view of the 0x24556c singleton (the SAME object as WwdGameReg* g_gameReg,
// the 0x24556c dual-view): typed CGameRegistry so its m_cmdGrid (CTriggerMgr) / m_cueSink
// (CGruntCueSink) slots are reached cast-free. Bound here (Win32 dual-view convention).
extern "C" CGameRegistry* g_gameReg;

// The registry's viewport-bounds path is the canonical chain g_gameReg->m_world
// (CSpriteFactoryHolder) ->m_24 (CGameLevel) ->m_mainPlane (CLevelPlane, +0x5c); the
// on-screen clip bounds are the plane's +0x40..+0x4c origin/extent block
// (m_originX/m_originY/m_extentX/m_extentY). The former Reg30/Reg24/Reg5c/Rect4 offset
// views were dissolved onto those canonicals (the offset chain is byte-identical).

// @early-stop
// ~95%: logic byte-exact except (1) the PlaceAtTile (0x1640) arg-setup, where MSVC5
// pins m_placeKind in ecx + reuses edi for b->m_screenY while the recompile picks
// eax/ecx/edx; pure regalloc selection in the push sequence, not steerable; and
// (2) the on-screen clip-rect check: reading the CLevelPlane +0x40 origin/extent
// fields directly (m_originX..m_extentY) vs the ex-view's materialized Rect4* at
// +0x40 shifts the compare block's addressing mode. Both are accepted addressing/
// regalloc ripple from the Reg30/Reg24/Reg5c/Rect4 view dissolution (structure phase).
RVA(0x000f7d90, 0x171)
int CObjectTracker::Update() {
    m_lastX = m_posX;
    m_lastY = m_posY;
    if (m_target == 0) {
        m_idleState = 5;
        m_idleFlag = 0;
        m_reportTimer = 0;
        return 1;
    }
    CObjectTracker* p = m_peerSource->GetPeer(this);
    if (p == 0) {
        return 1;
    }
    if (p->m_active == 0) {
        return 1;
    }
    CGameObject* a = p->m_display;
    if (a->m_screenX == p->m_posX && a->m_screenY == p->m_posY
        && CheckScreenPos(a->m_screenX, a->m_screenY)) {
        CGameObject* b = p->m_display;
        g_gameReg->m_cmdGrid->ReportObjectAt(m_areaId, m_subId, b->m_screenX, b->m_screenY);
        return 1;
    }
    if (m_reportTimer <= 0x3e8) {
        return 1;
    }
    if (CheckOwnerCell(p->m_areaId, p->m_subId)) {
        CGameObject* b = p->m_display;
        PlaceAtTile(b->m_screenX >> 5, b->m_screenY >> 5, 0, m_placeKind, 1, 0);
        m_reportTimer = 0;
        if (m_cueArmed == 0) {
            return 1;
        }
        CGameObject* c = m_display;
        CGameRegistry* g = g_gameReg;
        int y = c->m_screenY;
        int x = c->m_screenX;
        CLevelPlane* r = g->m_world->m_24->m_mainPlane;
        if (x < r->m_extentX && x >= r->m_originX && y < r->m_extentY && y >= r->m_originY) {
            g->m_cueSink->CueEvent(this, 0x366, -1, 0, -1, -1);
        }
    }
    m_cueArmed = 0;
    return 1;
}

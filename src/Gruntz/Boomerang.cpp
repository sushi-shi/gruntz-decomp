#include <Gruntz/Boomerang.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GameRegistry.h>
#include <rva.h>
#include <Io/FileMem.h>
#include <Gruntz/FreeNodePool.h>

VTBL(CBoomerang, 0x001e792c);
DATA(0x001eaae8)
const double g_projPhase0 = 3.1415927;

DATA(0x001eaad8)
const double g_boomHalf = 0.5;
DATA(0x001eaae0)
const double g_boomTimeScale = 0.03125;
DATA(0x001eaaf0)
const double g_boomRetC3 = 0.0625;
DATA(0x001eaaf8)
const double g_boomRetC4 = -500.0;

RVA_COMPGEN(0x000129d0, 0x1e, ??_GCBoomerang@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a00, 0x5, ??1CBoomerang@@UAE@XZ)

// @early-stop
RVA(0x000e0650, 0x2b)
CBoomerang::CBoomerang(CGameObject* owner) : CProjectile(owner) {

    m_wwdObject->m_flags |= 0x2000002;
}

// @early-stop
RVA(0x000e0690, 0x1a9)
i32 CBoomerang::LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1) {
    if (CProjectile::LoadProjectileSprites(kind, a, b, sx, sy, t0, t1) == 0) {
        return 0;
    }
    CWwdGameObjectA* owner = m_object;
    m_launchX = owner->m_screenX;
    m_launchY = owner->m_screenY;
    double d =
        g_projPhase0
        / (static_cast<double>(static_cast<u32>(m_timePerTile)) * g_boomTimeScale * m_flightDist);
    double originX =
        (static_cast<double>(m_targetX) + static_cast<double>(owner->m_screenX)) * g_boomHalf;
    double originY =
        (static_cast<double>(m_targetY) + static_cast<double>(owner->m_screenY)) * g_boomHalf;
    m_originX = originX;
    m_originY = originY;
    m_dirX = originX - static_cast<double>(owner->m_screenX);
    m_dirY = originY - static_cast<double>(owner->m_screenY);
    m_phase = 0.0;
    m_velScale = d;
    CGrunt* g = g_gameReg->m_cmdGrid->m_grid[15 * a + b];
    if (g != 0) {
        g->m_holdWindowLo = static_cast<i32>((d * m_flightDist * g_boomRetC3 - g_boomRetC4));
        g->m_holdWindowHi = 0;
        g->m_holdAnchorLo = g_frameTime;
        g->m_holdAnchorHi = 0;
        if (g->CoordCount() != 0) {
            POSITION pos = g->m_coordList.GetHeadPosition();
            while (pos != 0) {
                void* data = g->m_coordList.GetNext(pos);
                if (data != 0) {
                    CoordPoolNode* p = g_coordPool.NodeOf(data);
                    p->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = p;
                }
            }
            g->m_coordList.RemoveAll();
        }
    }
    m_launched = 0;
    return 1;
}

RVA(0x000e15d0, 0x155)
i32 CBoomerang::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {
        case 7:
            ar->Read(&m_launchX, 4);
            ar->Read(&m_launchY, 4);
            ar->Read(&m_dirX, 8);
            ar->Read(&m_dirY, 8);
            ar->Read(&m_originX, 8);
            ar->Read(&m_originY, 8);
            ar->Read(&m_phase, 8);
            ar->Read(&m_launched, 4);
            break;
        case 4:
            ar->Write(&m_launchX, 4);
            ar->Write(&m_launchY, 4);
            ar->Write(&m_dirX, 8);
            ar->Write(&m_dirY, 8);
            ar->Write(&m_originX, 8);
            ar->Write(&m_originY, 8);
            ar->Write(&m_phase, 8);
            ar->Write(&m_launched, 4);
            break;
    }
    return CProjectile::SerializeMove(ar, mode, typeId, pObj) ? 1 : 0;
}

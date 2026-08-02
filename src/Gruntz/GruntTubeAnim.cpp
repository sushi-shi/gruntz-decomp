#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/State.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/GameRegistry.h>

#include <Wap32/ZVec.h>
#include <string.h>

// @early-stop
RVA(0x00050a50, 0x1c5)
i32 CGrunt::SetupTubeAnim(i32 isWater) {
    m_reachRect.left = -1;
    m_reachRect.top = -1;
    m_reachRect.right = 1;
    m_reachRect.bottom = 1;
    m_coordToggle = isWater;
    m_reachExclusionRect.left = 0;
    m_reachExclusionRect.top = 0;
    m_reachExclusionRect.right = 0;
    m_reachExclusionRect.bottom = 0;

    m_animSetName = (isWater == 0) ? "TOOBGRUNT" : "TOOBWATERGRUNT";
    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_animSetName, 1, 1, 0);
    ReadConfigFromButeMgr();
    LoadCellAnimNames(0, 0);
    LoadAnimNameTable(0, 0);

    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(0, 0, 1);
    }

    CString* node = g_typeColl.ScratchResolve(m_objAux->ActKey());

    CString* p = g_typeColl.Slots();
    i32 count = g_typeColl.m_grown;
    for (i32 i = count; i != 0; i--) {
        if (p != 0) {
            p->CString::CString();
        }
        p++;
    }

    if (strcmp(*node, "D") == 0) {
        GruntDirectionCell cell = m_entranceCell;
        i32 idx = cell.row * 3 + cell.column;
        char* buf = m_cells[idx].WalkName().GetBuffer(0);
        m_wwdObject->ApplyName(buf);
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
        return 1;
    }
    ResetEntranceAnimation(0, 0, 1);
    return 1;
}

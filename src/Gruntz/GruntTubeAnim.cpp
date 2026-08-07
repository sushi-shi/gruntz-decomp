#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/State.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>

#include <new>
#include <string.h>

static __inline void ConstructGrownSlots() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt--) {
        if (slot != NULL) {
            new (slot) CString();
        }
        slot++;
    }
}

// @early-stop
RVA(0x00050a50, 0x1c5)
i32 CGrunt::SetupTubeAnim(i32 isWater) {
    m_reachRect = CRect(-1, -1, 1, 1);
    m_reachExclusionRect = CRect(0, 0, 0, 0);
    m_coordToggle = isWater;

    if (isWater != 0) {
        m_animSetName = "TOOBWATERGRUNT";
    } else {
        m_animSetName = "TOOBGRUNT";
    }
    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_animSetName, 1, 1, 0);
    ReadConfigFromButeMgr();
    LoadCellAnimNames(0, 0);
    LoadAnimNameTable(0, 0);

    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }

    CString* node = g_typeColl.ScratchResolve(m_objAux->ActKey());
    ConstructGrownSlots();

    bool eq;
    eq = (strcmp(*node, "D") == 0);
    if (eq) {
        GruntDirectionCell cell = m_entranceCell;
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        char* buf = m_cells[base].WalkName().GetBuffer(0);
        m_wwdObject->ApplyName(buf);
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
        return 1;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 1;
}

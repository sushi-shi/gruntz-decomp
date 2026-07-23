#include <Mfc.h>                  // afx-first: CString + <windows.h>; keep before any Win32 header
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)            // canonical CGrunt (+ CEntranceAnimPlayer/CGruntCellRec/etc.)
#include <Gruntz/AniAdvanceCursor.h>  // CAniAdvanceCursor::Setup (the +0x1a0 blit param)
#include <Gruntz/State.h> // CState::BuildAssetNamespacePrefixes (ex CNamespaceLoader facet, m_curState)
#include <Gruntz/TypeKeyColl.h>  // g_typeColl (+ CAnimNameRecord, _zvec::IndexToPtr)
#include <Gruntz/GameRegistry.h> // g_gameReg

#include <Wap32/ZVec.h> // _zvec
#include <string.h>     // intrinsic strcmp ("D")

// @early-stop
// const-materialize-into-reg wall (docs/patterns/const-materialize-into-reg-vs-
// immediate.md): the {-1,-1,1,1}/{0,0,0,0} rect-block init, the kind latch, the
// TOOB(WATER)GRUNT name select + Register, the three reset helpers, the gated
// entrance re-init, the g_typeColl resolve + g_typeColl.m_alloc reset loop, the inline
// strcmp("D") and the matched-branch GetBuffer/CacheFirstFrame/blit-param/descriptor
// slot build are byte-faithful. Residual: the interleaved xor/mov const-into-reg
// scheduling of the two rect blocks + the dead m_entranceCell.reason spill - the
// MSVC5 scheduler coin-flip, not source-steerable.
RVA(0x00050a50, 0x1c5)
i32 CGrunt::SetupTubeAnim(i32 isWater) {
    m_reachRectLeft = -1;
    m_reachRectTop = -1;
    m_reachRadius = 1;
    m_reachRectBottom = 1;
    m_coordToggle = isWater;
    m_2a0 = 0;
    m_2a4 = 0;
    m_2a8 = 0;
    m_2ac = 0;
    m_animSetName = isWater ? "TOOBWATERGRUNT" : "TOOBGRUNT";
    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_animSetName, 1, 1, 0);
    ReadConfigFromButeMgr();
    LoadCellAnimNames(0, 0);
    LoadAnimNameTable(0, 0);
    if (m_poweredUp == 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(0, 0, 1);
    }

    CAnimNameRecord* node = reinterpret_cast<CAnimNameRecord*>(
        (static_cast<_zvec*>(&g_typeColl))->IndexToPtr(reinterpret_cast<i32>(m_objAux->m_1c))
    );
    void* p = reinterpret_cast<void*>(
        g_typeColl.m_alloc
    ); // m_alloc is the i32-typed slot base (the _zvec spelling)
    i32 count = g_typeColl.m_grown;
    for (i32 i = count; i != 0; i--) {
        if (p != 0) {
            (static_cast<CString*>(p))
                ->CString::CString(); // 0x1b9b93 re-init the freed registry slot
        }
        p = reinterpret_cast<char*>(p) + 4;
    }

    if (strcmp(node->m_name, "D") == 0) {
        GruntEntranceCell cell = m_entranceCell;
        i32 idx = cell.col * 3 + cell.row;
        char* buf = m_cells[idx].WalkName().GetBuffer(0);
        m_38->ApplyName(buf); // 0x1504d0 (the player IS the created game object)
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseWalk);
        return 1;
    }
    ResetEntranceAnimation(0, 0, 1);
    return 1;
}

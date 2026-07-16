// GruntTubeAnim.cpp - CGrunt::SetupTubeAnim (0x050a50, __thiscall ret 4). The
// "toob" (pipe) grunt entrance-anim setup: resets the +0x290/+0x2a0 rect blocks,
// latches the kind flag at +0x234, picks the TOOBGRUNT / TOOBWATERGRUNT anim-set
// name into m_animSetName (+0x1c0), registers it through the settings manager,
// runs three reset helpers, conditionally re-inits the entrance gate, clears the
// shared type-name registry array (g_typeColl/g_typeColl.m_alloc/g_typeColl.m_grown), and - when
// the resolved type name is "D" - caches the first entrance frame into the +0x154
// player and stamps its +0x1a0 blit param + +0x15c descriptor.
//
// This IS a CGrunt method on the canonical CGrunt (<Gruntz/Grunt.h>): the offset-
// faithful local CGruntTube view is DISSOLVED - m_14 is CGrunt::m_14 (CAnimLookupNode),
// m_154 is CEntranceAnimPlayer, m_1c0 is m_animSetName, m_394 is m_poseWalk, m_43c is
// m_entranceCell (GruntEntranceCell), m_470 is the m_cells[9] entrance-record table
// (the record's +8 CString is m_walk). The four reset helpers were reloc-masked thunk
// callees; XREF (sema xref) proved their real identities: Reset30ee = ReadConfigFromButeMgr
// (0x48400), Reset1677 = LoadCellAnimNames (0x48470), Reset160e = LoadAnimNameTable
// (0x49c60), ResetGate136b = ResetEntranceAnimation (0x62e10). The +0x154 anim player's
// type node resolves to a CAnimNameRecord (<Gruntz/TypeKeyColl.h>).
#include <Mfc.h> // afx-first: CString + <windows.h>; keep before any Win32 header
#include <rva.h>

#include <Gruntz/Grunt.h>            // canonical CGrunt (+ CEntranceAnimPlayer/CGruntCellRec/etc.)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Setup_15c2d0 (the +0x1a0 blit param)
#include <Gruntz/State.h> // CState::BuildAssetNamespacePrefixes (ex CNamespaceLoader facet, m_curState)
#include <Gruntz/TypeKeyColl.h>  // g_typeColl (+ CAnimNameRecord, _zvec::IndexToPtr)
#include <Gruntz/GameRegistry.h> // g_gameReg

#include <Wap32/ZVec.h> // _zvec
#include <string.h>     // intrinsic strcmp ("D")

// The settings singleton (0x64556c) viewed for its +0x2c manager.
extern "C" CGameRegistry* g_gameReg;

// The shared type-name registry (0x6bf650/0x6bf66c/0x6bf670).

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

    CAnimNameRecord* node = (CAnimNameRecord*)((_zvec*)&g_typeColl)->IndexToPtr((i32)m_14->m_1c);
    void* p = (void*)g_typeColl.m_alloc; // m_alloc is the i32-typed slot base (the _zvec spelling)
    i32 count = g_typeColl.m_grown;
    for (i32 i = count; i != 0; i--) {
        if (p != 0) {
            ((CString*)p)->CString::CString(); // 0x1b9b93 re-init the freed registry slot
        }
        p = (char*)p + 4;
    }

    if (strcmp(node->m_name, "D") == 0) {
        GruntEntranceCell cell = m_entranceCell;
        i32 idx = cell.col * 3 + cell.row;
        char* buf = m_cells[idx].m_walk.GetBuffer(0);
        m_154->CacheFirstFrame(buf); // 0x1504d0 (the player IS the created game object)
        m_prevEntranceDesc = m_154->m_1a0.m_14;
        m_154->Cursor()->Setup_15c2d0((CAniElement*)m_poseWalk);
        return 1;
    }
    ResetEntranceAnimation(0, 0, 1);
    return 1;
}

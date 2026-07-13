// GruntTubeAnim.cpp - CGrunt::SetupTubeAnim (0x050a50, __thiscall ret 4). The
// "toob" (pipe) grunt entrance-anim setup: resets the +0x290/+0x2a0 rect blocks,
// latches the kind flag at +0x234, picks the TOOBGRUNT / TOOBWATERGRUNT anim-set
// name into m_animSetName (+0x1c0), registers it through the settings manager,
// runs three reset helpers, conditionally re-inits the entrance gate, clears the
// shared type-name registry array (g_typeColl/g_typeColl.m_alloc/g_typeColl.m_grown), and - when
// the resolved type name is "D" - caches the first entrance frame into the +0x154
// player and stamps its +0x1a0 blit param + +0x15c descriptor.
//
// This IS a CGrunt method (the object is the canonical CGrunt: m_14 anim-lookup,
// m_154 CEntranceAnimPlayer, m_1c0 m_animSetName, m_394 _WALK pose, m_43c entrance
// cell, m_470 entrance-record table - see include/Gruntz/Grunt.h). It is modeled
// here with an offset-faithful local CGrunt view rather than the full shared header
// so the (large, plateauing) reconstruction stays isolated; the real home is
// Grunt.cpp and a final-sweep re-home is harmless (offsets + bytes are identical).
// wave3-I partition note: SetupTubeAnim @0x50a50's birth interval is the lone
// 0x50a50-0x50c15 block 139 bytes before the GruntSteps TU (0x50ca0) - a PROBABLE
// head of that obj (small gap; the toob IS a vehicle, thematically adjacent to
// LoadVehicleGruntSprites @0x50ce0), but with no private .data cells or init
// frags to prove it, it stays split (@identity-TODO).
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/AssetNamespaceLoader.h>
#include <DDrawMgr/DDrawBlitParam.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>    // CString + <windows.h>
#include <string.h> // intrinsic strcmp ("D")

// --- modeled externs (reloc-masked, no body) -------------------------------
// The +0x154 tube-anim sub-object; ApplyName @0x150540 is CGameObject::ApplyName (the
// created sprite's first-frame cache), cast to the real owner class at the call.
#include <Gruntz/UserLogic.h>
struct CTubeAnimPlayer { // CGrunt::m_154
    char _00[0x1a0];
    i32 m_1a0; // +0x1a0 CDDrawBlitParam sub-descriptor (Setup_15c2d0)
    char _1a4[0x1b4 - 0x1a4];
    i32 m_1b4; // +0x1b4
};
struct CTubeRecord { // entrance-record, 0x68-byte stride; element 0 = the CString
    CString name;    // +0x00
    char _04[0x68 - 4];
};
struct CTubeTypeNode {
    char* m_name; // +0x00  resolved type name
};
struct CTubeAnimLookup { // CGrunt::m_14
    char _00[0x1c];
    i32 m_1c; // +0x1c
};

// The settings singleton (0x64556c) viewed for its +0x2c manager.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The shared type-name registry (0x6bf650/0x6bf66c/0x6bf670).
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl;

// --- the offset-faithful CGrunt view ---------------------------------------
struct CGruntTube {
    i32 SetupTubeAnim(i32 isWater);          // 0x050a50
    void Reset30ee();                        // 0x30ee
    void Reset1677(i32 a, i32 b);            // 0x1677
    void Reset160e(i32 a, i32 b);            // 0x160e
    void ResetGate136b(i32 a, i32 b, i32 c); // 0x136b

    char _00[0x14];
    CTubeAnimLookup* m_14; // +0x14
    char _18[0x154 - 0x18];
    CTubeAnimPlayer* m_154; // +0x154
    char _158[0x15c - 0x158];
    i32 m_15c; // +0x15c
    char _160[0x1c0 - 0x160];
    CString m_1c0; // +0x1c0  m_animSetName
    char _1c4[0x1e4 - 0x1c4];
    i32 m_1e4; // +0x1e4
    char _1e8[0x218 - 0x1e8];
    i32 m_218; // +0x218
    i32 m_21c; // +0x21c
    i32 m_220; // +0x220
    char _224[0x234 - 0x224];
    i32 m_234; // +0x234
    char _238[0x290 - 0x238];
    i32 m_290[4]; // +0x290
    i32 m_2a0[4]; // +0x2a0
    char _2b0[0x394 - 0x2b0];
    i32 m_394; // +0x394  _WALK pose
    char _398[0x43c - 0x398];
    i32 m_43c[3]; // +0x43c  entrance cell {col,row,reason}
    char _448[0x470 - 0x448];
    CTubeRecord m_470[1]; // +0x470  entrance-record table
};

// @early-stop
// const-materialize-into-reg wall (docs/patterns/const-materialize-into-reg-vs-
// immediate.md): the {-1,-1,1,1}/{0,0,0,0} rect-block init, the kind latch, the
// TOOB(WATER)GRUNT name select + Register, the three reset helpers, the gated
// entrance re-init, the g_typeColl resolve + g_typeColl.m_alloc reset loop, the inline
// strcmp("D") and the matched-branch GetBuffer/CacheFirstFrame/blit-param/descriptor
// slot build are byte-faithful. Residual: the interleaved xor/mov const-into-reg
// scheduling of the two rect blocks + the dead m_43c[2] spill - the MSVC5 scheduler
// coin-flip, not source-steerable.
RVA(0x00050a50, 0x1c5)
i32 CGruntTube::SetupTubeAnim(i32 isWater) {
    m_290[0] = -1;
    m_290[1] = -1;
    m_290[2] = 1;
    m_290[3] = 1;
    m_234 = isWater;
    m_2a0[0] = 0;
    m_2a0[1] = 0;
    m_2a0[2] = 0;
    m_2a0[3] = 0;
    m_1c0 = isWater ? "TOOBWATERGRUNT" : "TOOBGRUNT";
    ((CNamespaceLoader*)g_gameReg->m_curState)->BuildAssetNamespacePrefixes(m_1c0, 1, 1, 0);
    Reset30ee();
    Reset1677(0, 0);
    Reset160e(0, 0);
    if (m_220 == 0 && m_21c == 0) {
        m_1e4 = 0;
        m_218 = 0;
        m_21c = 0;
        m_220 = 0;
        ResetGate136b(0, 0, 1);
    }

    CTubeTypeNode* node = (CTubeTypeNode*)((_zvec*)&g_typeColl)->IndexToPtr((i32)m_14->m_1c);
    void* p = (void*)g_typeColl.m_alloc; // m_alloc is the i32-typed slot base (the _zvec spelling)
    i32 count = g_typeColl.m_grown;
    for (i32 i = count; i != 0; i--) {
        if (p != 0) {
            ((CString*)p)->CString::CString(); // 0x1b9b93 re-init the freed registry slot
        }
        p = (char*)p + 4;
    }

    if (strcmp(node->m_name, "D") == 0) {
        i32 idx = m_43c[0] * 3 + m_43c[1];
        char* buf = m_470[idx].name.GetBuffer(0);
        ((CGameObject*)m_154)->ApplyName(buf); // 0x150540 (real owner CGameObject)
        m_15c = m_154->m_1b4;
        ((CDDrawBlitParam*)&m_154->m_1a0)->Setup_15c2d0((CDDrawBlitParamSrc*)m_394);
        return 1;
    }
    ResetGate136b(0, 0, 1);
    return 1;
}

SIZE_UNKNOWN(CGruntTube);
SIZE_UNKNOWN(CTubeAnimLookup);
SIZE_UNKNOWN(CTubeAnimPlayer);
SIZE_UNKNOWN(CTubeMgr2c);
SIZE_UNKNOWN(CTubeRecord);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CTubeTypeNode);

#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>

#include <Mfc.h> // afx-first (TU pulls MFC via unified CObject; superset of Win32.h) // wsprintfA
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections (MakeAndAddB/RemoveItemA) + CDDSurface
#include <DDrawMgr/DDSurface.h>           // CDDSurface (BltEx 0x13eef0, SaveFile 0x13f910)
#include <Gruntz/GameRegistry.h>  // CGameRegistry (m_world @+0x30, m_modeW/H) + CDDrawSurfaceMgr
#include <Utils/RegistryHelper.h> // Utils::RegistryHelper (GetValueDword/SetValueDword)

// @source: decomp-xref
// @early-stop
// regalloc-coloring wall (~96.8%): frame/instruction-selection/constants/strings
// are byte-faithful (frame 0xa8, all locals + the descriptor block at the retail
// offsets), but MSVC colored the two callee-saved registers oppositely - retail
// pins the persistent zero (and the reused return temp) in esi and cnt/img in edi,
// while this /O2 recompile pins them edi<->esi swapped; that propagates through
// every null-compare/push. The descriptor fill also CSEs the g_gameReg m_modeW/m_modeH
// loads where retail re-reads each twice (a scheduling choice). Logic complete; the
// coloring/scheduling is the allocator's, not source-steerable. See
// docs/patterns/zero-register-pinning.md + pin-local-for-callee-saved-reg.md.
RVA(0x00114ff0, 0x1b3)
i32 SaveScreenshot(
    CDDSurface* src,
    Utils::RegistryHelper* bute,
    CGruntzMgr* owner,
    i32 arg4,
    i32 arg5,
    char* name,
    void* arg7
) {
    char nameBuf[0x80];
    i32 descB[6];
    i32 descA[4];

    if (src == 0) {
        return 0;
    }
    if (bute == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (owner->m_world == 0) {
        return 0;
    }
    if (name == 0) {
        i32 cnt = bute->GetValueDword("Screen Dump Count", 0) + 1;
        bute->SetValueDword("Screen Dump Count", cnt);
        wsprintfA(nameBuf, "Gruntz%04i.BMP", cnt);
        name = nameBuf;
    }

    CDDrawPtrCollections* surf = owner->m_world->m_ptrColl;
    if (surf == 0) {
        return 0;
    }
    CDDSurface* img = surf->MakeAndAddB(arg4, arg5, 0x10, 0, -1);
    if (img == 0) {
        return 0;
    }

    CGruntzMgr* mgr = g_gameReg;
    descA[0] = 0;
    descA[1] = 0;
    descA[2] = 0;
    descA[3] = 0;
    descB[0] = 0;
    descB[1] = 0;
    descB[2] = 0;
    descB[3] = 0;
    descA[2] = mgr->m_modeW;
    descB[5] = mgr->m_modeH;
    descA[3] = mgr->m_modeH;
    descB[4] = mgr->m_modeW;
    descB[2] = arg4;
    descB[3] = arg5;
    if (img->BltEx(descB, src, descA, 0x1000000, 0)) {
        surf->RemoveItemA(img);
        return 0;
    }
    i32 r = img->SaveFile(name, 1, 0, arg7);
    surf->RemoveItemA(img);
    return r;
}

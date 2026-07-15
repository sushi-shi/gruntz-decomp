#include <Mfc.h>            // CPtrList/CString machinery (reloc-masked); /GX EH frame
#include <Bute/SymParser.h> // canonical CSymParser + CSymTab (ResolvePath @0x13c030/0x13bae0)
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h> // the CDrawTarget pages (m_width/m_height extent)

#include <rva.h>
#include <Gruntz/GameRegistry.h> // canonical CSpriteFactoryHolder (this->m_c) + CGameRegistry
#include <Gruntz/GameMode.h>     // canonical CMenuState : CState (the one true shape)
#include <Gruntz/ResMgr.h>       // canonical CImageRegistry (this->m_c->m_10) + CDrawTarget
#include <Gruntz/GruntzMgr.h>    // canonical CGruntzMgr (m_4 owner) + WAP32::CGameWnd (m_gameWnd)
#include <Gruntz/ChatBox.h>      // canonical CChatBox (this->m_1b4 menu UI object)
#include <Gruntz/LeafCue.h>      // canonical LeafCue (m_1bc/m_1b8 sound-cue map value)
#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst (LeafCue::m_10 player; m_durationMs +0x28)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (HasKeyEqual_155550)
// MenuStateAssets.cpp - CMenuState::LoadAssets (0x09fe50, 835 B), the MENU game-
// state asset loader.  Sibling of CHelpState::LoadAssets / GameLevelState loaders:
// chains the base namespace loader, registers the "MENU" IMAGEZ+SOUNDZ namespaces
// through the m_c->m_10 (vtable +0x48) / m_c->m_28 registries, primes the state
// core (m_c->m_4 IsReady/Init), then heap-allocates the menu HUD object (CPtrList +
// two CString members) and wires its MENU_CURSOR/SELECT/ACTIVATE/MENU keys + the
// MENU_ACTIVATE / MENU_MENU sound cues.  The destructible CPtrList/CString members
// of the heap object give the routine its /GX exception frame.
//
// Only offsets / code bytes are load-bearing; every engine callee is a reloc-
// masked external (no body).

// The image registry reached through this->m_c->m_10 is the canonical CImageRegistry
// (<Gruntz/ResMgr.h>): non-virtual Has + the vtable-slot-18 (+0x48) Install. Shared,
// so no local view.

// --- the sound-cue lookup ---
// The MENU_ACTIVATE / MENU_MENU cues are looked up in the LeafScan cache's embedded
// CMapStringToPtr (+0x10, Lookup @0x1b8438 - mfc_class-confirmed CMapStringToPtr, NOT
// CMapStringToOb).  The map VALUE is the canonical LeafCue (<Gruntz/LeafCue.h>, the
// 0x1c-byte element the LeafScan factory CreateEntry @0x157d70 news): its DSoundCloneInst
// m_10 player carries the cached DS-buffer duration at m_durationMs (+0x28).  The former
// MenuSndEntry / MenuSndEntryInner views are dissolved onto LeafCue / DSoundCloneInst
// (the map value's real identity - see the LeafCue.h header verdict).

// The CState base facets are the ONE real classes (State.h) - this TU no longer keeps
// per-TU views of any of them:
//   * m_4  (CGruntzMgr)            - RestoreVideoMode; its CGameMgr base carries
//                                    m_gameWnd (+0x04), whose m_hwnd (+0x04) is the
//                                    HWND the menu layout is seeded with.
//   * m_8  (CBankMgr)              - reached as CSymParser for ResolvePath.
//   * m_c  (CSpriteFactoryHolder)  - the resource holder: m_drawTarget (+0x04) page
//                                    pump, m_10 image registry, m_28 sound registry.
//   * m_2c (CResSource)            - the registered STATEZ_MENU object.
//   * m_1b4 (CChatBox)             - the heap menu-UI object this routine builds.

// The global mgr singleton (*0x24556c): its resource holder's +0x28 sound registry
// carries the shared cue map the MENU_MENU cue is resolved from. That holder slot
// (CSpriteFactoryHolder::m_28) is a genuinely heterogeneous void* - other TUs view it
// as a sound-set (HbSndSet) or a mute gate - so it is cast to the sound-registry view
// at this one use-site (the authentic proven-heterogeneous-slot cast).
extern "C" CGameRegistry* g_gameReg; // *0x64556c canonical singleton (def: GruntzMgr.cpp)
extern i32 g_resourceInstallActive;

// The heap-allocated MENU UI object (0x7c bytes) IS the canonical CChatBox
// (<Gruntz/ChatBox.h>): its CPtrList m_nodeList (+0x24), m_activeNode (+0x40) and the
// two CString row keys (+0x44/+0x48) are exactly the members this routine constructs
// and writes, and GameMode.h already types the receiving slot `CChatBox* m_1b4`.  The
// destructible CPtrList/CString members give LoadAssets its /GX EH frame.  The former
// MenuHudObj view is gone; its own comments already named every method as CChatBox's
// (Init 0xa0280 via thunk 0x10c8, AdvanceRow0 0x182df0, AdvanceRow1 0x182e60).

// FUN_00402fcc __cdecl: commit the menu UI object (ret BOOL).
i32 MenuCommit(CChatBox* obj, i32 idx); // 0x402fcc

// The menu-region seeder's `this` record (0x182ab0) IS the CChatBox LoadAssets just
// newed - now dissolved onto CChatBox::InitRegion (<Gruntz/ChatBox.h>). The three
// canonical changes the old view was blocked on are all resolved in ChatBox.h now
// (m_page is CSpriteFactoryHolder*, +0x08 is a real RECT m_rect8, m_wrapFlag is i32).

// CMenuState is the canonical <Gruntz/GameMode.h> `CMenuState : CState`. The MENU
// asset loader reaches the CState base region through the SAME facets the game-state
// hierarchy documents (CState.h: the +0x04 owner and +0x0c CSpriteFactoryHolder holder are downcast
// to each TU's local facet views): m_4 (CGruntzMgr owner) -> MenuRoot cursor gate,
// m_8 (CBankMgr) -> MenuRegSet, m_c (CSpriteFactoryHolder) -> MenuAssetMgr resource holder, m_2c
// (CResSource) -> the STATEZ_MENU MenuRegObj. m_1b4 (CGMMenuUI) is the heap MenuHudObj
// the routine builds. Only offsets / code bytes are load-bearing.

// @early-stop
// frame-layout / regalloc wall: complete + correct body - instruction
// selection, the guarded registry-call chain, the heap CChatBox CPtrList+2 CString
// construction + EH trylevel, the MENU_CURSOR/SELECT/ACTIVATE keys and the two sound
// finds all match retail (verified instruction-by-instruction; the ARG_MISMATCH
// rows are the reloc-name scoring artifact).  Residual: retail frame-allocates 0x10
// of locals while this /O2 recompile allocates 0x14 (one extra slot), yielding a +4
// cascade across every [esp+N] operand, plus the base-loader arg push scheduling and
// the new-obj-vs-EH-state interleave - all allocator choices, not source-steerable.
// See docs/patterns/zero-register-pinning.md + identical-return-epilogue-tailmerge.md.
RVA(0x0009fe50, 0x343)
i32 CMenuState::LoadAssets(i32 a1, i32 a2, i32 a3) {
    if (a3 == 0) {
        return 0;
    }
    if (!LoadGameAssetNamespaces(a2, a3, a3)) {
        return 0;
    }
    m_4->RestoreVideoMode(0);
    m_2c = (CResSource*)m_8->ResolvePath("STATEZ_MENU");
    if (m_2c == 0) {
        return 0;
    }

    if (!((CDDrawWorkerRegistry*)m_c->m_10)->HasKeyEqual_155550("MENU")) {
        void* set = SymTab2c()->ResolvePath("IMAGEZ");
        if (set == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_c->m_10->InstallTree(set, "MENU", "_");
        g_resourceInstallActive = 0;
    }

    if (!((CDDrawSubMgrLeafScan*)m_c->m_28)->HasKeyEqual_1583c0("MENU")) {
        void* set = SymTab2c()->ResolvePath("SOUNDZ");
        if (set == 0) {
            return 0;
        }
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->ScanTree_157ee0((CSymTab*)set, "MENU", "_");
    }

    if (!((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158d20()) {
        if (!((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158cb0(0, 0x30000)) {
            return 0;
        }
    }

    RECT rc;
    rc.left = 0;
    rc.top = 8;
    rc.right = 0x27f;
    rc.bottom = 0x1df;
    m_1b4 = new CChatBox;
    m_1b4->Init();

    // 0x182ab0 is __thiscall on the freshly-built CChatBox (retail: `mov [esi+0x1b4],ecx`
    // then `call 0x182ab0` with ecx still the new object; `ret 0x18` = callee-cleaned
    // 6 stack args).  It seeds the box from the resource holder + the game window's HWND.
    if (!m_1b4->InitRegion(m_c, (i32)m_4->m_gameWnd->m_hwnd, &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    if (m_1b4->AdvanceRow0((void*)"MENU_CURSOR", 0x64, 0x20)) {
        m_1b4->AdvanceRow1((void*)"MENU_CURSOR", 0x64, 0x20);
    }
    m_1b4->m_row0Key = "MENU_SELECT";
    m_1b4->m_row1Key = "MENU_ACTIVATE";

    LeafCue* e;
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->m_10.Lookup("MENU_ACTIVATE", (void*&)e);
    if (e != 0) {
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->m_10.Lookup("MENU_ACTIVATE", (void*&)e);
        m_1b8 = e->m_10->m_durationMs;
    } else {
        m_1b8 = 0;
    }

    if (!MenuCommit(m_1b4, -1)) {
        return 0;
    }

    LeafCue* fm;
    ((CDDrawSubMgrLeafScan*)g_gameReg->m_world->m_28)->m_10.Lookup("MENU_MENU", (void*&)fm);
    m_1bc = fm;
    return 1;
}

// ---------------------------------------------------------------------------
// The menu-region seeder (0x0182ab0). The three SOURCE-side views are gone: the retail
// call proves the arg chain is entirely canonical classes -
//   arg1 = CMenuState::m_c            -> CSpriteFactoryHolder (<Gruntz/GameRegistry.h>)
//          holder->m_drawTarget       -> CDrawTarget          (<Gruntz/ResMgr.h>)
//          drawTarget->m_10           -> CDrawTarget::SurfaceA (its +0x10/+0x14 pixel
//                                        extent is now named there; see the disasm cited
//                                        in ResMgr.h) - the default RECT is (0,0,w-1,h-1).
//   arg2 = m_4->m_gameWnd->m_hwnd     -> the game window's HWND (WAP32::CGameWnd +0x04).
//
// The `this` IS the CChatBox LoadAssets just newed (retail `mov [esi+0x1b4],ecx` right
// before `call 0x182ab0`, ecx unchanged) - now a real CChatBox method. Every store lands
// on a CChatBox member (m_page/m_4/the m_rect8 RECT/m_18/m_1c/m_wrapFlag/m_activeNode);
// the three ex-blockers (m_page CChatPage->CSpriteFactoryHolder, m_pad8->RECT, m_wrapFlag
// char->i32) are all resolved in ChatBox.h, so the ex MenuRegion view is dissolved.
RVA(0x00182ab0, 0x7b)
i32 CChatBox::InitRegion(CSpriteFactoryHolder* src, i32 a, RECT* rc, i32 d, i32 e, i32 f) {
    if (!src) {
        return 0;
    }
    m_page = src;
    m_4 = a;
    m_wrapFlag = f;
    m_18 = d;
    m_1c = e;
    m_activeNode = 0;
    if (rc) {
        CopyRect(&m_rect8, rc);
        return 1;
    }
    m_rect8.left = 0;
    m_rect8.top = 0;
    m_rect8.right = src->m_drawTarget->m_10->m_width - 1;
    m_rect8.bottom = src->m_drawTarget->m_10->m_height - 1;
    return 1;
}

SIZE_UNKNOWN(CGameRegistry);

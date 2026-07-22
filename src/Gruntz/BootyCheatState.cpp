#include <Bute/ButeMgr.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Bute/SymParser.h> // canonical CSymParser (pulls CSymTab)

#include <rva.h>

#include <string.h> // inline strcpy intrinsic (/O2) for the cheat-table copy
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // CDDrawWorkerRegistry (InstallTree)
#include <Gruntz/Sprite.h>                // CDDrawWorker (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (fold: ex ResMgr.h CDrawTarget)    // canonical CDDrawWorkerRegistry (the +0x10 image registrar)
#include <Gruntz/GameMode.h>           // the REAL owner: CBootyState (0x18830 IS its vtable slot 1)
#include <Gruntz/GruntzMgr.h>          // CState::m_4 is CGruntzMgr (RestoreVideoMode @0x8ddd0)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree)
#include <DDrawMgr/DDrawChildGroup.h>     // CDDrawChildGroup - holder+0x08 (DestroyChildren_159ef0)
#include <Gruntz/BootyCheatState.h> // own exported globals (ex Globals.h)

char g_cheatTable[0xfa0]; // 0x629f50  (25 entries x 0xa0 stride)
char g_cheatTableEnd[4];  // 0x62aef0  (loop end sentinel = &g_cheatTable[0xfa0])
DATA(0x0022af10)
i32 g_bootyCheatBuilt = 0; // 0x22af10

// @source: string-xref
// @early-stop
// epilogue tail-merge layout wall (~86%): logic complete + verified vs retail - the
// inline /GX prologue, the whole first-run cheat-table build (idiv %3 + reciprocal
// /3, the 5-CString EH frame, the Format/GetIntDef/GetStringDef/op= chain, the
// inline-strcpy rep-movs, the signed `jl` loop bound), the STATEZ_BOOTY/GAME/GRUNTZ
// registration, the sound/image installs, the ShowCursor cached-ptr loop, the Pump,
// and the 5-stage Init chain (`test;je` guards) all byte-match. Residual: retail
// routes ALL 12 returns to ONE shared epilogue, emitting per-site `jne skip; xor
// eax,eax; jmp <epilogue>` for the 6 pointer-null guards (flat `if(!x)return 0` with
// a cross-jumped epilogue tail); flat source here does NOT cross-jump (MSVC inlines
// 12 full epilogues, 54%), and the `goto fail` shared-exit that recovers the merge
// emits `je fail` (86%) not the retail per-site xor+jmp. The merge-vs-inline choice
// is the allocator's block-layout decision, not source-steerable. See
// docs/patterns/identical-return-epilogue-tailmerge.md (reverse direction).
RVA(0x00018830, 0x380)
i32 CBootyState::LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) {
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if (!CState::LoadGameAssetNamespaces(a1, a2, a3)) {
        goto fail;
    }

    if (g_bootyCheatBuilt == 0) {
        CString bootyCheatz("BootyCheatz");
        CString empty(g_emptyString);
        CString grp;
        CString text;
        CString desc;
        i32 i = 0;
        for (char* p = g_cheatTable; reinterpret_cast<i32>(p) < reinterpret_cast<i32>(g_cheatTableEnd); p += 0xa0) {
            grp.Format("A%dC%d", i / 3 + 1, i % 3 + 1);
            i32 id = g_buteMgr.GetIntDef(bootyCheatz, grp, 1);
            grp.Format("Cheat%i", id);
            text = *g_buteMgr.GetStringDef(grp, "Text", &empty);
            desc = *g_buteMgr.GetStringDef(grp, "Desc", &empty);
            strcpy(p - 0x20, text);
            strcpy(p, desc);
            i++;
        }
        g_bootyCheatBuilt = 1;
    }

    m_mgr->RestoreVideoMode(0); // thunk 0x34ef -> CGruntzMgr::RestoreVideoMode (0x8ddd0)

    m_2c = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_BOOTY"));
    if (!m_2c) {
        goto fail;
    }
    m_gameBank = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME"));
    if (!m_gameBank) {
        goto fail;
    }
    m_gruntzBank = static_cast<CSymTab*>(m_symParser->ResolvePath("GRUNTZ"));
    if (!m_gruntzBank) {
        goto fail;
    }

    m_world->m_childGroup->DestroyChildren_159ef0();

    {
        void* soundz = SymTab2c()->FindSub("SOUNDZ");
        if (!soundz) {
            goto fail;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(soundz), "BOOTY", "_");

        void* wand = m_gruntzBank->ResolvePath("SOUNDZ_WANDGRUNT");
        if (!wand) {
            goto fail;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(wand), "GRUNTZ_WANDGRUNT", "_");

        void* imagez = SymTab2c()->FindSub("IMAGEZ");
        if (!imagez) {
            goto fail;
        }
        m_world->m_imageRegistry->InstallTree(imagez, "BOOTY", "_");
    }

    {
        int(WINAPI * sc)(BOOL) = ::ShowCursor;
        while (sc(0) >= 0) {
        }
    }

    // The cast is GONE - the binary settled it. This call was bound to
    // CMoviePlayer::Pump (0x17c790), which 0x18830 does not call at all: retail does
    //     mov eax,[esi+0x4]   ; this->m_4      (CGruntzMgr)
    //     push 0x40 / push 0x100
    //     mov ecx,[eax+0x4]   ; m_4->m_gameWnd (+0x04)
    //     call 0x13d4e0       ; ?PumpMessages@CGameWnd@@QAEXIH@Z
    // so the callee is CGameWnd::PumpMessages - the real method of the class that actually
    // sits at that offset, already reconstructed in the `gamewnd` unit. Wrong callee AND a
    // wrong cast; naming the true owner dissolved both.
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);

    m_1b8 = 0;
    // The five-stage build chain - all real, rva-bound methods of THIS class now (the
    // Init1..Init5 declared-only aliases they used to hide behind are gone).
    if (!BuildWarpStoneGlitterAnimation()) { // 0x19540
        goto fail;
    }
    if (!BuildGruntSprintAnimation()) { // 0x19920
        goto fail;
    }
    if (!LoadGruntEffectSprites()) { // 0x1a040
        goto fail;
    }
    if (!BuildBootyWalkingGruntz()) { // 0x1b450
        goto fail;
    }
    if (!BuildBootyPerfectAnimation()) { // 0x1c070
        goto fail;
    }

    m_1c8 = 0x21;
    m_1cc = 0;
    m_1c0 = g_frameTime;
    m_1c4 = 0;
    return 1;

fail:
    return 0;
}

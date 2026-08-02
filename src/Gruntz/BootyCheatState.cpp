#include <Bute/ButeMgr.h>
#include <Rez/FrameClock.h>
#include <Bute/SymParser.h>

#include <rva.h>
#include <AddrWord.h>

#include <string.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/Sprite.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/BootyCheatState.h>

char g_cheatTable[0xfa0];
char g_cheatTableEnd[4];
DATA(0x0022af10)
i32 g_bootyCheatBuilt = 0;

// @early-stop
RVA(0x00018830, 0x380)
i32 CBootyState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    i32 ok = 0;

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        goto done;
    }

    if (g_bootyCheatBuilt == 0) {
        CString bootyCheatz("BootyCheatz");
        CString empty(g_emptyString);
        CString grp;
        CString text;
        CString desc;
        i32 i = 0;

        AddrWord<char> cur;
        AddrWord<char> last;
        last.m_addr = g_cheatTableEnd;
        for (char* p = g_cheatTable; (cur.m_addr = p, cur.m_word) < last.m_word; p += 0xa0) {
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

    m_mgr->RestoreVideoMode(0);

    m_stateBank = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_BOOTY"));
    if (!m_stateBank) {
        goto done;
    }
    m_gameBank = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME"));
    if (!m_gameBank) {
        goto done;
    }
    m_gruntzBank = static_cast<CSymTab*>(m_symParser->ResolvePath("GRUNTZ"));
    if (!m_gruntzBank) {
        goto done;
    }

    m_world->m_childGroup->ClearChildren();

    {
        void* soundz = SymTab2c()->FindSub("SOUNDZ");
        if (!soundz) {
            goto done;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(soundz), "BOOTY", "_");

        void* wand = m_gruntzBank->ResolvePath("SOUNDZ_WANDGRUNT");
        if (!wand) {
            goto done;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(wand), "GRUNTZ_WANDGRUNT", "_");

        void* imagez = SymTab2c()->FindSub("IMAGEZ");
        if (!imagez) {
            goto done;
        }
        m_world->m_imageRegistry->InstallTree(imagez, "BOOTY", "_");
    }

    {
        int(WINAPI * sc)(BOOL) = ::ShowCursor;
        while (sc(0) >= 0) {
        }
    }

    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);

    m_secretHudHandled = 0;

    if (!BuildWarpStoneGlitterAnimation()) {
        goto done;
    }
    if (!BuildGruntSprintAnimation()) {
        goto done;
    }
    if (!LoadGruntEffectSprites()) {
        goto done;
    }
    if (!BuildBootyWalkingGruntz()) {
        goto done;
    }
    if (!BuildBootyPerfectAnimation()) {
        goto done;
    }

    m_frameIntervalLo = 0x21;
    m_frameIntervalHi = 0;
    m_frameStampLo = g_frameTime;
    m_frameStampHi = 0;
    ok = 1;

done:
    return ok;
}

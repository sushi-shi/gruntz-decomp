

// @early-stop

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GameMenuMgrBuilders.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SBI_MenuItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>

RVA(0x00101580, 0x806)
void CStatusBarMgr::BuildGameMenu() {
    CDDrawSurfaceMgr* code = m_world;
    i32 bx = m_rect10.left;
    i32 by = m_rect10.top;
    CSBI_Image* it;
    RECT r;

    if (m_itemKind != 0x1fb) {

        if (m_hitTestDisabled != 0 && g_gameReg->m_frameGate != 0) {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->SetupImage(
                    this,
                    code,
                    0x1f4,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_RESUME",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return;
            }
            m_tabLists[5].AddTail(it);
        } else {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->SetupImage(
                    this,
                    code,
                    0x1f4,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_PAUSE",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return;
            }
            m_tabLists[5].AddTail(it);
        }
        m_tabSprite5 = static_cast<CSBI_MenuItem*>(it);

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x125;
        r.right = bx + 0x9f;
        r.bottom = by + 0x13c;
        if (!it->SetupImage(this, code, 0x1f5, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_LOAD", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite6 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_gameMode == 2) {
            it->m_enabled = 0;
        }

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0xfd;
        r.right = bx + 0x9f;
        r.bottom = by + 0x114;
        if (!it->SetupImage(this, code, 0x1f6, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_SAVE", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite7 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_gameMode == 2) {
            it->m_enabled = 0;
        }

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x14d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x164;
        if (!it->SetupImage(
                this,
                code,
                0x1f7,
                5,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_SETTINGS",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite8 = static_cast<CSBI_MenuItem*>(it);

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x175;
        r.right = bx + 0x9f;
        r.bottom = by + 0x18c;
        if (!it->SetupImage(this, code, 0x1f8, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_HELP", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite9 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_gameMode == 2) {
            it->m_enabled = 0;
        }

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x19d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x1b4;
        if (!it->SetupImage(this, code, 0x1f9, 5, r, "GAME_STATUSBAR_TABZ_GAMETAB_QUIT", -1, 0)) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite10 = static_cast<CSBI_MenuItem*>(it);

        it = new CSBI_ImageSet;
        r.left = bx + 0x22;
        r.top = by + 0x1be;
        r.right = bx + 0x7d;
        r.bottom = by + 0x1d6;
        if (!it->SetupImage(
                this,
                code,
                0x1fc,
                5,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
                m_modeState,
                0
            )) {
            if (it) {
                delete it;
            }
            return;
        }
        m_tabLists[5].AddTail(it);
        m_modeNotify = static_cast<CSBI_ImageSet*>(it);
        if (g_gameReg->m_gameMode != 1) {
            it->m_enabled = 0;
            m_modeState = 7;
            m_destructWarnActive = 0;
            m_modeNotify->Notify(7);
        }
        return;
    }

    it = new CSBI_ImageSet;
    i32 variant = (g_gameReg->m_cmdGrid->m_phase == 1) ? 1 : 2;
    r.left = bx;
    r.top = by + 0xd7;
    r.right = bx + 0x9f;
    r.bottom = by + 0xec;
    if (!it->SetupImage(
            this,
            code,
            0x1fb,
            5,
            r,
            "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
            variant,
            0
        )) {
        if (it) {
            delete it;
        }
        return;
    }
    m_tabLists[5].AddTail(it);
}

// ??0CSBI_RectOnly: BuildGameMenu's construction set is complete; retail keeps
// 5 base-ctor calls where our cl flattens the chains (variable per-site inline
// depth, see docs/patterns/msvc5-variable-ctor-inline-depth.md), so nothing
// emits this COMDAT and the pin dangles.
RVA_COMPGEN(0x00101fa0, 0x1b, ??0CSBI_RectOnly@@QAE@XZ)
RVA_COMPGEN(0x00101fd0, 0x1e, ??_GCSBI_ImageSet@@UAEPAXI@Z)
RVA_COMPGEN(0x00102000, 0x7f, ??1CSBI_ImageSet@@UAE@XZ)

#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/CurPlayer.h>
#include <Image/ImageSet.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>

#include <Gruntz/SbGeom.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/StatusBarMgrBuilders.h>

#include <Gruntz/TriggerMgr.h>

VTBL(CSBI_GruntMachine, 0x001eadbc);

// @early-stop
RVA(0x00102250, 0x1de4)
RVA_COMPGEN(0x00104cb0, 0x1e, ??_GCSBI_GruntMachine@@UAEPAXI@Z)
i32 CStatusBarMgr::LoadTabSprites() {
    CDDrawSurfaceMgr* code = m_world;
    i32 bx = m_rect10.left;
    i32 by = m_rect10.top;

    CSBI_Image* it;
    CSBI_ImageSetAni* ani;
    CSBI_StatzTabArrow* arrow;
    CSBI_GruntMachine* mach;
    CSBI_StatzTabGruntBar* bar;
    RECT r;
    i32 i;

    switch (m_activeTab) {
        case 2:
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
                    this,
                    code,
                    0x25c,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);

            {
                CSBI_ImageSet** aptr = m_slotNotify;
                i32* bptr = &m_slots[0].m_value;
                i32 y = by + 0xfe;
                for (i = 0; i < 5; i++) {
                    CSBI_ImageSet* set = new CSBI_ImageSet;
                    r.left = bx + 0xe;
                    r.top = y - 0x32;
                    r.right = bx + 0x39;
                    r.bottom = y;
                    if (!set->SetupImage(
                            this,
                            code,
                            0x64 + i,
                            2,
                            r,
                            "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                            *bptr,
                            0
                        )) {
                        if (set) {
                            delete set;
                        }
                        return 0;
                    }
                    m_tabLists[2].AddTail(set);
                    *aptr = set;
                    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(
                        g_gameReg->m_options[g_curPlayer].m_colorIndex,
                        0
                    );
                    if (sel == 0) {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                    }
                    (static_cast<CDDrawWorker*>(set->m_frameSet))->SetAllTypes(10);
                    (static_cast<CDDrawWorker*>(set->m_frameSet))->SetAllFormats(sel);
                    aptr++;
                    bptr += 6;
                    y += 0x36;
                }
            }
            it = new CSBI_Image;
            r.left = bx + 0x4c;
            r.top = by + 0xc8;
            r.right = bx + 0x97;
            r.bottom = by + 0x1cd;
            if (!it->SetupImage(
                    this,
                    code,
                    0x69,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELL",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);
            m_gaugeNotify = it;
            it = new CSBI_Image;
            r.left = bx + 0x1e;
            r.top = by + 0xc4;
            r.right = bx + 0x3d;
            r.bottom = by + 0xcd;
            if (!it->SetupImage(
                    this,
                    code,
                    0x6b,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_OVENZTEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);
            it = new CSBI_Image;
            r.left = bx + 0x68;
            r.top = by + 0x1cf;
            r.right = bx + 0x87;
            r.bottom = by + 0x1d8;
            if (!it->SetupImage(
                    this,
                    code,
                    0x6c,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLTEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);
            it = new CSBI_WellGoo;
            r.left = bx + 0x6e;
            r.top = by + 0xf8;
            r.right = bx + 0xef;
            r.bottom = by + 0x1b3;
            if (!it->SetupImage(
                    this,
                    code,
                    0x6a,
                    2,
                    r,
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO",
                    m_gauge,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[2].AddTail(it);
            m_gaugeSink = static_cast<CSBI_WellGoo*>(it);
            return 1;

        case 3:
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
                    this,
                    code,
                    0x25c,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            it = new CSBI_Image;
            r.left = bx;
            r.top = by + 0x135;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1be;
            if (!it->SetupImage(
                    this,
                    code,
                    0xc8,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MAINBACKGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_notify0 = it;
            it = new CSBI_Image;
            r.left = bx;
            r.top = by + 0xfb;
            r.right = bx + 0x9f;
            r.bottom = by + 0x134;
            if (!it->SetupImage(
                    this,
                    code,
                    0xc9,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_UPPERBACKGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_notify2 = it;
            it = new CSBI_Image;
            r.left = bx + 0x48;
            r.top = by + 0xd3;
            r.right = bx + 0x67;
            r.bottom = by + 0xf3;
            if (!it->SetupImage(
                    this,
                    code,
                    0xca,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_WINDOWBACKGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_notify3 = it;

            it = new CSBI_ImageSet;
            r.left = bx + 0x19;
            r.top = by + 0x11c;
            r.right = bx + 0x3c;
            r.bottom = by + 0x130;
            if (!it->SetupImage(
                    this,
                    code,
                    0xcb,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_groupSlots[0].m_value,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_groupNotify[0] = static_cast<CSBI_ImageSet*>(it);
            it = new CSBI_ImageSet;
            r.left = bx + 0x40;
            r.top = by + 0x11c;
            r.right = bx + 0x63;
            r.bottom = by + 0x130;
            if (!it->SetupImage(
                    this,
                    code,
                    0xcc,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_groupSlots[1].m_value,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_groupNotify[1] = static_cast<CSBI_ImageSet*>(it);
            it = new CSBI_ImageSet;
            r.left = bx + 0x68;
            r.top = by + 0x11c;
            r.right = bx + 0x8b;
            r.bottom = by + 0x130;
            if (!it->SetupImage(
                    this,
                    code,
                    0xcd,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_groupSlots[2].m_value,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_groupNotify[2] = static_cast<CSBI_ImageSet*>(it);

            it = new CSBI_ImageSet;
            r.left = m_itemRect.left + bx;
            r.top = m_itemRect.top + by;
            r.right = m_itemRect.right + bx;
            r.bottom = m_itemRect.bottom + by;
            if (!it->SetupImage(
                    this,
                    code,
                    0xdf,
                    3,
                    r,
                    "GAME_INGAMEICONZ_GREYCHIPZ",
                    m_extraNotifyArg0,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_extraNotify0 = static_cast<CSBI_ImageSet*>(it);
            it->m_enabled = 0;

            {
                i32* cfgp = &m_hlGrid[4].m_value;
                CSBI_ImageSet** cachep = &m_hlNotify[4];
                i32 y = by + 0x155;
                i32 c = 0xd7;
                for (i = 0; i < 4; i++) {
                    CSBI_ImageSet* set = new CSBI_ImageSet;
                    r.left = bx + 0x1d;
                    r.top = y - 0x17;
                    r.right = bx + 0x34;
                    r.bottom = y;
                    if (!set->SetupImage(
                            this,
                            code,
                            c - 4,
                            3,
                            r,
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[-24],
                            0
                        )) {
                        if (set) {
                            delete set;
                        }
                        return 0;
                    }
                    m_tabLists[3].AddTail(set);
                    cachep[-4] = set;
                    set = new CSBI_ImageSet;
                    r.left = bx + 0x45;
                    r.top = y - 0x17;
                    r.right = bx + 0x5c;
                    r.bottom = y;
                    if (!set->SetupImage(
                            this,
                            code,
                            c,
                            3,
                            r,
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[0],
                            0
                        )) {
                        if (set) {
                            delete set;
                        }
                        return 0;
                    }
                    m_tabLists[3].AddTail(set);
                    cachep[0] = set;
                    set = new CSBI_ImageSet;
                    r.left = bx + 0x6d;
                    r.top = y - 0x17;
                    r.right = bx + 0x84;
                    r.bottom = y;
                    if (!set->SetupImage(
                            this,
                            code,
                            c + 4,
                            3,
                            r,
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[24],
                            0
                        )) {
                        if (set) {
                            delete set;
                        }
                        return 0;
                    }
                    m_tabLists[3].AddTail(set);
                    cachep[4] = set;
                    cfgp += 6;
                    cachep += 1;
                    y += 0x20;
                    c++;
                }
            }

            mach = new CSBI_GruntMachine;
            r.left = bx;
            r.top = by + 0xc8;
            r.right = bx + 0x9f;
            r.bottom = by + 0xfa;
            if (!mach->BuildResourceTabStatusBar(
                    this,
                    code,
                    0xd1,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND",
                    m_machineA.m_counter,
                    m_machineB.m_counter
                )) {
                if (mach) {
                    delete mach;
                }
                return 0;
            }
            m_machineDisplay = mach;
            m_tabLists[3].AddTail(mach);

            it = new CSBI_Image;
            r.left = bx;
            r.top = by + 0x1a6;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1df;
            if (!it->SetupImage(
                    this,
                    code,
                    0xd2,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEFOREGROUND",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_notify1 = it;

            ani = new CSBI_ImageSetAni;
            r.left = bx;
            r.top = by + 0x1bf;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1cc;
            if (!ani->Init(
                    this,
                    code,
                    0xce,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_CONVEYORTOP",
                    -1,
                    -1,
                    0x64,
                    1,
                    1
                )) {
                if (ani) {
                    delete ani;
                }
                return 0;
            }
            m_tabLists[3].AddTail(ani);

            it = new CSBI_ImageSet;
            r.left = m_fallRect.left + bx;
            r.top = m_fallRect.top + by;
            r.right = m_fallRect.right + bx;
            r.bottom = m_fallRect.bottom + by;
            if (!it->SetupImage(
                    this,
                    code,
                    0xe0,
                    3,
                    r,
                    "GAME_INGAMEICONZ_NORMCHIPZ",
                    m_extraNotifyArg1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[3].AddTail(it);
            m_extraNotify1 = static_cast<CSBI_ImageSet*>(it);
            it->m_enabled = 0;

            ani = new CSBI_ImageSetAni;
            r.left = bx;
            r.top = by + 0x1c7;
            r.right = bx + 0x9f;
            r.bottom = by + 0x1df;
            if (!ani->Init(
                    this,
                    code,
                    0xd0,
                    3,
                    r,
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_CONVEYORBOTTOM",
                    -1,
                    -1,
                    0x64,
                    1,
                    1
                )) {
                if (ani) {
                    delete ani;
                }
                return 0;
            }
            m_tabLists[3].AddTail(ani);
            return 1;

        case 4:
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
                    this,
                    code,
                    0x25c,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);

            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0xcf;
            r.right = bx + 0x8e;
            r.bottom = by + 0x10a;
            if (!it->SetupImage(
                    this,
                    code,
                    0x190,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD1",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_warlordHead[0] = static_cast<CSBI_WarlordHead*>(it);
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0x112;
            r.right = bx + 0x8e;
            r.bottom = by + 0x14d;
            if (!it->SetupImage(
                    this,
                    code,
                    0x191,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD2",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_warlordHead[1] = static_cast<CSBI_WarlordHead*>(it);
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0x155;
            r.right = bx + 0x8e;
            r.bottom = by + 0x190;
            if (!it->SetupImage(
                    this,
                    code,
                    0x192,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD3",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_warlordHead[2] = static_cast<CSBI_WarlordHead*>(it);
            it = new CSBI_WarlordHead;
            r.left = bx + 0x53;
            r.top = by + 0x197;
            r.right = bx + 0x8e;
            r.bottom = by + 0x1d2;
            if (!it->SetupImage(
                    this,
                    code,
                    0x193,
                    4,
                    r,
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD4",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[4].AddTail(it);
            m_warlordHead[3] = static_cast<CSBI_WarlordHead*>(it);

            {
                CSBI_WarlordHead** slot = m_warlordHead;
                i32 pi = 0;
                GruntzPlayer* p = g_gameReg->m_options;
                do {
                    CShadeTable* sel;
                    if (p->m_joined != 0 && p->m_doneFlag == 0) {
                        sel = g_gameReg->m_spriteFactory->GetSel(p->m_colorIndex, 0);
                        if (pi == m_tabCycle) {
                            (*slot)->SetState(1);
                        }
                    } else {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                        (*slot)->SetState(2);
                    }

                    (*slot)->ShowFrames(0xa, sel);
                    slot++;
                    pi++;
                    p++;
                } while (p < g_gameReg->m_options + 4);
            }

            {
                i32 by17 = bx + 0x17;
                i32 by52 = bx + 0x52;
                i32 y = by + 0xd9;
                for (i = 0; i < 15; i++) {
                    bar = new CSBI_StatzTabGruntBar;
                    r.left = by17;
                    r.top = y - 0x11;
                    r.right = by52;
                    r.bottom = y;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            0x13b + i,
                            4,
                            r,
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            m_tabCycle,
                            i,
                            0
                        )) {
                        if (bar) {
                            delete bar;
                        }
                        return 0;
                    }
                    m_tabLists[4].AddTail(bar);
                    y += 0x12;
                }
            }
            return 1;

        case 1:
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
                    this,
                    code,
                    0x25c,
                    1,
                    r,
                    "GAME_STATUSBAR_TABZ_STATZTAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[1].AddTail(it);

            {
                i32 aOff, cOff;
                if (m_position == 1) {
                    aOff = 0x7d;
                    cOff = 0x95;
                } else {
                    aOff = 0xa;
                    cOff = 0x21;
                }
                i32 arrowL = bx + aOff;
                i32 arrowR = bx + cOff;
                i32 y = by + 0xd9;
                for (i = 0; i < 15; i++) {
                    i32 id = 0x13b + i;
                    arrow = new CSBI_StatzTabArrow;
                    r.left = arrowL;
                    r.top = y - 0x11;
                    r.right = arrowR;
                    r.bottom = y;
                    if (!arrow->Init(
                            this,
                            code,
                            id - 0xf,
                            1,
                            r,
                            "GAME_STATUSBAR_TABZ_STATZTAB_ARROW",
                            -1,
                            -1,
                            0x64,
                            0,
                            0
                        )) {
                        if (arrow) {
                            delete arrow;
                        }
                        return 0;
                    }
                    m_tabLists[1].AddTail(arrow);
                    m_statObj[i] = arrow;
                    if (m_statFlags[i] != 0) {
                    } else {
                        arrow->SetDirection(m_position, 0);
                    }
                    bar = new CSBI_StatzTabGruntBar;
                    r.left = bx + 0x28;
                    r.top = y - 0x11;
                    r.right = bx + 0x77;
                    r.bottom = y;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            id,
                            1,
                            r,
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            g_curPlayer,
                            i,
                            1
                        )) {
                        if (bar) {
                            delete bar;
                        }
                        return 0;
                    }
                    m_tabLists[1].AddTail(bar);
                    y += 0x12;
                }
            }
            return 1;

        case 5:
            it = new CSBI_Image;
            r.left = bx + 0x18;
            r.top = by + 0xaf;
            r.right = bx + 0x70;
            r.bottom = by + 0xbe;
            if (!it->SetupImage(
                    this,
                    code,
                    0x25c,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_TITLETEXT",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[5].AddTail(it);

            it = new CSBI_ImageSet;
            r.left = bx;
            r.top = by;
            r.right = bx + 0x9f;
            r.bottom = by + 0x7f;
            if (!it->SetupImage(
                    this,
                    code,
                    0x2bc,
                    5,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                    1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[5].AddTail(it);
            if ((static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->ByteTableHas(1)) {
                it = new CSBI_ImageSet;
                r.left = bx + 0x17;
                r.top = by + 0xe;
                r.right = bx + 0x52;
                r.bottom = by + 0x44;
                if (!it->SetupImage(
                        this,
                        code,
                        0x2bd,
                        5,
                        r,
                        "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                        2,
                        0
                    )) {
                    if (it) {
                        delete it;
                    }
                    return 0;
                }
                m_tabLists[5].AddTail(it);
                if ((static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->ByteTableHas(2)) {
                    it = new CSBI_ImageSet;
                    r.left = bx + 0x4c;
                    r.top = by + 0xf;
                    r.right = bx + 0x87;
                    r.bottom = by + 0x3e;
                    if (!it->SetupImage(
                            this,
                            code,
                            0x2be,
                            5,
                            r,
                            "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                            3,
                            0
                        )) {
                        if (it) {
                            delete it;
                        }
                        return 0;
                    }
                    m_tabLists[5].AddTail(it);
                    if ((static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->ByteTableHas(3)) {
                        it = new CSBI_ImageSet;
                        r.left = bx + 0x1b;
                        r.top = by + 0x3b;
                        r.right = bx + 0x52;
                        r.bottom = by + 0x71;
                        if (!it->SetupImage(
                                this,
                                code,
                                0x2bf,
                                5,
                                r,
                                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                4,
                                0
                            )) {
                            if (it) {
                                delete it;
                            }
                            return 0;
                        }
                        m_tabLists[5].AddTail(it);
                        if ((static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->ByteTableHas(4)) {
                            it = new CSBI_ImageSet;
                            r.left = bx + 0x4a;
                            r.top = by + 0x35;
                            r.right = bx + 0x89;
                            r.bottom = by + 0x74;
                            if (!it->SetupImage(
                                    this,
                                    code,
                                    0x2c0,
                                    5,
                                    r,
                                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                    5,
                                    0
                                )) {
                                if (it) {
                                    delete it;
                                }
                                return 0;
                            }
                            m_tabLists[5].AddTail(it);
                        }
                    }
                }
            }
            BuildGameMenu();
            return 1;
    }
    return 1;
}

#include <Mfc.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Grunt.h>
#include <rva.h>
#include <Ints.h>

#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/Sprite.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/StatusBarItem.h>
#include <Image/CImage.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_SideTab.h>
#include <Gruntz/SbiSideTabBuildViews.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>

#include <Gruntz/StatusBarTabBuildersViews.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Gruntz/SerialCounter.h>
#include <string.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>

namespace StatusBarTabBuilders {}

// @early-stop
RVA(0x000e8a70, 0x18c)
i32 CSBI_GruntMachine::BuildResourceTabStatusBar(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT g,
    const char* key,
    i32 idxA,
    i32 idxB
) {

    if (host == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* h = host;
    m_owner = owner;
    m_tab = tab;
    m_host = h;
    m_redrawFrames = 0;
    m_enabled = 1;

    m_rect14 = g;

    CObject* found = 0;
    m_cmd = cmd;
    h->m_imageRegistry->m_10map.Lookup("GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND", found);
    CDDrawWorker* rec = static_cast<CDDrawWorker*>(found);
    CImage* spr;
    if (rec == 0 || rec->m_minIndex > 1 || rec->m_maxIndex < 1) {
        spr = 0;
    } else {
        spr = static_cast<CImage*>(rec->m_items.GetAt(1));
    }
    m_standaloneFrame = spr;
    if (spr == 0) {
        return 0;
    }
    found = 0;
    m_host->m_imageRegistry->m_10map.Lookup(key, found);
    CDDrawWorker* cfg = static_cast<CDDrawWorker*>(found);
    m_config = cfg;
    if (cfg == 0) {
        return 0;
    }
    m_frameIdxA = idxA;
    m_frameIdxB = idxB;
    CImage* s;
    if (idxA < m_config->m_minIndex || idxA > m_config->m_maxIndex) {
        s = 0;
    } else {
        s = static_cast<CImage*>(m_config->m_items.GetAt(idxA));
    }
    m_frameA = s;
    if (s == 0) {
        return 0;
    }
    CShadeTable* sel =
        g_gameReg->m_spriteFactory->GetSel(g_gameReg->m_options[g_curPlayer].m_colorIndex, 0);
    if (sel == 0) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    m_config->SetAllTypes(10);
    m_config->SetAllFormats(sel);
    CImage* val;
    if (m_frameIdxB < m_config->m_minIndex || m_frameIdxB > m_config->m_maxIndex) {
        val = 0;
    } else {
        val = static_cast<CImage*>(m_config->m_items.GetAt(m_frameIdxB));
    }
    m_frameB = val;
    return val != 0;
}

RVA(0x000e8c70, 0xc)
void CSBI_GruntMachine::Reset() {
    m_frameA = 0;
    m_frameB = 0;
    m_config = 0;
}

RVA(0x000e8c90, 0x8)
i32 CSBI_GruntMachine::Refresh(i32) {
    return 1;
}

RVA(0x000e8cb0, 0xc4)
i32 CSBI_GruntMachine::Render() {
    if (m_redrawFrames > 0) {
        i32 idx = m_frameIdxA;
        m_redrawFrames--;
        CDDrawWorker* cfg = m_config;

        m_frameA = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex)
                       ? 0
                       : static_cast<CImage*>(cfg->m_items.GetAt(idx));
        idx = m_frameIdxB;
        m_frameB = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex)
                       ? 0
                       : static_cast<CImage*>(cfg->m_items.GetAt(idx));

        CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;

        CImage* f = m_standaloneFrame;
        if (f) {
            f->RenderFrame(ctx, m_rect14.left + f->m_anchorX, m_rect14.top + f->m_anchorY, 0);
        }
        f = m_frameB;
        if (f) {
            f->RenderFrame(
                ctx,
                m_rect14.left + f->m_anchorX + 0x2c,
                m_rect14.top + f->m_anchorY,
                0
            );
        }
        f = m_frameA;
        if (f) {
            f->RenderFrame(ctx, m_rect14.left + f->m_anchorX, m_rect14.top + f->m_anchorY, 0);
        }
    }
    return 1;
}

RVA(0x000e8dc0, 0x22)
void CSBI_GruntMachine::SetFrames(i32 idxA, i32 idxB) {
    if (idxA != -1) {
        m_frameIdxA = idxA;
    }
    if (idxB != -1) {
        m_frameIdxB = idxB;
    }
    m_redrawFrames = 2;
}

// @early-stop
RVA(0x000e8e00, 0x41a)
i32 CSBI_GruntMachine::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 4: {
            i32 v;

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_config != 0) {
                strcpy(buf, m_config->m_name);
            }
            s->Write(buf, 0x80);
            s->Write(&m_frameIdxA, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_frameA != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_frameA, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);
            s->Write(&m_frameIdxB, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_frameB != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_frameB, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_standaloneFrame != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_standaloneFrame, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);
            break;
        }

        case 7: {
            CObject* out;
            i32 idx;

            g_serialCounter++;
            s->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                m_config = static_cast<CDDrawWorker*>(out);
            } else {
                m_config = 0;
            }
            s->Read(&m_frameIdxA, 4);

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_frameA = r;
            } else {
                m_frameA = 0;
            }
            s->Read(&m_frameIdxB, 4);

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_frameB = r;
            } else {
                m_frameB = 0;
            }

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_standaloneFrame = r;
            } else {
                m_standaloneFrame = 0;
            }
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, pObj) != 0 ? 1 : 0;
}

namespace StatusBarTabBuilders {}

// @early-stop
RVA(0x000e9600, 0x18c)
i32 CSBI_SideTab::BuildStatzTabStatusBar(
    CStatusBarMgr* parent,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    i32 left,
    i32 top,
    i32 right,
    i32 bottom,
    const char* unused,
    i32 rowIndex,
    i32 colIndex,
    i32 enabled,
    i32 onLeft
) {
    static_cast<void>(unused);
    if (host == 0 || parent == 0) {
        return 0;
    }
    m_host = host;
    m_tab = tab;
    m_owner = parent;
    m_rect14.left = left;
    m_redrawFrames = 0;
    m_rect14.top = top;
    m_rect14.right = right;
    m_rect14.bottom = bottom;
    m_cmd = cmd;

    if (enabled != 0) {
        m_enabled = 1;
    } else {
        m_enabled = 0;
    }
    m_rowIndex = rowIndex;
    m_colIndex = colIndex;
    m_onLeft = onLeft;

    if (onLeft != 0) {
        CDDrawWorker* n = 0;
        CObject* nOb = 0;
        g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT",
            nOb
        );
        n = static_cast<CDDrawWorker*>(nOb);
        CImage* v;
        if (n == 0) {
            v = 0;
        } else if (n->m_minIndex > 1 || n->m_maxIndex < 1) {
            v = 0;
        } else {
            v = static_cast<CImage*>(n->m_items.GetAt(1));
        }
        m_topFrame = v;
        m_bottomFrameDy = 1;
        m_drawPosition.m_x = parent->m_rect10.left - (right - left) / 2;
    } else {
        CDDrawWorker* n = 0;
        CObject* nOb = 0;
        g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT",
            nOb
        );
        n = static_cast<CDDrawWorker*>(nOb);
        CImage* v;
        if (n == 0) {
            v = 0;
        } else if (n->m_minIndex > 1 || n->m_maxIndex < 1) {
            v = 0;
        } else {
            v = static_cast<CImage*>(n->m_items.GetAt(1));
        }
        m_topFrame = v;
        m_bottomFrameDy = -1;
        m_drawPosition.m_x = (right - left) / 2 + parent->m_rect10.right;
    }
    m_drawPosition.m_y = colIndex * 0x12 + 0xd1;
    if (m_topFrame == 0) {
        return 0;
    }
    m_sampleMode = enabled;
    m_sampledValue = -1;
    m_drawGate = BuildHandle();
    return 1;
}

RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_topFrame = 0;
    m_bottomFrame = 0;
}

RVA(0x000e9820, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_drawGate = BuildHandle();
    return 0;
}

// @early-stop
RVA(0x000e9850, 0x111)
i32 CSBI_SideTab::BuildHandle() {
    i32 mode = m_sampleMode;
    if (mode == 0) {
        return 0;
    }
    CGrunt* unit = g_gameReg->m_cmdGrid->m_grid[m_colIndex + 15 * m_rowIndex];
    if (unit == 0) {
        m_owner->ClearStat(m_colIndex);
        return 0;
    }
    i32 val;
    if (mode == 2) {
        i32 level = unit->m_entranceReason;
        if (level > 0x16) {
            val = unit->m_toolId;
            if (val == 0) {
                m_sampleMode = 1;
            }
        } else {
            val = level;
            if (val == 0) {
                m_sampleMode = 1;
            }
        }
    } else if (mode == 3) {
        val = unit->m_vehiclePickupType;
        if (val == 0) {
            m_sampleMode = 1;
        }
    }
    if (m_sampleMode == 1) {
        i32 hp = unit->m_health;
        if (hp >= 0x50) {
            val = 0x24;
        } else if (hp >= 0x28) {
            val = 0x25;
        } else {
            val = (hp <= 0 ? 1 : 0) + 0x26;
        }
    }
    if (m_sampledValue == val) {
        return 1;
    }
    CObject* gm_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
        "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
        gm_ob
    );
    CDDrawWorker* gm = static_cast<CDDrawWorker*>(gm_ob);
    CImage* glyph;
    if (gm == 0 || val < gm->m_minIndex || val > gm->m_maxIndex) {
        glyph = 0;
    } else {
        glyph = static_cast<CImage*>(gm->m_items.GetAt(val));
    }
    m_sampledValue = val;
    m_bottomFrame = glyph;
    return 1;
}

RVA(0x000e99c0, 0x4c)
i32 CSBI_SideTab::Render() {
    if (m_drawGate) {
        CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
        m_topFrame->RenderFrame(ctx, m_drawPosition.m_x, m_drawPosition.m_y, 0);
        m_bottomFrame
            ->RenderFrame(ctx, m_drawPosition.m_x + m_bottomFrameDy, m_drawPosition.m_y, 0);
    }
    return 1;
}

RVA(0x000e9a30, 0x31e)
i32 CSBI_SideTab::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 4: {
            i32 v;

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_topFrame != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_topFrame, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_bottomFrame != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_bottomFrame, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);

            s->Write(&m_sampledValue, 4);
            s->Write(&m_rowIndex, 4);
            s->Write(&m_colIndex, 4);
            s->Write(&m_sampleMode, 4);
            s->Write(&m_drawPosition, 8);
            s->Write(&m_bottomFrameDy, 4);
            s->Write(&m_onLeft, 4);
            s->Write(&m_drawGate, 4);
            break;
        }

        case 7: {
            CObject* out;
            i32 idx;

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_topFrame = r;
            } else {
                m_topFrame = 0;
            }

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_bottomFrame = r;
            } else {
                m_bottomFrame = 0;
            }

            s->Read(&m_sampledValue, 4);
            s->Read(&m_rowIndex, 4);
            s->Read(&m_colIndex, 4);
            s->Read(&m_sampleMode, 4);
            s->Read(&m_drawPosition, 8);
            s->Read(&m_bottomFrameDy, 4);
            s->Read(&m_onLeft, 4);
            s->Read(&m_drawGate, 4);
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, pObj) != 0 ? 1 : 0;
}

RVA(0x000ea0f0, 0x5c)
void CSBI_StatzTabArrow::SetDirection(i32 position, i32 animate) {
    if (position == 0) {
        if (animate == 0) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    } else {
        if (animate == 0) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    }
}

RVA(0x000ea170, 0x5c)
void CSBI_StatzTabArrow::SetDirectionAlt(i32 position, i32 animate) {
    if (position == 0) {
        if (animate == 0) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    } else {
        if (animate == 0) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    }
}

// @early-stop
RVA(0x000ea1f0, 0x1fa)
i32 CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT g,
    const char* key,
    i32 unitRow,
    i32 unitCol,
    i32 selMode
) {

    if (host == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* h = host;
    m_owner = owner;
    m_tab = tab;
    m_host = h;
    m_redrawFrames = 0;
    m_enabled = 1;

    m_rect14 = g;

    CObject* found = 0;
    m_cmd = cmd;
    h->m_imageRegistry->m_10map.Lookup(key, found);
    CDDrawWorker* head = static_cast<CDDrawWorker*>(found);
    m_glyphMap = head;
    if (head == 0) {
        return 0;
    }
    CImage* v;
    if (head->m_minIndex > 0x21 || head->m_maxIndex < 0x21) {
        v = 0;
    } else {
        v = static_cast<CImage*>(head->m_items.GetAt(0x21));
    }
    m_statusGlyph = v;
    if (v == 0) {
        return 0;
    }
    CImage* w;
    if (head->m_minIndex > 0x22 || head->m_maxIndex < 0x22) {
        w = 0;
    } else {
        w = static_cast<CImage*>(head->m_items.GetAt(0x22));
    }
    m_abilityGlyph = w;
    if (w == 0) {
        return 0;
    }

    CImage* val;
    if (selMode != 0) {
        found = 0;
        m_host->m_imageRegistry->m_10map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR", found);
        CDDrawWorker* sel = static_cast<CDDrawWorker*>(found);
        m_timerGlyphMap = sel;
        if (sel == 0) {
            return 0;
        }
        CImage* x;
        if (m_glyphMap->m_minIndex > 0x23 || m_glyphMap->m_maxIndex < 0x23) {
            x = 0;
        } else {
            x = static_cast<CImage*>(m_glyphMap->m_items.GetAt(0x23));
        }
        m_selectKey = x;
        if (x == 0) {
            return 0;
        }
        if (m_glyphMap->m_minIndex > 0x22 || m_glyphMap->m_maxIndex < 0x22) {
            val = 0;
        } else {
            val = static_cast<CImage*>(m_glyphMap->m_items.GetAt(0x22));
        }
    } else {
        found = 0;
        m_host->m_imageRegistry->m_10map.Lookup(
            "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR",
            found
        );
        CDDrawWorker* sel = static_cast<CDDrawWorker*>(found);
        m_timerGlyphMap = sel;
        if (sel == 0) {
            return 0;
        }
        if (m_glyphMap->m_minIndex > 0x23 || m_glyphMap->m_maxIndex < 0x23) {
            val = 0;
        } else {
            val = static_cast<CImage*>(m_glyphMap->m_items.GetAt(0x23));
        }
    }
    m_overrideGlyph = val;
    if (val == 0) {
        return 0;
    }
    m_unitRow = unitRow;
    m_unitCol = unitCol;
    m_timerValue = -1;
    m_overrideValue = -1;
    m_abilityValue = -1;
    m_statusValue = -1;
    m_selectValue = 0;
    m_timerAnchorLo = 0;
    m_timerWindowLo = 0;
    m_timerAnchorHi = 0;
    m_timerWindowHi = 0;
    Update();
    return 1;
}

RVA(0x00104ce0, 0x55)
CSBI_GruntMachine::~CSBI_GruntMachine() {
    Reset();
}

RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}

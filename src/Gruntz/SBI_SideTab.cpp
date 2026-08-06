#include <rva.h>

#include <Gruntz/SBI_SideTab.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

// @early-stop
RVA(0x000e9600, 0x18c)
i32 CSBI_SideTab::BuildStatzTabStatusBar(
    CStatusBarMgr* parent,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* unused,
    i32 rowIndex,
    i32 colIndex,
    StatusSampleMode enabled,
    i32 onLeft
) {
    static_cast<void>(unused);
    if (host == NULL || parent == NULL) {
        return 0;
    }
    m_host = host;
    m_tab = tab;
    m_owner = parent;
    m_rect14.left = rc.left;
    m_redrawFrames = 0;
    m_rect14.top = rc.top;
    m_rect14.right = rc.right;
    m_rect14.bottom = rc.bottom;
    m_cmd = cmd;

    if (enabled != STATUS_SAMPLE_NONE) {
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
        if (n == NULL) {
            v = NULL;
        } else if (n->m_minIndex > 1 || n->m_maxIndex < 1) {
            v = NULL;
        } else {
            v = static_cast<CImage*>(n->m_items.GetAt(1));
        }
        m_topFrame = v;
        m_bottomFrameDy = 1;
        m_drawPosition.m_x = parent->m_rect10.left - (rc.right - rc.left) / 2;
    } else {
        CDDrawWorker* n = 0;
        CObject* nOb = 0;
        g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT",
            nOb
        );
        n = static_cast<CDDrawWorker*>(nOb);
        CImage* v;
        if (n == NULL) {
            v = NULL;
        } else if (n->m_minIndex > 1 || n->m_maxIndex < 1) {
            v = NULL;
        } else {
            v = static_cast<CImage*>(n->m_items.GetAt(1));
        }
        m_topFrame = v;
        m_bottomFrameDy = -1;
        m_drawPosition.m_x = (rc.right - rc.left) / 2 + parent->m_rect10.right;
    }
    m_drawPosition.m_y = colIndex * 0x12 + 0xd1;
    if (m_topFrame == NULL) {
        return 0;
    }
    m_sampleMode = enabled;
    m_sampledValue = -1;
    m_drawGate = BuildHandle();
    return 1;
}

RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_topFrame = NULL;
    m_bottomFrame = NULL;
}

RVA(0x000e9820, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_drawGate = BuildHandle();
    return 0;
}

// @early-stop
RVA(0x000e9850, 0x111)
i32 CSBI_SideTab::BuildHandle() {
    StatusSampleMode mode = m_sampleMode;
    if (mode == STATUS_SAMPLE_NONE) {
        return 0;
    }
    CGrunt* unit = g_gameReg->m_cmdGrid->m_grid[m_colIndex + 15 * m_rowIndex];
    if (unit == NULL) {
        m_owner->ClearStat(m_colIndex);
        return 0;
    }
    i32 val;
    if (mode == STATUS_SAMPLE_TOOL) {
        PickupType level = unit->m_entranceReason;
        if (level > PICKUP_EQUIPPABLE_LAST) {
            val = IDX(unit->m_toolId);
            if (unit->m_toolId == PICKUP_NONE) {
                m_sampleMode = STATUS_SAMPLE_HEALTH;
            }
        } else {
            val = IDX(level);
            if (level == PICKUP_NONE) {
                m_sampleMode = STATUS_SAMPLE_HEALTH;
            }
        }
    } else if (mode == STATUS_SAMPLE_VEHICLE) {
        val = IDX(unit->m_vehiclePickupType);
        if (unit->m_vehiclePickupType == PICKUP_NONE) {
            m_sampleMode = STATUS_SAMPLE_HEALTH;
        }
    }
    if (m_sampleMode == STATUS_SAMPLE_HEALTH) {
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
    if (gm == NULL || val < gm->m_minIndex || val > gm->m_maxIndex) {
        glyph = NULL;
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
i32 CSBI_SideTab::SerializeFields(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];

    switch (mode) {
        case SERIAL_SAVE: {
            i32 v;

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_topFrame != NULL) {
                reg->m_imageRegistry->AnyValueMatches(m_topFrame, buf, &v);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            s->Write(&v, sizeof(v));

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_bottomFrame != NULL) {
                reg->m_imageRegistry->AnyValueMatches(m_bottomFrame, buf, &v);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            s->Write(&v, sizeof(v));

            s->Write(&m_sampledValue, sizeof(m_sampledValue));
            s->Write(&m_rowIndex, sizeof(m_rowIndex));
            s->Write(&m_colIndex, sizeof(m_colIndex));
            s->Write(&m_sampleMode, sizeof(m_sampleMode));
            s->Write(&m_drawPosition, sizeof(m_drawPosition));
            s->Write(&m_bottomFrameDy, sizeof(m_bottomFrameDy));
            s->Write(&m_onLeft, sizeof(m_onLeft));
            s->Write(&m_drawGate, sizeof(m_drawGate));
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;
            i32 idx;

            g_serialCounter++;
            s->Read(buf, SERIAL_NAME_LEN);
            s->Read(&idx, sizeof(idx));
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = NULL;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != NULL && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = NULL;
                }
                m_topFrame = r;
            } else {
                m_topFrame = NULL;
            }

            g_serialCounter++;
            s->Read(buf, SERIAL_NAME_LEN);
            s->Read(&idx, sizeof(idx));
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = NULL;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != NULL && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = NULL;
                }
                m_bottomFrame = r;
            } else {
                m_bottomFrame = NULL;
            }

            s->Read(&m_sampledValue, sizeof(m_sampledValue));
            s->Read(&m_rowIndex, sizeof(m_rowIndex));
            s->Read(&m_colIndex, sizeof(m_colIndex));
            s->Read(&m_sampleMode, sizeof(m_sampleMode));
            s->Read(&m_drawPosition, sizeof(m_drawPosition));
            s->Read(&m_bottomFrameDy, sizeof(m_bottomFrameDy));
            s->Read(&m_onLeft, sizeof(m_onLeft));
            s->Read(&m_drawGate, sizeof(m_drawGate));
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, pObj) != 0 ? 1 : 0;
}

RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}

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

RVA(0x000e9730, 0x18c)
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
    if (host == NULL) {
        goto fail;
    }
    if (parent == NULL) {
        goto fail;
    }
    m_host = host;
    m_tab = tab;
    m_owner = parent;
    m_rect = rc;
    m_redrawFrames = 0;
    m_cmd = cmd;

    if (enabled != STATUS_SAMPLE_NONE) {
        SetEnabled(1);
    } else {
        SetEnabled(0);
    }
    m_rowIndex = rowIndex;
    m_colIndex = colIndex;
    m_onLeft = onLeft;

    if (onLeft != 0) {
        CDDrawWorker* worker;
        CObject* found = NULL;
        g_gameReg->m_world->m_imageRegistry->m_workersByName.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT",
            found
        );
        worker = static_cast<CDDrawWorker*>(found);
        CImage* frame;
        if (worker == NULL) {
            frame = NULL;
        } else if (DDRAW_WORKER_MISSES_FRAME(worker, 1)) {
            frame = NULL;
        } else {
            frame = DDRAW_WORKER_FRAME_AT_UNCHECKED(worker, 1);
        }
        m_topFrame = frame;
        m_drawPosition.m_x = parent->m_barRect.left - (rc.right - rc.left) / 2;
        m_bottomFrameDy = 1;
    } else {
        CDDrawWorker* worker;
        CObject* found = NULL;
        g_gameReg->m_world->m_imageRegistry->m_workersByName.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT",
            found
        );
        worker = static_cast<CDDrawWorker*>(found);
        CImage* frame;
        if (worker == NULL) {
            frame = NULL;
        } else if (DDRAW_WORKER_MISSES_FRAME(worker, 1)) {
            frame = NULL;
        } else {
            frame = DDRAW_WORKER_FRAME_AT_UNCHECKED(worker, 1);
        }
        m_topFrame = frame;
        m_drawPosition.m_x = (rc.right - rc.left) / 2 + parent->m_barRect.right;
        m_bottomFrameDy = -1;
    }
    m_drawPosition.m_y = colIndex * 0x12 + 0xd1;
    if (m_topFrame == NULL) {
        goto fail;
    }
    m_sampleMode = enabled;
    m_sampledValue = -1;
    m_drawGate = BuildHandle();
    return 1;
fail:
    return 0;
}

RVA(0x000e9930, 0x9)
void CSBI_SideTab::Reset() {
    m_topFrame = NULL;
    m_bottomFrame = NULL;
}

RVA(0x000e9950, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_drawGate = BuildHandle();
    return 0;
}

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

// @early-stop
RVA(0x000e9980, 0x111)
i32 CSBI_SideTab::BuildHandle() {
    StatusSampleMode mode = m_sampleMode;
    if (mode == STATUS_SAMPLE_NONE) {
        return 0;
    }
    CGrunt* unit = g_gameReg->m_triggerMgr->m_units[m_colIndex + TM_UNITS_PER_PLAYER * m_rowIndex];
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
    CDDrawWorker* gm = LookupWorker(
        g_gameReg->m_world->m_imageRegistry->m_workersByName,
        "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ"
    );
    CImage* glyph;
    if (gm == NULL || DDRAW_WORKER_FRAME_OUT_OF_RANGE(gm, val)) {
        glyph = NULL;
    } else {
        glyph = DDRAW_WORKER_FRAME_AT_UNCHECKED(gm, val);
    }
    m_sampledValue = val;
    m_bottomFrame = glyph;
    return 1;
}

RVA(0x000e9af0, 0x4c)
i32 CSBI_SideTab::Render() {
    if (m_drawGate) {
        CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
        m_topFrame->RenderFrame(ctx, m_drawPosition.m_x, m_drawPosition.m_y, 0);
        m_bottomFrame
            ->RenderFrame(ctx, m_drawPosition.m_x + m_bottomFrameDy, m_drawPosition.m_y, 0);
    }
    return 1;
}

RVA(0x000e9b60, 0x31e)
i32 CSBI_SideTab::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
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
                reg->m_imageRegistry->m_workersByName.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != NULL && DDRAW_WORKER_FRAME_IN_RANGE(rec, i)) {
                    r = DDRAW_WORKER_FRAME_AT_UNCHECKED(rec, i);
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
                reg->m_imageRegistry->m_workersByName.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != NULL && DDRAW_WORKER_FRAME_IN_RANGE(rec, i)) {
                    r = DDRAW_WORKER_FRAME_AT_UNCHECKED(rec, i);
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

    return CStatusBarItem::SerializeFields(s, mode, typeId, payload) != 0 ? 1 : 0;
}

RVA_COMPGEN(0x00105300, 0x1e, ??_GCSBI_SideTab@@UAEPAXI@Z)
RVA(0x00105330, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}

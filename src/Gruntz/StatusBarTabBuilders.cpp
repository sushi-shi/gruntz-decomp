#include <rva.h>

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
#include <Gruntz/SBI_SideTab.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarItemInline.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

// @early-stop
RVA(0x000e8a70, 0x18c)
i32 CSBI_GruntMachine::BuildResourceTabStatusBar(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT g,
    const char* key,
    i32 idxA,
    i32 idxB
) {

    CDDrawSurfaceMgr* h;
    CObject* found;
    CDDrawWorker* rec;
    CImage* spr;
    CDDrawWorker* cfg;
    CImage* s;
    CShadeTable* sel;
    CImage* val;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    h = host;
    INITIALIZE_STATUS_BAR_ITEM(owner, tab, h)

    m_rect14 = g;

    found = NULL;
    m_cmd = cmd;
    h->m_imageRegistry->m_workersByName.Lookup(
        "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND",
        found
    );
    rec = static_cast<CDDrawWorker*>(found);
    if (rec == NULL || DDRAW_WORKER_MISSES_FRAME(rec, 1)) {
        spr = NULL;
    } else {
        spr = DDRAW_WORKER_FRAME_AT_UNCHECKED(rec, 1);
    }
    m_standaloneFrame = spr;
    if (spr == NULL) {
        return 0;
    }
    found = NULL;
    m_host->m_imageRegistry->m_workersByName.Lookup(key, found);
    cfg = static_cast<CDDrawWorker*>(found);
    m_config = cfg;
    if (cfg == NULL) {
        return 0;
    }
    m_frameIdxA = idxA;
    m_frameIdxB = idxB;
    s = m_config->GetAt(idxA);
    m_frameA = s;
    if (s == NULL) {
        goto fail;
    }
    sel =
        g_gameReg->m_spriteFactory->GetSel(IDX(g_gameReg->m_options[g_curPlayer].m_colorIndex), 0);
    if (sel == NULL) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    m_config->SetAllTypes(SHADE_PAL_16);
    m_config->SetAllFormats(sel);
    val = m_config->GetAt(m_frameIdxB);
    m_frameB = val;
    return val != NULL;
fail:
    return 0;
}

RVA(0x000e8c70, 0xc)
void CSBI_GruntMachine::Reset() {
    m_frameA = NULL;
    m_frameB = NULL;
    m_config = NULL;
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

        m_frameA = cfg->GetAt(idx);
        idx = m_frameIdxB;
        m_frameB = cfg->GetAt(idx);

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
// Frame is 4 bytes bigger than retail's - the LOAD arm needs two address-taken
// locals (out, idx) where retail's needs one. Block-scoping `out` does not
// overlay them and costs another 84 diff lines.
RVA(0x000e8e00, 0x41a)
i32 CSBI_GruntMachine::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 pObj
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
            if (m_config != NULL) {
                strcpy(buf, m_config->m_name);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            s->Write(&m_frameIdxA, sizeof(m_frameIdxA));

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_frameA != NULL) {
                reg->m_imageRegistry->AnyValueMatches(m_frameA, buf, &v);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            s->Write(&v, sizeof(v));
            s->Write(&m_frameIdxB, sizeof(m_frameIdxB));

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_frameB != NULL) {
                reg->m_imageRegistry->AnyValueMatches(m_frameB, buf, &v);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            s->Write(&v, sizeof(v));

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_standaloneFrame != NULL) {
                reg->m_imageRegistry->AnyValueMatches(m_standaloneFrame, buf, &v);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            s->Write(&v, sizeof(v));
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;
            i32 idx;

            g_serialCounter++;
            s->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                out = NULL;
                reg->m_imageRegistry->m_workersByName.Lookup(buf, out);
                m_config = static_cast<CDDrawWorker*>(out);
            } else {
                m_config = NULL;
            }
            s->Read(&m_frameIdxA, sizeof(m_frameIdxA));

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
                m_frameA = r;
            } else {
                m_frameA = NULL;
            }
            s->Read(&m_frameIdxB, sizeof(m_frameIdxB));

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
                m_frameB = r;
            } else {
                m_frameB = NULL;
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
                m_standaloneFrame = r;
            } else {
                m_standaloneFrame = NULL;
            }
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, pObj) != 0 ? 1 : 0;
}

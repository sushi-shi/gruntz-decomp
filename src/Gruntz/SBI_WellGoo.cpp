#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_WellGoo.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/SpriteRefTable.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

// @early-stop
RVA(0x000e6020, 0x288)
i32 CSBI_WellGoo::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 fillScale
) {

    i32 sel;
    CShadeTable* node;
    CDDrawWorker* set;

    CImage* f;
    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    Initialize(owner, tab, host);
    m_rect = rc;
    m_cmd = cmd;
    m_fillScale = fillScale;
    m_dstRect.left = m_rect.left;
    m_dstRect.right = m_rect.right + 1;
    m_dstRect.bottom = m_rect.bottom + 1;
    if (key == NULL) {
        goto fail;
    }
    m_gooSrc =
        g_gameReg->World()->GetDeviceManager()->CreateOffscreenSurface(0x14, 5, BPP_RGB_16, 0, -1);
    if (m_gooSrc == NULL) {
        goto fail;
    }
    sel = IDX(g_gameReg->GetPlayer(g_curPlayer).GetColor());
    node = g_gameReg->GruntPalettes()->GetShadeTable(sel, 0);
    if (node == NULL) {
        node = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
    }

    set = m_host->FindWorker(key);
    SetFrame((set != NULL) ? set->GetAt(4) : NULL);
    if (m_frame == NULL) {
        goto fail;
    }
    if (m_frame->GetShadeBlitter() != NULL) {
        m_frame->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
    }
    f = m_frame;
    if (node != NULL && f->GetShadeBlitter() != NULL) {
        f->GetShadeBlitter()->m_palDescr = node;
    }
    m_blitter = m_frame->GetShadeBlitter();
    if (m_blitter == NULL) {
        goto fail;
    }

    set = m_host->FindWorker(key);
    m_baseFrame = (set != NULL) ? set->GetAt(2) : NULL;
    if (m_baseFrame == NULL) {
        goto fail;
    }
    if (m_baseFrame->GetShadeBlitter() != NULL) {
        m_baseFrame->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
    }
    f = m_baseFrame;
    if (node != NULL && f->GetShadeBlitter() != NULL) {
        f->GetShadeBlitter()->m_palDescr = node;
    }

    set = m_host->FindWorker(key);
    m_fgFrame = (set != NULL) ? set->GetAt(3) : NULL;
    if (m_fgFrame != NULL) {
        if (m_fgFrame->GetShadeBlitter() != NULL) {
            m_fgFrame->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
        }
        f = m_fgFrame;
        if (node != NULL && f->GetShadeBlitter() != NULL) {
            f->GetShadeBlitter()->m_palDescr = node;
        }

        SetRect(&rc, 0, 0, m_frame->GetWidth() - 1, m_frame->GetHeight() - 1);
        m_srcRect = rc;

        m_drawX = m_rect.left + ((m_rect.right - m_rect.left) >> 1) + 1;
        return 1;
    }
fail:
    return 0;
}

RVA(0x000e6360, 0x8)
i32 CSBI_WellGoo::Refresh(i32) {
    return 1;
}

RVA(0x000e6380, 0xf9)
i32 CSBI_WellGoo::Render() {
    if (m_redrawFrames <= 0) {
        return 1;
    }
    m_redrawFrames--;
    if (m_fillScale == 0) {
        return 1;
    }

    CDDrawSurfacePair* ctx = g_gameReg->World()->GetDrawTarget()->GetBackPair();
    m_baseFrame->RenderFrame(ctx, m_drawX, m_rect.bottom + 3, 0);

    double fill = static_cast<float>((m_rect.bottom - m_rect.top)) * m_fillScale * 0.01f - 3.0f;
    if (fill <= 1.0) {
        fill = 1.0;
    }
    m_dstRect.top = static_cast<i32>((static_cast<double>(m_rect.bottom) - fill));

    m_blitter->Blit(&m_srcRect, m_gooSrc, &m_srcRect, 0, 0);

    m_srcRect.right++;
    m_srcRect.bottom++;
    ctx->GetSurface()->BltEx(&m_dstRect, m_gooSrc, &m_srcRect, DDBLT_WAIT, NULL);
    m_srcRect.right--;
    m_srcRect.bottom--;

    m_fgFrame->RenderFrame(ctx, m_drawX, m_dstRect.top - 2, 0);
    return 1;
}

// @early-stop
RVA(0x000e64c0, 0x3e7)
i32 CSBI_WellGoo::SerializeFields(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (arc == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->World();
    if (mgr == NULL) {
        return 0;
    }

    if (CSBI_Image::SerializeFields(arc, mode, typeId, payload) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE: {

            arc->Write(&m_fillScale, sizeof(m_fillScale));
            arc->Write(&m_drawX, sizeof(m_drawX));
            arc->Write(&m_srcRect, sizeof(m_srcRect));
            arc->Write(&m_dstRect, sizeof(m_dstRect));
            char buf[SERIAL_NAME_LEN];
            i32 idx;
            SERIAL_WRITE_FRAME(arc, mgr, buf, idx, m_fgFrame);
            SERIAL_WRITE_FRAME(arc, mgr, buf, idx, m_baseFrame);
            return 1;
        }
        case SERIAL_LOAD: {

            arc->Read(&m_fillScale, sizeof(m_fillScale));
            arc->Read(&m_drawX, sizeof(m_drawX));
            arc->Read(&m_srcRect, sizeof(m_srcRect));
            arc->Read(&m_dstRect, sizeof(m_dstRect));
            char buf[SERIAL_NAME_LEN];
            i32 idx;
            SERIAL_READ_FRAME(arc, mgr, buf, idx, m_fgFrame);
            SERIAL_READ_FRAME(arc, mgr, buf, idx, m_baseFrame);
            return 1;
        }
        case SERIAL_POSTLOAD: {

            m_gooSrc = g_gameReg->World()
                           ->GetDeviceManager()
                           ->CreateOffscreenSurface(0x14, 5, BPP_RGB_16, 0, -1);
            if (m_gooSrc == NULL) {
                return 0;
            }
            i32 sel = IDX(g_gameReg->GetPlayer(g_curPlayer).GetColor());
            CShadeTable* node = g_gameReg->GruntPalettes()->GetShadeTable(sel, 0);
            if (node == NULL) {
                node = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
            }
            CImage* fr = m_frame;
            if (fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
            }
            fr = m_frame;
            if (node != NULL && fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->m_palDescr = node;
            }
            fr = m_baseFrame;
            if (fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
            }
            fr = m_baseFrame;
            if (node != NULL && fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->m_palDescr = node;
            }
            fr = m_fgFrame;
            if (fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
            }
            fr = m_fgFrame;
            if (node != NULL && fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->m_palDescr = node;
            }
            break;
        }
    }
    return 1;
}

RVA_COMPGEN(0x00104b80, 0x1e, ??_GCSBI_WellGoo@@UAEPAXI@Z)
RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    if (m_gooSrc != NULL) {
        m_host->GetDeviceManager()->RemoveSurface(m_gooSrc);
        m_gooSrc = NULL;
    }
}

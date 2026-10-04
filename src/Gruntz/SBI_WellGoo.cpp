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
    i32 fillPercent
) {

    i32 colorIndex;
    CShadeTable* shadeTable;
    CDDrawWorker* frames;

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
    m_fillPercent = fillPercent;
    m_fillDestRect.left = m_rect.left;
    m_fillDestRect.right = m_rect.right + 1;
    m_fillDestRect.bottom = m_rect.bottom + 1;
    if (key == NULL) {
        goto fail;
    }
    m_fillSurface =
        g_gameReg->World()->GetDeviceManager()->CreateOffscreenSurface(0x14, 5, BPP_RGB_16, 0, -1);
    if (m_fillSurface == NULL) {
        goto fail;
    }
    colorIndex = IDX(g_gameReg->GetPlayer(g_curPlayer).GetColor());
    shadeTable = g_gameReg->GruntPalettes()->GetShadeTable(colorIndex, 0);
    if (shadeTable == NULL) {
        shadeTable = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
    }

    frames = m_host->FindWorker(key);
    SetFrame((frames != NULL) ? frames->GetAt(4) : NULL);
    if (m_frame == NULL) {
        goto fail;
    }
    if (m_frame->GetShadeBlitter() != NULL) {
        m_frame->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
    }
    f = m_frame;
    if (shadeTable != NULL && f->GetShadeBlitter() != NULL) {
        f->GetShadeBlitter()->m_palDescr = shadeTable;
    }
    m_fillBlitter = m_frame->GetShadeBlitter();
    if (m_fillBlitter == NULL) {
        goto fail;
    }

    frames = m_host->FindWorker(key);
    m_bottomImage = (frames != NULL) ? frames->GetAt(2) : NULL;
    if (m_bottomImage == NULL) {
        goto fail;
    }
    if (m_bottomImage->GetShadeBlitter() != NULL) {
        m_bottomImage->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
    }
    f = m_bottomImage;
    if (shadeTable != NULL && f->GetShadeBlitter() != NULL) {
        f->GetShadeBlitter()->m_palDescr = shadeTable;
    }

    frames = m_host->FindWorker(key);
    m_topImage = (frames != NULL) ? frames->GetAt(3) : NULL;
    if (m_topImage != NULL) {
        if (m_topImage->GetShadeBlitter() != NULL) {
            m_topImage->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
        }
        f = m_topImage;
        if (shadeTable != NULL && f->GetShadeBlitter() != NULL) {
            f->GetShadeBlitter()->m_palDescr = shadeTable;
        }

        SetRect(&rc, 0, 0, m_frame->GetWidth() - 1, m_frame->GetHeight() - 1);
        m_fillSourceRect = rc;

        m_centerX = m_rect.left + ((m_rect.right - m_rect.left) >> 1) + 1;
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
    if (m_fillPercent == 0) {
        return 1;
    }

    CDDrawSurfacePair* backPair = g_gameReg->World()->GetDrawTarget()->GetBackPair();
    m_bottomImage->RenderFrame(backPair, m_centerX, m_rect.bottom + 3, 0);

    double fillHeight =
        static_cast<float>((m_rect.bottom - m_rect.top)) * m_fillPercent * 0.01f - 3.0f;
    if (fillHeight <= 1.0) {
        fillHeight = 1.0;
    }
    m_fillDestRect.top = static_cast<i32>((static_cast<double>(m_rect.bottom) - fillHeight));

    m_fillBlitter->Blit(&m_fillSourceRect, m_fillSurface, &m_fillSourceRect, 0, 0);

    m_fillSourceRect.right++;
    m_fillSourceRect.bottom++;
    backPair->GetSurface()
        ->BltEx(&m_fillDestRect, m_fillSurface, &m_fillSourceRect, DDBLT_WAIT, NULL);
    m_fillSourceRect.right--;
    m_fillSourceRect.bottom--;

    m_topImage->RenderFrame(backPair, m_centerX, m_fillDestRect.top - 2, 0);
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

            arc->Write(&m_fillPercent, sizeof(m_fillPercent));
            arc->Write(&m_centerX, sizeof(m_centerX));
            arc->Write(&m_fillSourceRect, sizeof(m_fillSourceRect));
            arc->Write(&m_fillDestRect, sizeof(m_fillDestRect));
            char buf[SERIAL_NAME_LEN];
            i32 idx;
            SERIAL_WRITE_FRAME(arc, mgr, buf, idx, m_topImage);
            SERIAL_WRITE_FRAME(arc, mgr, buf, idx, m_bottomImage);
            return 1;
        }
        case SERIAL_LOAD: {

            arc->Read(&m_fillPercent, sizeof(m_fillPercent));
            arc->Read(&m_centerX, sizeof(m_centerX));
            arc->Read(&m_fillSourceRect, sizeof(m_fillSourceRect));
            arc->Read(&m_fillDestRect, sizeof(m_fillDestRect));
            char buf[SERIAL_NAME_LEN];
            i32 idx;
            SERIAL_READ_FRAME(arc, mgr, buf, idx, m_topImage);
            SERIAL_READ_FRAME(arc, mgr, buf, idx, m_bottomImage);
            return 1;
        }
        case SERIAL_POSTLOAD: {

            m_fillSurface = g_gameReg->World()
                                ->GetDeviceManager()
                                ->CreateOffscreenSurface(0x14, 5, BPP_RGB_16, 0, -1);
            if (m_fillSurface == NULL) {
                return 0;
            }
            i32 colorIndex = IDX(g_gameReg->GetPlayer(g_curPlayer).GetColor());
            CShadeTable* shadeTable = g_gameReg->GruntPalettes()->GetShadeTable(colorIndex, 0);
            if (shadeTable == NULL) {
                shadeTable = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
            }
            CImage* fr = m_frame;
            if (fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
            }
            fr = m_frame;
            if (shadeTable != NULL && fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->m_palDescr = shadeTable;
            }
            fr = m_bottomImage;
            if (fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
            }
            fr = m_bottomImage;
            if (shadeTable != NULL && fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->m_palDescr = shadeTable;
            }
            fr = m_topImage;
            if (fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->Select(SHADE_PAL_16, NULL);
            }
            fr = m_topImage;
            if (shadeTable != NULL && fr->GetShadeBlitter() != NULL) {
                fr->GetShadeBlitter()->m_palDescr = shadeTable;
            }
            break;
        }
    }
    return 1;
}

RVA_COMPGEN(0x00104b80, 0x1e, ??_GCSBI_WellGoo@@UAEPAXI@Z)
RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    if (m_fillSurface != NULL) {
        m_host->GetDeviceManager()->RemoveSurface(m_fillSurface);
        m_fillSurface = NULL;
    }
}

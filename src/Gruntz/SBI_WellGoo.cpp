#include <rva.h>

#include <Gruntz/SBI_WellGoo.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SpriteRefTable.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

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
    m_owner = owner;
    m_host = host;
    m_tab = tab;
    m_redrawFrames = 0;
    SetEnabled(1);
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
        g_gameReg->m_world->m_deviceManager->CreateOffscreenSurface(0x14, 5, BPP_RGB_16, 0, -1);
    if (m_gooSrc == NULL) {
        goto fail;
    }
    sel = IDX(g_gameReg->m_players[g_curPlayer].m_color);
    node = g_gameReg->m_spriteFactory->GetSel(sel, 0);
    if (node == NULL) {
        node = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }

    set = LookupWorker(m_host->m_imageRegistry->m_workersByName, key);
    SetFrame((set != NULL) ? set->GetAt(4) : NULL);
    if (m_frame == NULL) {
        goto fail;
    }
    if (m_frame->m_owned != NULL) {
        m_frame->m_owned->Select(SHADE_PAL_16, NULL);
    }
    f = m_frame;
    if (node != NULL && f->m_owned != NULL) {
        f->m_owned->m_palDescr = node;
    }
    m_blitter = m_frame->m_owned;
    if (m_blitter == NULL) {
        goto fail;
    }

    set = LookupWorker(m_host->m_imageRegistry->m_workersByName, key);
    m_baseFrame = (set != NULL) ? set->GetAt(2) : NULL;
    if (m_baseFrame == NULL) {
        goto fail;
    }
    if (m_baseFrame->m_owned != NULL) {
        m_baseFrame->m_owned->Select(SHADE_PAL_16, NULL);
    }
    f = m_baseFrame;
    if (node != NULL && f->m_owned != NULL) {
        f->m_owned->m_palDescr = node;
    }

    set = LookupWorker(m_host->m_imageRegistry->m_workersByName, key);
    m_fgFrame = (set != NULL) ? set->GetAt(3) : NULL;
    if (m_fgFrame != NULL) {
        if (m_fgFrame->m_owned != NULL) {
            m_fgFrame->m_owned->Select(SHADE_PAL_16, NULL);
        }
        f = m_fgFrame;
        if (node != NULL && f->m_owned != NULL) {
            f->m_owned->m_palDescr = node;
        }

        SetRect(&rc, 0, 0, m_frame->m_width - 1, m_frame->m_height - 1);
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

    CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
    m_baseFrame->RenderFrame(ctx, m_drawX, m_rect.bottom + 3, 0);

    double fill = static_cast<float>((m_rect.bottom - m_rect.top)) * m_fillScale * 0.01f - 3.0f;
    if (fill <= 1.0) {
        fill = 1.0;
    }
    m_dstRect.top = static_cast<i32>((static_cast<double>(m_rect.bottom) - fill));

    m_blitter->Blit(&m_srcRect, m_gooSrc, &m_srcRect, 0, 0);

    m_srcRect.right++;
    m_srcRect.bottom++;
    ctx->m_surface->BltEx(&m_dstRect, m_gooSrc, &m_srcRect, DDBLT_WAIT, NULL);
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
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
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
            g_serialCounter++;
            memset(buf, 0, SERIAL_NAME_LEN);
            idx = 0;
            if (m_fgFrame != NULL) {
                mgr->m_imageRegistry->AnyValueMatches(m_fgFrame, buf, &idx);
            }
            arc->Write(buf, SERIAL_NAME_LEN);
            arc->Write(&idx, sizeof(idx));
            g_serialCounter++;
            memset(buf, 0, SERIAL_NAME_LEN);
            idx = 0;
            if (m_baseFrame != NULL) {
                mgr->m_imageRegistry->AnyValueMatches(m_baseFrame, buf, &idx);
            }
            arc->Write(buf, SERIAL_NAME_LEN);
            arc->Write(&idx, sizeof(idx));
            return 1;
        }
        case SERIAL_LOAD: {

            arc->Read(&m_fillScale, sizeof(m_fillScale));
            arc->Read(&m_drawX, sizeof(m_drawX));
            arc->Read(&m_srcRect, sizeof(m_srcRect));
            arc->Read(&m_dstRect, sizeof(m_dstRect));
            char buf[SERIAL_NAME_LEN];
            i32 idx;
            g_serialCounter++;
            arc->Read(buf, SERIAL_NAME_LEN);
            arc->Read(&idx, sizeof(idx));
            if (strlen(buf) != 0) {
                i32 frameIndex = idx;
                CObject* found = NULL;
                mgr->m_imageRegistry->m_workersByName.Lookup(buf, found);
                CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
                if (set != NULL) {
                    m_fgFrame = set->GetAt(frameIndex);
                } else {
                    m_fgFrame = NULL;
                }
            } else {
                m_fgFrame = NULL;
            }
            g_serialCounter++;
            arc->Read(buf, SERIAL_NAME_LEN);
            arc->Read(&idx, sizeof(idx));
            if (strlen(buf) != 0) {
                i32 frameIndex = idx;
                CObject* found = NULL;
                mgr->m_imageRegistry->m_workersByName.Lookup(buf, found);
                CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
                if (set != NULL) {
                    m_baseFrame = set->GetAt(frameIndex);
                } else {
                    m_baseFrame = NULL;
                }
            } else {
                m_baseFrame = NULL;
            }
            return 1;
        }
        case SERIAL_POSTLOAD: {

            m_gooSrc = g_gameReg->m_world->m_deviceManager
                           ->CreateOffscreenSurface(0x14, 5, BPP_RGB_16, 0, -1);
            if (m_gooSrc == NULL) {
                return 0;
            }
            i32 sel = IDX(g_gameReg->m_players[g_curPlayer].m_color);
            CShadeTable* node = g_gameReg->m_spriteFactory->GetSel(sel, 0);
            if (node == NULL) {
                node = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }
            CImage* fr = m_frame;
            if (fr->m_owned != NULL) {
                fr->m_owned->Select(SHADE_PAL_16, NULL);
            }
            if (node != NULL && m_frame->m_owned != NULL) {
                m_frame->m_owned->m_palDescr = node;
            }
            fr = m_baseFrame;
            if (fr->m_owned != NULL) {
                fr->m_owned->Select(SHADE_PAL_16, NULL);
            }
            if (node != NULL && m_baseFrame->m_owned != NULL) {
                m_baseFrame->m_owned->m_palDescr = node;
            }
            fr = m_fgFrame;
            if (fr->m_owned != NULL) {
                fr->m_owned->Select(SHADE_PAL_16, NULL);
            }
            if (node != NULL && m_fgFrame->m_owned != NULL) {
                m_fgFrame->m_owned->m_palDescr = node;
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
        m_host->m_deviceManager->RemoveSurface(m_gooSrc);
        m_gooSrc = NULL;
    }
}

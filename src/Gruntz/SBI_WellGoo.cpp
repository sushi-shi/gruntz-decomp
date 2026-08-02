#include <rva.h>

#include <Gruntz/SBI_WellGoo.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SpriteRefTable.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

VTBL(CSBI_WellGoo, 0x001eadfc);

// @early-stop
RVA(0x000e6020, 0x288)
i32 CSBI_WellGoo::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 fillScale
) {

    i32 sel;
    CShadeTable* node;
    CObject* found;
    CDDrawWorker* set;

    CImage* f;
    if (host == 0) {
        goto fail;
    }
    if (owner == 0) {
        goto fail;
    }
    m_owner = owner;
    m_host = host;
    m_tab = tab;
    m_redrawFrames = 0;
    m_enabled = 1;
    m_rect14 = rc;
    m_cmd = cmd;
    m_fillScale = fillScale;
    m_dstRect.left = m_rect14.left;
    m_dstRect.right = m_rect14.right + 1;
    m_dstRect.bottom = m_rect14.bottom + 1;
    if (key == 0) {
        goto fail;
    }
    m_gooSrc = g_gameReg->m_world->m_ptrColl->MakeAndAddB(0x14, 5, 0x10, 0, -1);
    if (m_gooSrc == 0) {
        goto fail;
    }
    sel = g_gameReg->m_options[g_curPlayer].m_colorIndex;
    node = g_gameReg->m_spriteFactory->GetSel(sel, 0);
    if (node == 0) {
        node = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }

    found = 0;
    m_host->m_imageRegistry->m_10map.Lookup(key, found);
    set = static_cast<CDDrawWorker*>(found);
    m_frame = (set != 0) ? set->GetAt(4) : 0;
    if (m_frame == 0) {
        goto fail;
    }
    if (m_frame->m_owned != 0) {
        m_frame->m_owned->Select(0xa, 0);
    }
    f = m_frame;
    if (node != 0 && f->m_owned != 0) {
        f->m_owned->m_palDescr = node;
    }
    m_blitter = m_frame->m_owned;
    if (m_blitter == 0) {
        goto fail;
    }

    found = 0;
    m_host->m_imageRegistry->m_10map.Lookup(key, found);
    set = static_cast<CDDrawWorker*>(found);
    m_baseFrame = (set != 0) ? set->GetAt(2) : 0;
    if (m_baseFrame == 0) {
        goto fail;
    }
    if (m_baseFrame->m_owned != 0) {
        m_baseFrame->m_owned->Select(0xa, 0);
    }
    f = m_baseFrame;
    if (node != 0 && f->m_owned != 0) {
        f->m_owned->m_palDescr = node;
    }

    found = 0;
    m_host->m_imageRegistry->m_10map.Lookup(key, found);
    set = static_cast<CDDrawWorker*>(found);
    m_fgFrame = (set != 0) ? set->GetAt(3) : 0;
    if (m_fgFrame == 0) {
        goto fail;
    }
    if (m_fgFrame->m_owned != 0) {
        m_fgFrame->m_owned->Select(0xa, 0);
    }
    f = m_fgFrame;
    if (node != 0 && f->m_owned != 0) {
        f->m_owned->m_palDescr = node;
    }

    ::SetRect(&rc, 0, 0, m_frame->m_width - 1, m_frame->m_height - 1);
    m_srcRect = rc;

    m_drawX = m_rect14.left + ((m_rect14.right - m_rect14.left) >> 1) + 1;
    return 1;
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
    m_baseFrame->RenderFrame(ctx, m_drawX, m_rect14.bottom + 3, 0);

    double fill = static_cast<float>((m_rect14.bottom - m_rect14.top)) * m_fillScale * 0.01f - 3.0f;
    if (fill <= 1.0) {
        fill = 1.0;
    }
    m_dstRect.top = static_cast<i32>((static_cast<double>(m_rect14.bottom) - fill));

    m_blitter->Blit(&m_srcRect, m_gooSrc, &m_srcRect, 0, 0);

    m_srcRect.right++;
    m_srcRect.bottom++;
    ctx->m_surface->BltEx(&m_dstRect, m_gooSrc, &m_srcRect, 0x1000000, 0);
    m_srcRect.right--;
    m_srcRect.bottom--;

    m_fgFrame->RenderFrame(ctx, m_drawX, m_dstRect.top - 2, 0);
    return 1;
}

// @early-stop
RVA(0x000e64c0, 0x3e7)
i32 CSBI_WellGoo::SerializeFields(CFileMemBase* arc, i32 mode, i32 typeId, i32 pObj) {
    if (arc == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }

    if (CSBI_Image::SerializeFields(arc, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case 4: {

            arc->Write(&m_fillScale, 4);
            arc->Write(&m_drawX, 4);
            arc->Write(&m_srcRect, 0x10);
            arc->Write(&m_dstRect, 0x10);
            char buf[0x80];
            i32 idx;
            g_serialCounter++;
            memset(buf, 0, 0x80);
            idx = 0;
            if (m_fgFrame != 0) {
                mgr->m_imageRegistry->AnyValueMatches(m_fgFrame, buf, &idx);
            }
            arc->Write(buf, 0x80);
            arc->Write(&idx, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            idx = 0;
            if (m_baseFrame != 0) {
                mgr->m_imageRegistry->AnyValueMatches(m_baseFrame, buf, &idx);
            }
            arc->Write(buf, 0x80);
            arc->Write(&idx, 4);
            return 1;
        }
        case 7: {

            arc->Read(&m_fillScale, 4);
            arc->Read(&m_drawX, 4);
            arc->Read(&m_srcRect, 0x10);
            arc->Read(&m_dstRect, 0x10);
            char buf[0x80];
            i32 idx;
            g_serialCounter++;
            arc->Read(buf, 0x80);
            arc->Read(&idx, 4);
            if (strlen(buf) != 0) {
                CObject* found = 0;
                mgr->m_imageRegistry->m_10map.Lookup(buf, found);
                CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
                if (set != 0) {
                    m_fgFrame = set->GetAt(idx);
                } else {
                    m_fgFrame = 0;
                }
            } else {
                m_fgFrame = 0;
            }
            g_serialCounter++;
            arc->Read(buf, 0x80);
            arc->Read(&idx, 4);
            if (strlen(buf) != 0) {
                CObject* found = 0;
                mgr->m_imageRegistry->m_10map.Lookup(buf, found);
                CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
                if (set != 0) {
                    m_baseFrame = set->GetAt(idx);
                } else {
                    m_baseFrame = 0;
                }
            } else {
                m_baseFrame = 0;
            }
            return 1;
        }
        case 8: {

            m_gooSrc = mgr->m_ptrColl->MakeAndAddB(0x14, 5, 0x10, 0, -1);
            if (m_gooSrc == 0) {
                return 0;
            }
            i32 sel = g_gameReg->m_options[g_curPlayer].m_colorIndex;
            CShadeTable* node = g_gameReg->m_spriteFactory->GetSel(sel, 0);
            if (node == 0) {
                node = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }
            CImage* fr = m_frame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_frame->m_owned != 0) {
                m_frame->m_owned->m_palDescr = node;
            }
            fr = m_baseFrame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_baseFrame->m_owned != 0) {
                m_baseFrame->m_owned->m_palDescr = node;
            }
            fr = m_fgFrame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_fgFrame->m_owned != 0) {
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
    if (m_gooSrc != 0) {
        m_host->m_ptrColl->RemoveItemA(m_gooSrc);
        m_gooSrc = 0;
    }
}

RVA(0x00104c80, 0x1f)
void CSBI_WellGoo::Reset() {
    if (m_gooSrc != 0) {
        m_host->m_ptrColl->RemoveItemA(m_gooSrc);
        m_gooSrc = 0;
    }
}

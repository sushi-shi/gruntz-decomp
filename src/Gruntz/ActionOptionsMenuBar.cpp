#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GameLevel.h>
#include <string.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h>
#include <Wwd/WwdFile.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/Sprite.h>

// @early-stop
RVA(0x00009090, 0x32)
CActionOptionsMenuBar::CActionOptionsMenuBar() {
    m_frame = 0;
    m_normChipSprite = 0;
    m_highChipSprite = 0;
    m_greyChipSprite = 0;
    m_buttonFrame[0] = 0;
    m_buttonFrame[1] = 0;
    m_buttonIcon[0] = 0;
    m_buttonIcon[1] = 0;
    m_buttonState[0] = 0;
    m_buttonState[1] = 0;
    m_loaded = 0;
}

// @early-stop
RVA(0x000090e0, 0x100)
i32 CActionOptionsMenuBar::LoadAssets() {
    CObject* spr_ob = 0;

    m_active = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_ACTIONOPTIONZMENUBAR", spr_ob);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(spr_ob);
    m_frame = (spr && spr->m_minIndex <= 1 && spr->m_maxIndex >= 1)
                  ? static_cast<CImage*>(spr->m_items.GetAt(1))
                  : 0;
    if (!m_frame) {
        return 0;
    }

    spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_INGAMEICONZ_NORMCHIPZ", spr_ob);
    spr = static_cast<CDDrawWorker*>(spr_ob);
    m_normChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_INGAMEICONZ_HIGHCHIPZ", spr_ob);
    spr = static_cast<CDDrawWorker*>(spr_ob);
    m_highChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_INGAMEICONZ_GREYCHIPZ", spr_ob);
    spr = static_cast<CDDrawWorker*>(spr_ob);
    m_greyChipSprite = spr;
    if (!spr) {
        return 0;
    }

    m_loaded = 1;
    return 1;
}

// @early-stop
RVA(0x00009220, 0x8f)
i32 CActionOptionsMenuBar::Init(i32 gx, i32 a, i32 x, i32 y, i32 b, i32 gy) {
    if (m_active) {
        return 0;
    }
    if (x - 0x25 < 0) {
        x = 0x25;
    } else {
        i32 limit = (g_gameReg->m_world->m_level->m_mainPlane)->m_wrapW;
        if (x + 0x25 >= limit) {
            x = limit - 0x26;
        }
    }
    i32 ym = y - 0x34;
    i32 yy;
    if (ym - 0x19 >= 0) {
        yy = ym;
    } else {
        yy = y + 0x34;
    }
    m_screenX = x;
    m_gridX = b;
    m_screenY = yy;
    m_buttonState[1] = a;
    m_gridY = gy;
    m_buttonState[0] = gx;
    if (Refresh() == 0) {
        return 0;
    }
    m_active = 1;
    return 1;
}

RVA(0x000092e0, 0x8)
void CActionOptionsMenuBar::Clear() {
    m_loaded = 0;
}

RVA(0x00009300, 0x14)
i32 CActionOptionsMenuBar::Activate(i32 a) {
    if (m_active) {
        Refresh();
    }
    return 1;
}

RVA(0x00009330, 0x140)
i32 CActionOptionsMenuBar::Refresh() {
    CGrunt* grunt = g_gameReg->m_cmdGrid->m_grid[m_gridY + m_gridX * TM_GRID_COLS];
    if (grunt == 0) {
        m_buttonIcon[1] = 0;
        m_buttonIcon[0] = 0;
    } else {
        m_buttonIcon[1] = grunt->m_vehiclePickupType;
        if (grunt->m_entranceReason >= 0x17) {
            m_buttonState[1] = 3;
        } else if (m_buttonState[1] == 3) {
            m_buttonState[1] = 1;
        }
        i32 prim = (grunt->m_entranceReason > 0x16) ? grunt->m_toolId : grunt->m_entranceReason;
        m_buttonIcon[0] = prim;
        if (prim == 0) {
            m_buttonIcon[0] = 0x21;
        } else if (prim == 3) {
            m_buttonIcon[0] = grunt->m_brickPickupType;
        }
        if (!grunt->CanShowStamina()) {
            m_buttonState[0] = 3;
        } else if (m_buttonState[0] == 3) {
            m_buttonState[0] = 1;
        }
    }

    for (i32 i = 0; i < 2; i++) {
        if (m_buttonIcon[i] == 0) {
            m_buttonState[i] = 0;
        } else if (m_buttonState[i] == 0) {
            m_buttonState[i] = 1;
        }
        switch (m_buttonState[i]) {
            case 1:
                m_buttonFrame[i] = m_normChipSprite->GetAt(m_buttonIcon[i]);
                break;
            case 2:
                m_buttonFrame[i] = m_highChipSprite->GetAt(m_buttonIcon[i]);
                break;
            case 3:
                m_buttonFrame[i] = m_greyChipSprite->GetAt(m_buttonIcon[i]);
                break;
            default:
                m_buttonFrame[i] = 0;
                break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000094c0, 0x131)
i32 CActionOptionsMenuBar::Render() {
    if (!m_active) {
        return 1;
    }
    LONG sx = m_screenX;
    LONG sy = m_screenY;
    (g_gameReg->m_world->m_level->m_mainPlane)->WrapCoord(&sx, &sy);

    RECT r = g_gameReg->m_world->m_level->m_planeCtx;
    CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
    m_frame->RenderFrameClipped(ctx, sy, sx, &r, 0);

    if (m_buttonFrame[0]) {
        r = g_gameReg->m_world->m_level->m_planeCtx;
        m_frame->RenderFrameClipped(ctx, sy - 0xc, sx + 2, &r, 0);
    }
    if (m_buttonFrame[1]) {
        r = g_gameReg->m_world->m_level->m_planeCtx;
        m_frame->RenderFrameClipped(ctx, sy + 0x10, sx + 2, &r, 0);
    }
    return 1;
}

// @early-stop
RVA(0x00009650, 0xcf)
i32 CActionOptionsMenuBar::HitClick(i32 mx, i32 my) {
    if (!m_active) {
        return 1;
    }
    i32 cell = m_gridY + m_gridX * TM_GRID_COLS;
    CGrunt* unit = g_gameReg->m_cmdGrid->m_grid[cell];
    if (unit == 0) {
        return 1;
    }

    i32* btn = m_buttonState;
    i32* p = btn;
    i32 k = 2;
    do {
        if (*p == 2) {
            *p = 1;
        }
        ++p;
    } while (--k != 0);

    i32 y0 = m_screenY;
    i32 ylo = y0 - 0xa;
    i32 yhi = y0 + 0xe;
    i32 x0 = m_screenX;

    if (mx < x0 && mx >= x0 - 0x18 && my < yhi && my >= ylo) {
        if (*btn == 1) {
            *btn = 2;
        }
        return 1;
    }

    if (mx < x0 + 0x1c && mx >= x0 + 0x4 && my < yhi && my >= ylo) {
        if (m_buttonState[1] == 1) {
            m_buttonState[1] = 2;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00009760, 0x6c)
i32 CActionOptionsMenuBar::HitHover(i32 mx, i32 my) {
    if (!m_active) {
        return 0;
    }
    i32 y0 = m_screenY;
    i32 x0 = m_screenX;
    i32 ylo = y0 - 0xc;
    i32 yhi = y0 + 0xc;
    if (mx < x0 && mx >= x0 - 0x18 && my < yhi && my >= ylo && m_buttonState[0] != 3) {
        return 2;
    }

    if (mx < x0 + 0x18 && mx >= x0 && my < yhi && my >= ylo && m_buttonState[1] != 3) {
        return 3;
    }
    return 0;
}

RVA(0x000097f0, 0x8)
void CActionOptionsMenuBar::Deactivate() {
    m_active = 0;
}

// @early-stop
RVA(0x00009810, 0x2df)
i32 CActionOptionsMenuBar::Serialize(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    CGruntzMgr* reg = g_gameReg;
    if (reg == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = reg->m_world;
    if (mgr == 0) {
        return 0;
    }

    ar->Write(this, 8);
    ar->Write(&m_screenX, 4);
    ar->Write(&m_screenY, 4);
    ar->Write(&m_loaded, 4);
    ar->Write(&m_active, 4);
    ar->Write(m_buttonState, 8);
    ar->Write(m_buttonIcon, 8);

    char tmp[0x80];

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_normChipSprite) {
        strcpy(tmp, m_normChipSprite->m_name);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_highChipSprite) {
        strcpy(tmp, m_highChipSprite->m_name);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_greyChipSprite) {
        strcpy(tmp, m_greyChipSprite->m_name);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frame) {
            mgr->m_imageRegistry->AnyValueMatches(m_frame, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_buttonFrame[0]) {
            mgr->m_imageRegistry->AnyValueMatches(m_buttonFrame[0], tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    {
        i32 zero = 0;
        CImage* v20 = m_buttonFrame[1];
        memset(tmp, 0, sizeof(tmp));
        if (v20) {
            mgr->m_imageRegistry->AnyValueMatches(v20, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }
    return 1;
}

// @early-stop
RVA(0x00009bb0, 0x367)
i32 CActionOptionsMenuBar::Deserialize(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    CGruntzMgr* gr = g_gameReg;
    if (gr == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = gr->m_world;
    if (mgr == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;

    s->Read(this, 8);
    s->Read(&m_screenX, 4);
    s->Read(&m_screenY, 4);
    s->Read(&m_loaded, 4);
    s->Read(&m_active, 4);
    s->Read(&m_buttonState[0], 8);
    s->Read(&m_buttonIcon[0], 8);

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        m_normChipSprite = static_cast<CDDrawWorker*>(out);
    } else {
        m_normChipSprite = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        m_highChipSprite = static_cast<CDDrawWorker*>(out);
    } else {
        m_highChipSprite = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        m_greyChipSprite = static_cast<CDDrawWorker*>(out);
    } else {
        m_greyChipSprite = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frame = r;
    } else {
        m_frame = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_buttonFrame[0] = r;
    } else {
        m_buttonFrame[0] = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_buttonFrame[1] = r;
    } else {
        m_buttonFrame[1] = 0;
    }

    return 1;
}

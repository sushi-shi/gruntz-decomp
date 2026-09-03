#include <Gruntz/ActionOptionsMenuBar.h>

#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Wwd/WwdFile.h>

#include <string.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

RVA(0x00009090, 0x32)
CActionOptionsMenuBar::CActionOptionsMenuBar() {
    m_frame = NULL;
    m_normChipSprite = NULL;
    m_highChipSprite = NULL;
    m_greyChipSprite = NULL;
    memset(m_buttonFrame, 0, sizeof(m_buttonFrame));
    memset(m_buttonIcon, 0, sizeof(m_buttonIcon));
    memset(m_buttonState, 0, sizeof(m_buttonState));
    m_loaded = false;
}

RVA(0x000090e0, 0x100)
i32 CActionOptionsMenuBar::LoadAssets() {
    m_active = false;
    CDDrawWorker* spr = LookupWorker(
        g_gameReg->m_world->m_imageRegistry->m_workersByName,
        "GAME_ACTIONOPTIONZMENUBAR"
    );
    m_frame = spr ? spr->GetAt(1) : NULL;
    if (!m_frame) {
        return 0;
    }

    spr = LookupWorker(
        g_gameReg->m_world->m_imageRegistry->m_workersByName,
        "GAME_INGAMEICONZ_NORMCHIPZ"
    );
    m_normChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr = LookupWorker(
        g_gameReg->m_world->m_imageRegistry->m_workersByName,
        "GAME_INGAMEICONZ_HIGHCHIPZ"
    );
    m_highChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr = LookupWorker(
        g_gameReg->m_world->m_imageRegistry->m_workersByName,
        "GAME_INGAMEICONZ_GREYCHIPZ"
    );
    m_greyChipSprite = spr;
    if (!spr) {
        return 0;
    }

    m_loaded = true;
    return 1;
}

RVA(0x00009220, 0x8f)
i32 CActionOptionsMenuBar::Init(
    ActionOptionButtonState primaryState,
    ActionOptionButtonState secondaryState,
    i32 x,
    i32 y,
    i32 playerIndex,
    i32 unitIndex
) {
    if (m_active) {
        return 0;
    }
    Coord position(x, y);
    if (position.m_x - 0x25 < 0) {
        position.m_x = 0x25;
    } else {
        i32 limit = (g_gameReg->m_world->m_level->m_mainPlane)->m_planePixelSize.cx;
        if (position.m_x + 0x25 >= limit) {
            position.m_x = limit - 0x26;
        }
    }
    i32 upperY = position.m_y - 0x34;
    if (upperY - 0x19 >= 0) {
        position.m_y = upperY;
    } else {
        position.m_y += 0x34;
    }
    m_playerIndex = playerIndex;
    m_unitIndex = unitIndex;
    m_screenPosition = position;
    m_buttonState[0] = primaryState;
    m_buttonState[1] = secondaryState;
    if (Refresh() == 0) {
        return 0;
    }
    m_active = true;
    return 1;
}

RVA(0x000092e0, 0x8)
void CActionOptionsMenuBar::Clear() {
    m_loaded = false;
}

RVA(0x00009300, 0x14)
i32 CActionOptionsMenuBar::RefreshIfActive(i32 unusedDeltaMs) {
    if (m_active) {
        Refresh();
    }
    return 1;
}

RVA(0x00009330, 0x140)
i32 CActionOptionsMenuBar::Refresh() {
    CGrunt* grunt =
        g_gameReg->m_triggerMgr->m_units[m_unitIndex + m_playerIndex * TM_UNITS_PER_PLAYER];
    if (grunt == NULL) {
        m_buttonIcon[1] = PICKUP_NONE;
        m_buttonIcon[0] = PICKUP_NONE;
    } else {
        m_buttonIcon[1] = grunt->m_vehiclePickupType;
        if (grunt->m_entranceReason >= PICKUP_TOYZ_FIRST) {
            m_buttonState[1] = ACTIONOPTION_DISABLED;
        } else if (m_buttonState[1] == ACTIONOPTION_DISABLED) {
            m_buttonState[1] = ACTIONOPTION_NORMAL;
        }
        PickupType prim = ArrivalPickup(grunt);
        m_buttonIcon[0] = prim;
        if (prim == PICKUP_NONE) {
            m_buttonIcon[0] = PICKUP_BARE_HANDS_ICON;
        } else if (prim == PICKUP_BRICK) {
            m_buttonIcon[0] = grunt->m_brickPickupType;
        }
        if (!grunt->CanShowStamina()) {
            m_buttonState[0] = ACTIONOPTION_DISABLED;
        } else if (m_buttonState[0] == ACTIONOPTION_DISABLED) {
            m_buttonState[0] = ACTIONOPTION_NORMAL;
        }
    }

    for (i32 i = 0; i < 2; i++) {
        if (m_buttonIcon[i] == PICKUP_NONE) {
            m_buttonState[i] = ACTIONOPTION_HIDDEN;
        } else if (m_buttonState[i] == ACTIONOPTION_HIDDEN) {
            m_buttonState[i] = ACTIONOPTION_NORMAL;
        }
        switch (m_buttonState[i]) {
            case ACTIONOPTION_NORMAL:
                m_buttonFrame[i] = m_normChipSprite->GetAt(IDX(m_buttonIcon[i]));
                break;
            case ACTIONOPTION_SELECTED:
                m_buttonFrame[i] = m_highChipSprite->GetAt(IDX(m_buttonIcon[i]));
                break;
            case ACTIONOPTION_DISABLED:
                m_buttonFrame[i] = m_greyChipSprite->GetAt(IDX(m_buttonIcon[i]));
                break;
            default:
                m_buttonFrame[i] = NULL;
                break;
        }
    }
    return 1;
}

RVA(0x000094c0, 0x131)
i32 CActionOptionsMenuBar::Render() {
    if (!m_active) {
        return 1;
    }
    CGameLevel* level = g_gameReg->m_world->m_level;
    CPoint screen(m_screenPosition.m_x, m_screenPosition.m_y);
    level->m_mainPlane->WorldToViewport(&screen.x, &screen.y);

    CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
    LevelCoordRect r = g_gameReg->m_world->m_level->m_viewportRect;
    m_frame->RenderFrameClipped(ctx, screen.x, screen.y, &r, 0);

    if (m_buttonFrame[0]) {
        r = g_gameReg->m_world->m_level->m_viewportRect;
        m_buttonFrame[0]->RenderFrameClipped(ctx, screen.x - 0xc, screen.y + 2, &r, 0);
    }
    if (m_buttonFrame[1]) {
        r = g_gameReg->m_world->m_level->m_viewportRect;
        m_buttonFrame[1]->RenderFrameClipped(ctx, screen.x + 0x10, screen.y + 2, &r, 0);
    }
    return 1;
}

RVA(0x00009650, 0xcf)
i32 CActionOptionsMenuBar::HitClick(i32 mx, i32 my) {
    if (!m_active) {
        return 1;
    }
    i32 registryIndex = m_unitIndex + m_playerIndex * TM_UNITS_PER_PLAYER;
    CGrunt* unit = g_gameReg->m_triggerMgr->m_units[registryIndex];
    if (unit == NULL) {
        return 1;
    }

    ActionOptionButtonState* btn = m_buttonState;
    ActionOptionButtonState* p = btn;
    i32 k = 2;
    do {
        if (*p == ACTIONOPTION_SELECTED) {
            *p = ACTIONOPTION_NORMAL;
        }
        ++p;
    } while (--k != 0);

    CRect primaryButton(
        m_screenPosition.m_x - 0x18,
        m_screenPosition.m_y - 0xa,
        m_screenPosition.m_x,
        m_screenPosition.m_y + 0xe
    );
    if (::PtInRect(&primaryButton, mx, my)) {
        if (*btn == ACTIONOPTION_NORMAL) {
            *btn = ACTIONOPTION_SELECTED;
        }
        return 1;
    }

    CRect secondaryButton(
        m_screenPosition.m_x + 0x4,
        m_screenPosition.m_y - 0xa,
        m_screenPosition.m_x + 0x1c,
        m_screenPosition.m_y + 0xe
    );
    if (::PtInRect(&secondaryButton, mx, my)) {
        if (m_buttonState[1] != ACTIONOPTION_NORMAL) {
            return 1;
        }
        m_buttonState[1] = ACTIONOPTION_SELECTED;
        return 1;
    }
    return 1;
}

RVA(0x00009760, 0x6c)
ActionOptionHit CActionOptionsMenuBar::HitHover(i32 mx, i32 my) {
    if (!m_active) {
        return ACTIONOPTION_HIT_NONE;
    }
    CRect primaryButton(
        m_screenPosition.m_x - 0x18,
        m_screenPosition.m_y - 0xc,
        m_screenPosition.m_x,
        m_screenPosition.m_y + 0xc
    );
    if (::PtInRect(&primaryButton, mx, my) && m_buttonState[0] != ACTIONOPTION_DISABLED) {
        return ACTIONOPTION_HIT_PRIMARY;
    }

    CRect secondaryButton(
        m_screenPosition.m_x,
        m_screenPosition.m_y - 0xc,
        m_screenPosition.m_x + 0x18,
        m_screenPosition.m_y + 0xc
    );
    if (::PtInRect(&secondaryButton, mx, my) && m_buttonState[1] != ACTIONOPTION_DISABLED) {
        return ACTIONOPTION_HIT_SECONDARY;
    }
    return ACTIONOPTION_HIT_NONE;
}

RVA(0x000097f0, 0x8)
void CActionOptionsMenuBar::Deactivate() {
    m_active = false;
}

// @early-stop
RVA(0x00009810, 0x2df)
i32 CActionOptionsMenuBar::Serialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CGruntzMgr* reg = g_gameReg;
    if (reg == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = reg->m_world;
    if (mgr == NULL) {
        return 0;
    }

    ar->Write(this, 8);
    ar->Write(&m_screenPosition.m_x, sizeof(m_screenPosition.m_x));
    ar->Write(&m_screenPosition.m_y, sizeof(m_screenPosition.m_y));
    ar->Write(&m_loaded, sizeof(m_loaded));
    ar->Write(&m_active, sizeof(m_active));
    ar->Write(m_buttonState, 8);
    ar->Write(m_buttonIcon, 8);

    char tmp[SERIAL_NAME_LEN];

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_normChipSprite) {
        strcpy(tmp, m_normChipSprite->m_name);
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_highChipSprite) {
        strcpy(tmp, m_highChipSprite->m_name);
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_greyChipSprite) {
        strcpy(tmp, m_greyChipSprite->m_name);
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        CImage* frame = m_frame;
        i32 zero = 0;
        if (frame) {
            mgr->m_imageRegistry->AnyValueMatches(frame, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        CImage* frame = m_buttonFrame[0];
        i32 zero = 0;
        if (frame) {
            mgr->m_imageRegistry->AnyValueMatches(frame, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        CImage* frame = m_buttonFrame[1];
        i32 zero = 0;
        if (frame) {
            mgr->m_imageRegistry->AnyValueMatches(frame, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }
    return 1;
}

RVA(0x00009bb0, 0x367)
i32 CActionOptionsMenuBar::Deserialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    CGruntzMgr* gr = g_gameReg;
    if (gr == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = gr->m_world;
    if (mgr == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    i32 idx;

    s->Read(this, 8);
    s->Read(&m_screenPosition.m_x, sizeof(m_screenPosition.m_x));
    s->Read(&m_screenPosition.m_y, sizeof(m_screenPosition.m_y));
    s->Read(&m_loaded, sizeof(m_loaded));
    s->Read(&m_active, sizeof(m_active));
    s->Read(&m_buttonState[0], 8);
    s->Read(&m_buttonIcon[0], 8);

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    if (strlen(buf) != 0) {
        m_normChipSprite = LookupWorker(mgr->m_imageRegistry->m_workersByName, buf);
    } else {
        m_normChipSprite = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    if (strlen(buf) != 0) {
        m_highChipSprite = LookupWorker(mgr->m_imageRegistry->m_workersByName, buf);
    } else {
        m_highChipSprite = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    if (strlen(buf) != 0) {
        m_greyChipSprite = LookupWorker(mgr->m_imageRegistry->m_workersByName, buf);
    } else {
        m_greyChipSprite = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(mgr->m_imageRegistry->m_workersByName, buf);
        CImage* r = tt != NULL ? tt->GetAt(i) : NULL;
        m_frame = r;
    } else {
        m_frame = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(mgr->m_imageRegistry->m_workersByName, buf);
        CImage* r = tt != NULL ? tt->GetAt(i) : NULL;
        m_buttonFrame[0] = r;
    } else {
        m_buttonFrame[0] = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(mgr->m_imageRegistry->m_workersByName, buf);
        CImage* r = tt != NULL ? tt->GetAt(i) : NULL;
        m_buttonFrame[1] = r;
    } else {
        m_buttonFrame[1] = NULL;
    }

    return 1;
}

RVA(0x0000a000, 0xac)
void CDDrawWorkerHost::WorldToViewport(LONG* px, LONG* py) {
    WwdPlaneFlags flags = static_cast<WwdPlaneFlags>(m_flags);
    if (HAS(flags, WWD_PLANE_FLAG_WRAP_X)) {
        if (*px < 0) {
            *px += m_planePixelSize.cx;
        } else if (*px >= m_planePixelSize.cx) {
            *px -= m_planePixelSize.cx;
        }
        if (m_planeViewRect.right >= m_planePixelSize.cx && *px < m_planeViewRect.left
            && *px <= m_planeViewRect.right - m_planePixelSize.cx) {
            *px = m_planePixelSize.cx + *px;
        }
    }

    if (HAS(flags, WWD_PLANE_FLAG_WRAP_Y)) {
        if (*py < 0) {
            *py += m_planePixelSize.cy;
        } else if (*py >= m_planePixelSize.cy) {
            *py -= m_planePixelSize.cy;
        }
        if (m_planeViewRect.bottom >= m_planePixelSize.cy && *py < m_planeViewRect.top
            && *py <= m_planeViewRect.bottom - m_planePixelSize.cy) {
            *py = m_planePixelSize.cy + *py;
        }
    }

    *px = *px - m_planeViewRect.left;
    *py = *py - m_planeViewRect.top;
    *px = *px + m_viewportRect.left;
    *py = *py + m_viewportRect.top;
}

#define GRUNTZ_MENUITEM_TU

#include <rva.h>

#include <Gruntz/MenuItem.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/MenuItem2.h>
#include <Gruntz/MenuItemState.h>
#include <Gruntz/MenuPage.h>
#include <Image/CImage.h>
#include <Wap32/CoordUnset.h>

#include <stdio.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

RVA(0x00185460, 0xa9)
i32 CMenuItem::Init(
    CMenuPage* page,
    const char* name,
    const char* spriteKey,
    i32 cmdId,
    const char* key,
    i32 flags
) {
    if (!page) {
        return 0;
    }
    m_flags = flags;
    m_owner = page->m_owner;
    m_host = page->m_host;
    m_template = page;
    m_name = name;
    m_key = key;
    m_cmdId = cmdId;
    m_secondaryCmdId = 0;
    m_cmdParam = 0;
    if (m_flags & 1) {
        m_state = MENUSTATE_DISABLED;
    } else {
        m_state = MENUSTATE_NORMAL;
    }
    if (!OnInit()) {
        CObject* slot = 0;

        m_owner->m_imageRegistry->m_workersByName.Lookup(spriteKey, slot);
        m_sprite = slot;
        if (!slot) {
            return 0;
        }
    }
    return 1;
}
RVA(0x00185510, 0x5)
void CMenuItem::Cleanup() {
    Reset();
}

RVA(0x00185520, 0x2c)
i32 CMenuItem::GetFrameWidth() {
    CDDrawWorker* s = static_cast<CDDrawWorker*>(m_sprite);
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(2);
    if (!f) {
        return 0;
    }
    return f->m_width;
}
RVA(0x00185550, 0x2c)
i32 CMenuItem::GetWidth() {
    CDDrawWorker* s = static_cast<CDDrawWorker*>(m_sprite);
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(2);
    if (!f) {
        return 0;
    }
    return f->m_height;
}
RVA(0x00185580, 0x4a)
i32 CMenuItem::NotifyCmd() {
    i32 id = m_cmdId;
    if (!id) {
        return id;
    }
    HWND wnd = m_host->m_wnd;
    if (wnd) {
        PostMessageA(wnd, WM_COMMAND, id, m_cmdParam);
    }
    if (m_secondaryCmdId && wnd) {
        PostMessageA(wnd, WM_COMMAND, m_secondaryCmdId, 0);
    }
    return 1;
}

RVA(0x001855d0, 0x6)
i32 CMenuItem::Detach() {
    return 1;
}

RVA(0x001855e0, 0x8)
i32 CMenuItem::Notify(u32) {
    return 1;
}

RVA(0x001855f0, 0x94)
i32 CMenuItem::Place(CDDrawSurfacePair* target, i32 x, i32 y) {
    CDDrawWorker* page = static_cast<CDDrawWorker*>(m_sprite);
    if (!page) {
        return 0;
    }

    if (m_fixedX != UNINIT_FILL) {
        x = m_fixedX;
        y = m_fixedY;
    }
    MenuItemState idx = m_state;
    CImage* row = page->GetAt(IDX(idx));
    if (!row) {
        return 0;
    }
    row->RenderFrame(target, x, y, 0);
    m_hitLeft = x - row->m_anchorX;
    m_hitRight = x + row->m_anchorX;
    m_hitTop = y - row->m_anchorY;
    m_hitBottom = y + row->m_anchorY;
    return 1;
}
RVA(0x00185690, 0x25)
i32 CMenuItem::Configure(i32 notify) {
    if (notify) {
        m_host->PlayFocusSound();
    }
    Disable(MENUSTATE_SELECTED);
    return 1;
}
RVA(0x001856c0, 0xd)
i32 CMenuItem::Release() {
    Disable(MENUSTATE_NORMAL);
    return 1;
}

RVA(0x001856d0, 0x25)
i32 CMenuItem::Trigger() {
    m_host->PlayActivationSound();
    NotifyCmd();
    m_host->ReplaceNode(m_key);
    return 1;
}
RVA(0x00185700, 0x4b)
i32 CMenuItem::Hit(i32 x, i32 y) {
    if (m_hitLeft == UNINIT_FILL) {
        return 0;
    }
    if (x < m_hitLeft) {
        return 0;
    }
    if (x > m_hitRight) {
        return 0;
    }
    if (y < m_hitTop) {
        return 0;
    }
    return y <= m_hitBottom;
}

RVA(0x00185750, 0x123)
i32 CMenuItem2::Init(
    CMenuPage* page,
    const char* name,
    const char* spriteKey,
    i32 cmdId,
    const char* key,
    i32 flags
) {
    if (!page) {
        return 0;
    }
    if (!CMenuItem::Init(page, name, spriteKey, cmdId, key, flags)) {
        return 0;
    }
    m_frameIdx = 0;
    m_frameCountdown = 0;
    m_frameDelay = 0x64;

    char buf[0x80];

    sprintf(buf, "%s_NORMAL", spriteKey);
    m_spriteNormal = LookupWorker(m_owner->m_imageRegistry->m_workersByName, buf);

    sprintf(buf, "%s_SELECTED", spriteKey);
    m_spriteSelected = LookupWorker(m_owner->m_imageRegistry->m_workersByName, buf);

    sprintf(buf, "%s_DISABLED", spriteKey);
    m_spriteDisabled = LookupWorker(m_owner->m_imageRegistry->m_workersByName, buf);

    return 1;
}
RVA(0x00185880, 0xe)
i32 CMenuItem2::GetFrameWidth() {
    CImage* f = GetCurrentFrame();
    if (!f) {
        return 0;
    }
    return f->m_width;
}

RVA(0x00185890, 0xe)
i32 CMenuItem2::GetWidth() {
    CImage* f = GetCurrentFrame();
    if (!f) {
        return 0;
    }
    return f->m_height;
}

RVA(0x001858a0, 0x2b)
i32 CMenuItem2::Notify(u32 a) {
    if (a >= static_cast<u32>(m_frameCountdown)) {
        m_frameCountdown = m_frameDelay;
        NextFrame();
        return 1;
    }
    m_frameCountdown = m_frameCountdown - a;
    return 1;
}

RVA(0x001858d0, 0x72)
i32 CMenuItem2::Place(CDDrawSurfacePair* target, i32 x, i32 y) {

    if (m_fixedX != UNINIT_FILL) {
        x = m_fixedX;
        y = m_fixedY;
    }
    CImage* f = GetCurrentFrame();
    if (!f) {
        return 0;
    }
    f->RenderFrame(target, x, y, 0);
    m_hitLeft = x - f->m_anchorX;
    m_hitRight = x + f->m_anchorX;
    m_hitTop = y - f->m_anchorY;
    m_hitBottom = y + f->m_anchorY;
    return 1;
}
RVA(0x00185950, 0x1b)
CDDrawWorker* CMenuItem2::GetCurrentSprite() {
    switch (m_state) {
        case MENUSTATE_NORMAL:
            return m_spriteNormal;
        case MENUSTATE_SELECTED:
            return m_spriteSelected;
        case MENUSTATE_DISABLED:
            return m_spriteDisabled;
    }
    return 0;
}

RVA(0x00185970, 0x4d)
CImage* CMenuItem2::GetCurrentFrame() {
    CDDrawWorker* s = GetCurrentSprite();
    if (!s) {
        return 0;
    }

    CImage* f = s->GetAt(m_frameIdx);
    if (f == NULL) {
        m_frameIdx = s->m_minIndex;
        f = s->GetAt(m_frameIdx);
    }
    return f;
}
RVA(0x001859c0, 0x4e)
i32 CMenuItem2::NextFrame() {
    if (!GetCurrentFrame()) {
        return 0;
    }
    m_frameIdx = m_frameIdx + 1;
    if (m_flags & 0x10000) {
        CDDrawWorker* s = GetCurrentSprite();
        if (s) {
            if (m_frameIdx > s->m_maxIndex) {
                m_frameIdx = m_frameIdx - 1;
                return 1;
            }
        }
    }
    return GetCurrentFrame() != NULL;
}

#define GRUNTZ_MENUITEM_TU
#include <rva.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Image/CImage.h>
#include <DDrawMgr/DDrawWorker.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuPage.h>
#include <Gruntz/MenuItem2.h>

#include <stdio.h>

VTBL(CMenuItem, 0x001f08c0);

VTBL(CMenuItem2, 0x001f08f8);

RVA(0x00184610, 0x20)
CString CMenuItem::GetUpName() {
    return m_upName;
}
RVA(0x00184630, 0x20)
CString CMenuItem::GetDownName() {
    return m_downName;
}
RVA(0x00184650, 0xa)
void CMenuItem::Disable(i32 mode) {
    m_state = mode;
}

RVA(0x00184660, 0x3)
i32 CMenuItem::OnInit() {
    return 0;
}

RVA_COMPGEN(0x00184670, 0x1e, ??_GCMenuItem@@UAEPAXI@Z)
RVA(0x00184690, 0x91)
inline CMenuItem::~CMenuItem() {
    Cleanup();
}
RVA(0x00184730, 0x41)
void CMenuItem::Reset() {
    m_host = 0;
    m_template = 0;
    m_sprite = 0;
    m_owner = 0;
    m_listPos = 0;
    m_hitLeft = static_cast<i32>(0xeeeeeeee);
    m_fixedX = static_cast<i32>(0xeeeeeeee);
    m_leftName.Empty();
    m_rightName.Empty();
    m_upName.Empty();
    m_downName.Empty();
}
RVA(0x00184780, 0x17)
void CMenuItem2::Disable(i32 mode) {
    i32 frameLimit = m_70;
    m_state = mode;
    m_frameIdx = 0;
    m_6c = frameLimit;
}

RVA(0x001847a0, 0xa)
void CMenuItem2::SetFrame(i32 v) {
    m_70 = v;
}
RVA(0x001847b0, 0x6)
i32 CMenuItem2::OnInit() {
    return 1;
}

RVA_COMPGEN(0x001847c0, 0x1e, ??_GCMenuItem2@@UAEPAXI@Z)
RVA(0x001847e0, 0xa6)
CMenuItem2::~CMenuItem2() {
    Cleanup();
}

RVA(0x00184890, 0x1a)
void CMenuItem2::Reset() {
    m_70 = 0x64;
    m_spriteNormal = 0;
    m_spriteSelected = 0;
    m_spriteDisabled = 0;
    m_frameIdx = 0;
    m_6c = 0;
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
    m_1c = 0;
    m_cmdParam = 0;
    if (m_flags & 1) {
        m_state = 3;
    } else {
        m_state = 1;
    }
    if (!OnInit()) {
        CObject* slot = 0;

        m_owner->m_imageRegistry->m_10map.Lookup(spriteKey, slot);
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
    if (m_1c && wnd) {
        PostMessageA(wnd, WM_COMMAND, m_1c, 0);
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

    if (m_fixedX != static_cast<i32>(0xeeeeeeee)) {
        x = m_fixedX;
        y = m_fixedY;
    }
    i32 idx = m_state;
    CImage* row;
    if (idx >= page->m_minIndex && idx <= page->m_maxIndex) {
        row = static_cast<CImage*>(page->m_items.GetAt(idx));
    } else {
        row = 0;
    }
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
    Disable(2);
    return 1;
}
RVA(0x001856c0, 0xd)
i32 CMenuItem::Release() {
    Disable(1);
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
    if (m_hitLeft == static_cast<i32>(0xeeeeeeee)) {
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

// @early-stop
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
    m_6c = 0;
    m_70 = 0x64;

    char buf[0x80];
    CObject* sprite;

    sprintf(buf, "%s_NORMAL", spriteKey);
    sprite = 0;
    m_owner->m_imageRegistry->m_10map.Lookup(buf, sprite);
    m_spriteNormal = static_cast<CDDrawWorker*>(sprite);

    sprintf(buf, "%s_SELECTED", spriteKey);
    sprite = 0;
    m_owner->m_imageRegistry->m_10map.Lookup(buf, sprite);
    m_spriteSelected = static_cast<CDDrawWorker*>(sprite);

    sprintf(buf, "%s_DISABLED", spriteKey);
    sprite = 0;
    m_owner->m_imageRegistry->m_10map.Lookup(buf, sprite);
    m_spriteDisabled = static_cast<CDDrawWorker*>(sprite);

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
    if (a >= static_cast<u32>(m_6c)) {
        m_6c = m_70;
        NextFrame();
        return 1;
    }
    m_6c = m_6c - a;
    return 1;
}

RVA(0x001858d0, 0x72)
i32 CMenuItem2::Place(CDDrawSurfacePair* target, i32 x, i32 y) {

    if (m_fixedX != static_cast<i32>(0xeeeeeeee)) {
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
        case 1:
            return m_spriteNormal;
        case 2:
            return m_spriteSelected;
        case 3:
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
    if (f == 0) {
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
    return GetCurrentFrame() != 0;
}

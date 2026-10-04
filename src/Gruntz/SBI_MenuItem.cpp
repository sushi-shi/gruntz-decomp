#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_MenuItem.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Globals.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>

// @early-stop
RVA(0x000e80e0, 0x8c)
i32 CSBI_MenuItem::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 frame,
    i32 unused
) {
    if (key == NULL) {
        return 0;
    }
    if (host != NULL && owner != NULL) {
        // Keep redraw and enable stores after the menu-specific setup.
        m_owner = owner;
        m_host = host;
        m_tab = tab;
        m_kind = SBI_KIND_MENU_ITEM;
        SetFrame(NULL);

        m_rect = rc;
        m_redrawFrames = 0;
        m_cmd = cmd;
        m_state = MENUITEM_NORMAL;
        SetEnabled(1);
        return ResolveFrame(key, frame) != 0;
    }
    return 0;
}

RVA(0x000e81a0, 0x8)
void CSBI_MenuItem::Reset() {
    SetFrame(NULL);
}

RVA(0x000e81c0, 0x8)
i32 CSBI_MenuItem::Refresh(i32) {
    return 1;
}

RVA(0x000e81e0, 0x8b)
i32 CSBI_MenuItem::ResolveFrame(const char* frameSetName, i32 frameIndex) {
    if (frameSetName == NULL) {
        return 0;
    }

    CDDrawWorker* frames = m_host->FindWorker(frameSetName);
    m_stateFrames = frames;
    if (frames == NULL) {
        return 0;
    }

    if (frameIndex == -1) {
        SetFrame(DDRAW_WORKER_FRAME_AT_UNCHECKED(frames, frames->GetMinIndex()));
    } else {
        SetFrame(frames->GetAt(frameIndex));
    }
    return m_frame != NULL;
}

RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CImage* image = m_frame;
        if (image) {
            i32 y = m_rect.top + image->GetAnchorY();
            i32 x = m_rect.left + image->GetAnchorX();
            image->RenderFrame(g_gameReg->World()->GetDrawTarget()->m_backPair, x, y, 0);
        }
    }
    return 1;
}

RVA(0x000e8310, 0x112)
i32 CSBI_MenuItem::SetState(SbiMenuItemState state, i32 playHighlightSound) {
    if (m_state == state || m_stateFrames == NULL) {
        return 0;
    }
    if (state == MENUITEM_HIGHLIGHT && m_state == MENUITEM_SELECTED) {
        return 1;
    }

    if (state == MENUITEM_SELECTED) {
        m_owner->ClearActiveTabContent();
        m_owner->SetActiveTab(static_cast<StatusBarTab>(IDX(m_cmd)));
        m_owner->BuildActiveTabContent();
        m_owner->RequestRedraw();
    } else if (state == MENUITEM_HIGHLIGHT && playHighlightSound) {

        PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_TABHIGHLIGHT2");
    }
    CDDrawWorker* frames = m_stateFrames;
    CImage* frame = frames->GetAt(IDX(state));
    SetFrame(frame);
    m_state = state;
    RequestRedraw();
    return 1;
}

RVA(0x000e8480, 0x4a)
i32 CSBI_MenuItem::ClearMatchingState(SbiMenuItemState stateToClear) {
    if (stateToClear == MENUITEM_NORMAL || m_stateFrames == NULL) {
        return 0;
    }
    if (stateToClear == MENUITEM_HIGHLIGHT && m_state == stateToClear) {
        return SetState(MENUITEM_NORMAL, 1);
    }
    if (stateToClear == MENUITEM_SELECTED && m_state == MENUITEM_SELECTED) {
        return SetState(MENUITEM_NORMAL, 1);
    }
    return 1;
}

RVA(0x000e84f0, 0x16)
i32 CSBI_MenuItem::ClearHighlight() {
    if (m_state != MENUITEM_HIGHLIGHT) {
        return 1;
    }
    return SetState(MENUITEM_NORMAL, 1);
}

RVA(0x000e8520, 0x152)
i32 CSBI_MenuItem::SerializeFields(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = g_gameReg->World();
    if (world == NULL) {
        return 0;
    }

    char frameSetName[SERIAL_NAME_LEN];
    switch (mode) {
        case SERIAL_LOAD:
            ar->Read(&m_state, sizeof(m_state));
            SERIAL_READ_WORKER(ar, world, frameSetName, m_stateFrames);
            break;
        case SERIAL_SAVE:
            ar->Write(&m_state, sizeof(m_state));
            SERIAL_WRITE_WORKER(ar, frameSetName, m_stateFrames);
            break;
    }

    return CSBI_Image::SerializeFields(ar, mode, typeId, payload) != 0;
}

RVA(0x0010bfa0, 0x1)
void CStatusBarItem::Reset() {}

RVA(0x0010bfc0, 0xe8)
i32 CStatusBarItem::SerializeFields(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->World();
    if (mgr == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD:
            ar->Read(&m_enabled, sizeof(m_enabled));
            ar->Read(&m_kind, sizeof(m_kind));
            ar->Read(&m_cmd, sizeof(m_cmd));
            ar->Read(&m_tab, sizeof(m_tab));
            ar->Read(&m_rect, sizeof(m_rect));
            ar->Read(&m_redrawFrames, sizeof(m_redrawFrames));
            break;
        case SERIAL_SAVE:
            ar->Write(&m_enabled, sizeof(m_enabled));
            ar->Write(&m_kind, sizeof(m_kind));
            ar->Write(&m_cmd, sizeof(m_cmd));
            ar->Write(&m_tab, sizeof(m_tab));
            ar->Write(&m_rect, sizeof(m_rect));
            ar->Write(&m_redrawFrames, sizeof(m_redrawFrames));
            break;
    }
    return 1;
}

#include <rva.h>

#include <Gruntz/SBI_MenuItem.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

static inline CDDrawWorker* LookupWorker(CDDrawSurfaceMgr* host, LPCTSTR name) {
    CObject* found = NULL;
    host->m_imageRegistry->m_workersByName.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

static inline SoundCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    SoundCue* found = NULL;
    MapLookup(cues, name, found);
    return found;
}

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
i32 CSBI_MenuItem::ResolveFrame(const char* key, i32 frameIndex) {
    if (key == NULL) {
        return 0;
    }

    CDDrawWorker* rec = LookupWorker(m_host->m_imageRegistry->m_workersByName, key);
    m_record = rec;
    if (rec == NULL) {
        return 0;
    }

    if (frameIndex == -1) {
        SetFrame(static_cast<CImage*>(rec->m_items.GetAt(rec->m_minIndex)));
    } else {
        SetFrame(rec->GetAt(frameIndex));
    }
    return m_frame != NULL;
}

RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CImage* f = m_frame;
        if (f) {
            i32 y = m_rect.top + f->m_anchorY;
            i32 x = m_rect.left + f->m_anchorX;
            f->RenderFrame(g_gameReg->m_world->m_drawTarget->m_backPair, x, y, 0);
        }
    }
    return 1;
}

RVA(0x000e8310, 0x112)
i32 CSBI_MenuItem::SetState(SbiMenuItemState state, i32 playHighlightSound) {
    if (m_state == state || m_record == NULL) {
        return 0;
    }
    if (state == MENUITEM_HIGHLIGHT && m_state == MENUITEM_SELECTED) {
        return 1;
    }

    if (state == MENUITEM_SELECTED) {
        m_owner->ClearTabGroup();
        m_owner->m_activeTab = static_cast<StatusBarTab>(IDX(m_cmd));
        m_owner->LoadTabSprites();
        m_owner->Deactivate();
    } else if (state == MENUITEM_HIGHLIGHT && playHighlightSound) {

        SoundCueRegistry* mh = g_gameReg->m_world->m_soundRegistry;
        if (mh->m_silentMode == false) {
            SoundCue* found = LookupCue(mh->m_cues, "GAME_TABHIGHLIGHT2");
            if (found) {
                b32 soundEnabled = g_soundEnabled;
                i32 volumePercent = g_soundVolumePercent;
                if (soundEnabled != false) {
                    SoundCue* p = found;
                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                        >= static_cast<u32>(p->m_replayDelayMs)) {
                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, false);
                    }
                }
            }
        }
    }
    CDDrawWorker* r = m_record;
    CImage* frame = r->GetAt(IDX(state));
    SetFrame(frame);
    m_state = state;
    RequestRedraw();
    return 1;
}

RVA(0x000e8480, 0x4a)
i32 CSBI_MenuItem::ProbeState(SbiMenuItemState state) {
    if (state == MENUITEM_NORMAL || m_record == NULL) {
        return 0;
    }
    if (state == MENUITEM_HIGHLIGHT && m_state == state) {
        return SetState(MENUITEM_NORMAL, 1);
    }
    if (state == MENUITEM_SELECTED && m_state == MENUITEM_SELECTED) {
        return SetState(MENUITEM_NORMAL, 1);
    }
    return 1;
}

RVA(0x000e84f0, 0x16)
i32 CSBI_MenuItem::Blit() {
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
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == NULL) {
        return 0;
    }

    char tmp[SERIAL_NAME_LEN];
    switch (mode) {
        case SERIAL_LOAD:
            ar->Read(&m_state, sizeof(m_state));
            g_serialCounter++;
            ar->Read(tmp, SERIAL_NAME_LEN);
            if (strlen(tmp) != 0) {
                m_record = LookupWorker(mgr, tmp);
            } else {
                m_record = NULL;
            }
            break;
        case SERIAL_SAVE:
            ar->Write(&m_state, sizeof(m_state));
            g_serialCounter++;
            memset(tmp, 0, sizeof(tmp));
            if (m_record) {
                strcpy(tmp, m_record->m_name);
            }
            ar->Write(tmp, SERIAL_NAME_LEN);
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
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
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

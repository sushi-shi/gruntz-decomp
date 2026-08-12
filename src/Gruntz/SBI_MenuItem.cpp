#include <rva.h>

#include <Gruntz/SBI_MenuItem.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>

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
    static_cast<void>(unused);
    if (key == NULL) {
        return 0;
    }
    if (host != NULL && owner != NULL) {
        m_owner = owner;
        m_host = host;
        m_tab = tab;
        m_kind = SBI_KIND_MENU_ITEM;
        m_frame = NULL;

        m_rect14 = rc;
        m_redrawFrames = 0;
        m_cmd = cmd;
        m_state = MENUITEM_NORMAL;
        m_enabled = 1;
        return ResolveFrame(key, frame) != 0;
    }
    return 0;
}

RVA(0x000e81a0, 0x8)
void CSBI_MenuItem::Reset() {
    m_frame = NULL;
}

RVA(0x000e81c0, 0x8)
i32 CSBI_MenuItem::Refresh(i32) {
    return 1;
}

// @early-stop
RVA(0x000e81e0, 0x8b)
i32 CSBI_MenuItem::ResolveFrame(const char* key, i32 a) {
    if (key == NULL) {
        return 0;
    }

    CObject* rec_v = 0;
    CDDrawSurfaceMgr* host = static_cast<CDDrawSurfaceMgr*>(m_host);
    host->m_imageRegistry->m_10map.Lookup(key, rec_v);
    CDDrawWorker* rec = static_cast<CDDrawWorker*>(rec_v);
    m_record = rec;
    if (rec == NULL) {
        return 0;
    }

    CDDrawWorker* r = rec;
    if (a == -1) {
        m_frame = static_cast<CImage*>(r->m_items.GetAt(r->m_minIndex));
    } else if (a >= r->m_minIndex && a <= r->m_maxIndex) {
        m_frame = static_cast<CImage*>(r->m_items.GetAt(a));
    } else {
        m_frame = NULL;
    }
    return m_frame != NULL;
}

RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CImage* f = m_frame;
        if (f) {
            i32 y = m_rect14.top + f->m_anchorY;
            i32 x = m_rect14.left + f->m_anchorX;
            f->RenderFrame(g_gameReg->m_world->m_drawTarget->m_backPair, x, y, 0);
        }
    }
    return 1;
}

RVA(0x000e8310, 0x112)
i32 CSBI_MenuItem::SetState(SbiMenuItemState state, i32 a) {
    if (m_state == state || m_record == NULL) {
        return 0;
    }
    if (state == MENUITEM_HIGHLIGHT && m_state == MENUITEM_SELECTED) {
        return 1;
    }

    if (state == MENUITEM_SELECTED) {
        m_owner->ClearTabGroup();
        // The domain ingest: CSBI_MenuItem::m_cmd is a general command id, but a
        // TAB menu item's command IS its tab - CStatusBarMgr bounds it at
        // `cmd <= 0 || cmd > TAB_LAST` before dispatching. Converted once, here.
        m_owner->m_activeTab = static_cast<StatusBarTab>(IDX(m_cmd));
        m_owner->LoadTabSprites();
        m_owner->Deactivate();
    } else if (state == MENUITEM_HIGHLIGHT && a) {

        CDDrawSubMgrLeafScan* mh = g_gameReg->m_world->m_soundRegistry;
        if (mh->m_emitGate == 0) {
            LeafCue* found = 0;
            void* foundP = 0;

            mh->m_cues.Lookup("GAME_TABHIGHLIGHT2", foundP);
            found = static_cast<LeafCue*>(foundP);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = found;
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    CDDrawWorker* r = m_record;
    CImage* frame;
    i32 frameIndex = IDX(state);
    if (frameIndex >= r->m_minIndex && frameIndex <= r->m_maxIndex) {
        frame = static_cast<CImage*>(r->m_items.GetAt(frameIndex));
    } else {
        frame = NULL;
    }
    m_frame = frame;
    m_state = state;
    SetSubtype();
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
i32 CSBI_MenuItem::SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == NULL) {
        return 0;
    }

    char tmp[SERIAL_NAME_LEN];
    switch (kind) {
        case SERIAL_LOAD:
            ar->Read(&m_state, sizeof(m_state));
            g_serialCounter++;
            ar->Read(tmp, SERIAL_NAME_LEN);
            if (strlen(tmp) != 0) {
                CObject* found_ob = 0;
                mgr->m_imageRegistry->m_10map.Lookup(tmp, found_ob);
                m_record = static_cast<CDDrawWorker*>(found_ob);
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

    return CSBI_Image::SerializeFields(ar, kind, a, b) != 0;
}

RVA(0x0010bfa0, 0x1)
void CStatusBarItem::Reset() {}

RVA(0x0010bfc0, 0xe8)
i32 CStatusBarItem::SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == NULL) {
        return 0;
    }
    switch (kind) {
        case SERIAL_LOAD:
            ar->Read(&m_enabled, sizeof(m_enabled));
            ar->Read(&m_kind, sizeof(m_kind));
            ar->Read(&m_cmd, sizeof(m_cmd));
            ar->Read(&m_tab, sizeof(m_tab));
            ar->Read(&m_rect14, sizeof(m_rect14));
            ar->Read(&m_redrawFrames, sizeof(m_redrawFrames));
            break;
        case SERIAL_SAVE:
            ar->Write(&m_enabled, sizeof(m_enabled));
            ar->Write(&m_kind, sizeof(m_kind));
            ar->Write(&m_cmd, sizeof(m_cmd));
            ar->Write(&m_tab, sizeof(m_tab));
            ar->Write(&m_rect14, sizeof(m_rect14));
            ar->Write(&m_redrawFrames, sizeof(m_redrawFrames));
            break;
    }
    return 1;
}

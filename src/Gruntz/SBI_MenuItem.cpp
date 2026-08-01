#define SBI_DTOR_CHAIN
#include <rva.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SerialCounter.h>
#include <Io/FileMem.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Mfc.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SBI_MenuItem.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/Sprite.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/StatusBarMgr.h>
#include <Image/CImage.h>

VTBL(CSBI_MenuItem, 0x001eab4c);

// @early-stop
RVA(0x000e80e0, 0x8c)
i32 CSBI_MenuItem::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 frame,
    i32 unused
) {
    static_cast<void>(unused);
    if (key == 0) {
        return 0;
    }
    if (host == 0 || owner == 0) {
        return 0;
    }
    m_2c = owner;
    m_24 = host;
    m_tab = tab;
    m_kind = 2;
    m_frame = 0;

    m_rect14 = rc;
    m_28 = 0;
    m_cmd = cmd;
    m_state = 1;
    m_enabled = 1;
    return ResolveFrame(key, frame) != 0;
}

RVA(0x000e81a0, 0x8)
void CSBI_MenuItem::Reset() {
    m_frame = 0;
}

RVA(0x000e81c0, 0x8)
i32 CSBI_MenuItem::Refresh(i32) {
    return 1;
}

// @early-stop
RVA(0x000e81e0, 0x8b)
i32 CSBI_MenuItem::ResolveFrame(const char* key, i32 a) {
    if (key == 0) {
        return 0;
    }

    CObject* rec_v = 0;
    CDDrawSurfaceMgr* host = static_cast<CDDrawSurfaceMgr*>(m_24);
    host->m_imageRegistry->m_10map.Lookup(key, rec_v);
    CDDrawWorker* rec = static_cast<CDDrawWorker*>(rec_v);
    m_record = rec;
    if (rec == 0) {
        return 0;
    }

    CDDrawWorker* r = rec;
    if (a == -1) {
        m_frame = static_cast<CImage*>(r->m_items.GetAt(r->m_minIndex));
    } else if (a >= r->m_minIndex && a <= r->m_maxIndex) {
        m_frame = static_cast<CImage*>(r->m_items.GetAt(a));
    } else {
        m_frame = 0;
    }
    return m_frame != 0;
}

RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::Render() {
    if (m_28 > 0) {
        m_28--;
        CImage* f = m_frame;
        if (f) {
            f->RenderFrame(
                g_gameReg->m_world->m_drawTarget->m_backPair,
                m_rect14.left + f->m_anchorX,
                m_rect14.top + f->m_anchorY,
                0
            );
        }
    }
    return 1;
}

RVA(0x000e8310, 0x112)
i32 CSBI_MenuItem::SetState(i32 state, i32 a) {
    if (m_state == state || m_record == 0) {
        return 0;
    }
    if (state == 2 && m_state == 3) {
        return 1;
    }

    if (state == 3) {
        m_2c->ClearTabGroup();
        m_2c->m_activeTab = m_cmd;
        m_2c->LoadTabSprites();
        m_2c->Deactivate();
    } else if (state == 2 && a) {

        CDDrawSubMgrLeafScan* mh = g_gameReg->m_world->m_soundRegistry;
        if (mh->m_emitGate == 0) {
            LeafCue* found = 0;
            void* foundP = 0;

            mh->m_10.Lookup("GAME_TABHIGHLIGHT2", foundP);
            found = static_cast<LeafCue*>(foundP);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = found;
                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    CDDrawWorker* r = m_record;
    CImage* frame;
    if (state >= r->m_minIndex && state <= r->m_maxIndex) {
        frame = static_cast<CImage*>(r->m_items.GetAt(state));
    } else {
        frame = 0;
    }
    m_frame = frame;
    m_state = state;
    SetSubtype();
    return 1;
}

RVA(0x000e8480, 0x4a)
i32 CSBI_MenuItem::ProbeState(i32 state) {
    if (state == 1 || m_record == 0) {
        return 0;
    }
    if (state == 2 && m_state == state) {
        return SetState(1, 1);
    }
    if (state == 3 && m_state == 3) {
        return SetState(1, 1);
    }
    return 1;
}

RVA(0x000e84f0, 0x16)
i32 CSBI_MenuItem::Blit() {
    if (m_state != 2) {
        return 1;
    }
    return SetState(1, 1);
}

RVA(0x000e8520, 0x152)
i32 CSBI_MenuItem::SerializeFields(CFileMemBase* ar, i32 kind, i32 a, i32 b) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }

    char tmp[0x80];
    switch (kind) {
        case 7:
            ar->Read(&m_state, 4);
            g_serialCounter++;
            ar->Read(tmp, 0x80);
            if (strlen(tmp) != 0) {
                CObject* found_ob = 0;
                mgr->m_imageRegistry->m_10map.Lookup(tmp, found_ob);
                m_record = static_cast<CDDrawWorker*>(found_ob);
            } else {
                m_record = 0;
            }
            break;
        case 4:
            ar->Write(&m_state, 4);
            g_serialCounter++;
            memset(tmp, 0, sizeof(tmp));
            if (m_record) {
                strcpy(tmp, m_record->m_name);
            }
            ar->Write(tmp, 0x80);
            break;
    }

    return CSBI_Image::SerializeFields(ar, kind, a, b) != 0;
}

RVA_COMPGEN(0x001007a0, 0x1e, ??_GCSBI_MenuItem@@UAEPAXI@Z)
RVA(0x001007d0, 0x7f)
CSBI_MenuItem::~CSBI_MenuItem() {
    Reset();
}

RVA(0x0010bfa0, 0x1)
void CStatusBarItem::Reset() {}

RVA(0x0010bfc0, 0xe8)
i32 CStatusBarItem::SerializeFields(CFileMemBase* ar, i32 kind, i32 a, i32 b) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }
    switch (kind) {
        case 7:
            ar->Read(&m_enabled, 4);
            ar->Read(&m_kind, 4);
            ar->Read(&m_cmd, 4);
            ar->Read(&m_tab, 4);
            ar->Read(&m_rect14, 0x10);
            ar->Read(&m_28, 4);
            break;
        case 4:
            ar->Write(&m_enabled, 4);
            ar->Write(&m_kind, 4);
            ar->Write(&m_cmd, 4);
            ar->Write(&m_tab, 4);
            ar->Write(&m_rect14, 0x10);
            ar->Write(&m_28, 4);
            break;
    }
    return 1;
}

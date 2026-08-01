#define SBI_DTOR_CHAIN
#define SBI_OWN_IMAGESET_DTOR
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialCounter.h>
#include <Io/FileMem.h>
#include <Mfc.h>
#include <Ints.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/Sprite.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SbiConfig.h>
#include <Image/CImage.h>

VTBL(CSBI_ImageSet, 0x001eac4c);

// @early-stop
RVA(0x000e72f0, 0xc4)
i32 CSBI_ImageSet::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 obj,
    RECT rect,
    const char* key,
    i32 frame,
    i32 extra
) {
    static_cast<void>(extra);

    if (host == 0 || owner == 0) {
        return 0;
    }
    m_2c = owner;
    m_tab = obj;
    m_24 = host;
    m_28 = 0;
    m_enabled = 1;

    m_rect14 = rect;
    m_cmd = cmd;
    if (key == 0) {
        return 0;
    }
    CObject* found = 0;
    host->m_imageRegistry->m_10map.Lookup(key, found);
    CDDrawWorker* rec = static_cast<CDDrawWorker*>(found);
    m_34 = rec;
    if (rec == 0) {
        return 0;
    }
    i32 f = frame;
    if (f == -1) {
        f = rec->m_minIndex;
    }
    m_38 = f;

    CImage* cel;
    if (f >= rec->m_minIndex && f <= rec->m_maxIndex) {
        cel = static_cast<CImage*>(rec->m_items.GetAt(f));
    } else {
        cel = 0;
    }
    m_frame = cel;
    return 1;
}

RVA(0x000e7400, 0x9)
void CSBI_ImageSet::Reset() {
    m_34 = 0;
    m_frame = 0;
}

RVA(0x000e7420, 0x8)
i32 CSBI_ImageSet::Refresh(i32) {
    return 1;
}

RVA(0x000e7440, 0x5e)
i32 CSBI_ImageSet::Render() {
    if (m_28 > 0) {
        m_28--;
        CDDrawWorker* tbl = m_34;
        CImage* cel;
        if (m_38 >= tbl->m_minIndex && m_38 <= tbl->m_maxIndex) {
            cel = static_cast<CImage*>(tbl->m_items.GetAt(m_38));
        } else {
            cel = 0;
        }
        m_frame = cel;
        if (cel != 0) {
            cel->RenderFrame(
                g_gameReg->m_world->m_drawTarget->m_backPair,
                cel->m_anchorX + m_rect14.left,
                cel->m_anchorY + m_rect14.top,
                0
            );
        }
    }
    return 1;
}

RVA(0x000e74c0, 0x16)
void CSBI_ImageSet::Notify(i32 id) {
    if (id != -1) {
        m_38 = id;
    }
    m_28 = 2;
}

RVA(0x000e74f0, 0x152)
i32 CSBI_ImageSet::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }
    char buf[0x80];
    switch (mode) {
        case 7:
            s->Read(&m_38, 4);
            g_serialCounter++;
            s->Read(buf, 0x80);
            if (strlen(buf)) {
                CDDrawWorker* out;

                CObject* outOb = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, outOb);
                out = static_cast<CDDrawWorker*>(outOb);
                m_34 = out;
            } else {
                m_34 = 0;
            }
            break;
        case 4:
            s->Write(&m_38, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            if (m_34) {
                strcpy(buf, m_34->m_name);
            }
            s->Write(buf, 0x80);
            break;
    }

    return CSBI_Image::SerializeFields(s, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x00101fd0, 0x1e, ??_GCSBI_ImageSet@@UAEPAXI@Z)
RVA(0x00102000, 0x7f)
CSBI_ImageSet::~CSBI_ImageSet() {
    Reset();
}

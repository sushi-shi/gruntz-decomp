#include <rva.h>
#include <Pix16.h>
#include <AddrWord.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/ParseSource.h>
#include <Win32.h>
#include <ddraw.h>
#include <string.h>
#include <stdio.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerCtx.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Io/FileMem.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/String.h>
#include <Mfc.h>
#include <Gruntz/ResolveNode.h>
#include <Image/ImageSet.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/AniRecord.h>
#include <Image/ImageFormatTag.h>

RVA(0x0003a1d0, 0x1d)
void CDDrawSurfacePair::BltSelf(CDDrawSurfacePair* src) {
    m_surface->BltFast(0, 0, src->m_surface, &src->m_srcRect, 0x10);
}

RVA(0x0006b270, 0x1b)
CObject* CAniElement::AtChecked(i32 i) const {
    if (i >= 0 && i < m_records.GetSize()) {
        return m_records.GetAt(i);
    }
    return 0;
}

RVA(0x00163bc0, 0x2c)
void CDDrawWorkerList::Unload() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
}

RVA(0x00163bf0, 0x6d)
void CDDrawWorkerList::PruneWorkers(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        POSITION cur = pos;
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        child->RenderFrame(a, b);
        child->m_refCount--;
        if ((b->m_surface != 0 && (b->m_flags & 0x20000) == 0) || child->m_refCount <= 0) {
            m_workers.RemoveAt(cur);
            if (child) {
                delete child;
            }
        }
    }
}

RVA(0x00163c60, 0x2c)
void CDDrawWorkerList::ClearWorkers() {
    POSITION pos = m_workers.GetHeadPosition();
    while (pos) {
        CDDrawWorkerBase* child = static_cast<CDDrawWorkerBase*>(m_workers.GetNext(pos));
        if (child) {
            delete child;
        }
    }
    m_workers.RemoveAll();
}

// @early-stop
RVA(0x00163c90, 0x116)
i32 CDDrawSurfacePair::Create(i32 w, i32 h, i32 bpp, i32 flags) {
    m_flags = flags;
    if (w <= 0 || h <= 0) {

        if (m_id == 1) {
            if (OwnerMgr()->m_lastError == 0) {
                OwnerMgr()->m_lastError = 0xfa1;
            }
        } else {
            if (OwnerMgr()->m_lastError == 0) {
                OwnerMgr()->m_lastError = 0xfa2;
            }
        }
        return 0;
    }
    i32 kind = m_id;

    m_width = w;
    m_height = h;
    m_bpp = bpp;
    i32* rect = m_srcRect;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = w;
    rect[3] = h;
    if (kind == 1) {
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        m_surface = static_cast<CDDSurface*>(mgr->m_ptrColl->CreatePoolItem(
            static_cast<void*>(mgr->m_drawTarget->m_frontPair->m_surface),
            4
        ));
        if (m_surface == 0) {
            if (OwnerMgr()->m_lastError == 0) {
                OwnerMgr()->m_lastError = 0xfa3;
            }
            return 0;
        }
    }
    if (m_id != 1) {
        if (m_flags & 0x10000) {
            m_surface = OwnerMgr()->m_ptrColl->MakeAndAddB(w, h, 0, 0, -1);
        } else {
            m_surface = OwnerMgr()->m_ptrColl->CreateKeyedSurface(w, h, 0, 0, -1);
        }
        if (m_surface == 0) {
            if (OwnerMgr()->m_lastError == 0) {
                OwnerMgr()->m_lastError = 0xfa4;
            }
            return 0;
        }
    }
    m_ownsSurface = 1;
    return 1;
}

// @early-stop
RVA(0x00163db0, 0x64)
i32 CDDrawSurfacePair::InitFromSurface(CDDSurface* src) {

    if (src == 0) {
        return 0;
    }
    i32 w = src->m_width;
    i32 bpp = src->m_bitDepth;
    i32 h = src->m_height;
    if (w <= 0 || h <= 0) {
        return 0;
    }
    m_width = w;
    m_srcRect[2] = w;
    m_height = h;
    m_bpp = bpp;
    m_srcRect[0] = 0;
    m_srcRect[1] = 0;
    m_srcRect[3] = h;
    m_id = 0x63;
    m_surface = src;
    m_ownsSurface = 0;
    return 1;
}

RVA(0x00163e20, 0x2d)
void CDDrawSurfacePair::Unload() {
    if (m_surface != 0 && m_ownsSurface != 0) {
        CDDrawPtrCollections* pool = OwnerMgr()->m_ptrColl;
        pool->RemoveItemA(m_surface);
        m_surface = 0;
    }
    m_width = 0;
}

RVA(0x00163e50, 0x8b)
i32 CDDrawSurfacePair::LoadImage(CParseSource* src) {
    i32 type;
    switch (static_cast<u32>(src->GetEntryTag())) {
        case 0x424d50:
            type = 1;
            break;
        case 0x504358:
            type = 2;
            break;
        case 0x524944:
            type = 3;
            break;
        case 0x504944:
            type = 4;
            break;
        default:
            return 0;
    }
    char* buf = src->BeginParse();
    if (buf == 0) {
        return 0;
    }
    i32 r = m_surface->Resolve(OwnerMgr()->m_ptrColl, buf, type, src->m_length, 0);
    src->EndParse();
    return r;
}

RVA(0x00163ee0, 0x19)
i32 CDDrawSurfacePair::ResolveImageName(char* name) {
    return m_surface->MakeImageKey(OwnerMgr()->m_ptrColl, name, 0);
}

RVA(0x00163f00, 0x40)
i32 CDDrawSurfacePair::RestoreIfLost() {
    if (m_surface == 0) {
        return 1;
    }
    IDirectDrawSurface* s = m_surface->m_ddSurface;
    if (s != 0 && s->IsLost() == 0) {
        return 1;
    }

    CDDSurface* held = m_surface;
    IDirectDrawSurface* r = held->m_ddSurface;

    i32 hr = r->Restore();
    return hr == 0;
}

// @early-stop
RVA(0x00163f40, 0x23e)
void CDDrawSurfacePair::DrawBox(RECT* rect, i32 color) {

    if (rect->left < 0 || rect->left >= m_width) {
        return;
    }
    if (rect->top < 0 || rect->top >= m_height) {
        return;
    }
    if (rect->right < 0 || rect->right >= m_width) {
        return;
    }
    if (rect->bottom < 0 || rect->bottom >= m_height) {
        return;
    }
    char* base = static_cast<char*>(m_surface->Lock(0));
    if (base == 0) {
        return;
    }

    u8 c = static_cast<u8>(color);
    i32 w = rect->right - rect->left + 1;

    if (m_bpp == 0x10) {
        i32 n = 2 * w;
        i32 offTop = m_surface->m_bytesPerPixel * rect->left + m_surface->m_pitch * rect->top;
        if (n > 0) {
            memset(base + offTop, color, n);
        }
        i32 offBot = m_surface->m_bytesPerPixel * rect->left + m_surface->m_pitch * rect->bottom;
        if (n > 0) {
            memset(base + offBot, color, n);
        }
    } else {
        i32 offTop = m_surface->m_bytesPerPixel * rect->left + m_surface->m_pitch * rect->top;
        if (w > 0) {
            memset(base + offTop, color, w);
        }
        i32 offBot = m_surface->m_bytesPerPixel * rect->left + m_surface->m_pitch * rect->bottom;
        if (w > 0) {
            memset(base + offBot, color, w);
        }
    }

    {
        i32 h = rect->bottom - rect->top + 1;
        for (i32 y = 0; y < h; ++y) {
            if (m_bpp == 0x10) {
                i32 lo =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->left;
                base[lo] = c;
                base[lo + 1] = c;
                i32 ro =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->right;
                base[ro] = c;
                base[ro + 1] = c;
            } else {
                i32 lo =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->left;
                base[lo] = c;
                i32 ro =
                    (rect->top + y) * m_surface->m_pitch + m_surface->m_bytesPerPixel * rect->right;
                base[ro] = c;
            }
        }
    }

    m_surface->m_ddSurface->Unlock(0);
}

// @early-stop
RVA(0x00164180, 0xcd)
void CDDrawSurfacePair::DrawCross(i32 x, i32 y) {
    if (x - 4 < 0) {
        return;
    }
    if (x + 4 >= m_width) {
        return;
    }
    if (y - 4 < 0) {
        return;
    }
    if (y + 4 >= m_height) {
        return;
    }
    char* base = static_cast<char*>(m_surface->Lock(0));
    if (base == 0) {
        return;
    }
    i32 off = m_surface->m_bytesPerPixel * x + m_surface->m_pitch * y;

    i32 i;
    char* p = base + off - 1;
    for (i = 0; i < 3; ++i) {
        *p = 0;
        --p;
    }

    for (i = 1; i <= 3; ++i) {
        base[off + i] = 0;
    }

    i32 up = off - m_surface->m_pitch;
    for (i = 0; i < 3; ++i) {
        base[up] = static_cast<char>(0xff);
        up -= m_surface->m_pitch;
    }

    i32 down = off + m_surface->m_pitch;
    for (i = 0; i < 3; ++i) {
        base[down] = static_cast<char>(0xff);
        down += m_surface->m_pitch;
    }

    m_surface->m_ddSurface->Unlock(0);
}

// @early-stop
RVA(0x00164250, 0x12b)
i32 CDDrawSurfacePair::SetGeom(i32 w, i32 h, i32 bpp) {
    if (m_width != w || m_height != h || m_bpp != bpp) {
        i32 sysmem;
        if (m_id == 2) {
            DDSCAPS caps;
            if (0 == m_surface->m_ddSurface->GetCaps(&caps)) {
                sysmem = 0x800 & caps.dwCaps;
            } else {
                sysmem = 0;
            }
        }
        OwnerMgr()->m_ptrColl->RemoveItemA(m_surface);
        m_surface = 0;
        if (m_id == 1) {
            CDDrawSurfaceMgr* mgr = OwnerMgr();
            m_surface = static_cast<CDDSurface*>(mgr->m_ptrColl->CreatePoolItem(
                static_cast<void*>(mgr->m_drawTarget->m_frontPair->m_surface),
                4
            ));
            if (m_surface == 0) {
                return 0;
            }
        }
        if (m_id != 1) {
            if (sysmem != 0) {
                m_surface = OwnerMgr()->m_ptrColl->MakeAndAddB(w, h, bpp, 0, -1);
            } else {
                m_surface = OwnerMgr()->m_ptrColl->CreateKeyedSurface(w, h, bpp, 0, -1);
            }
            if (m_surface == 0) {
                return 0;
            }
        }
        if (w > 0 && h > 0 && (8 == bpp || bpp == 16 || bpp == 24 || 32 == bpp)) {
            m_srcRect[0] = 0;
            m_srcRect[1] = 0;
            m_width = w;
            m_height = h;
            m_bpp = bpp;
            m_srcRect[2] = w;
            m_srcRect[3] = h;
            return 1;
        }
        return 0;
    }
    return 1;
}

RVA(0x00164380, 0x98)
void CDDrawSurfacePair::DrawCount(RECT* rc, i32 n) {
    char buf[0x20];
    sprintf(buf, "%i", n);
    CDDSurface* w = m_surface;
    if (!w) {
        return;
    }
    HDC hdc = 0;
    w->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0xffffff);
    DrawTextA(hdc, buf, strlen(buf), rc, 0x25);
    w->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x00164420, 0x79)
void CDDrawSurfacePair::DrawLabel(RECT* rc, char* text) {
    CDDSurface* w = m_surface;
    if (!w) {
        return;
    }
    HDC hdc = 0;
    w->m_ddSurface->GetDC(&hdc);
    if (!hdc) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0xffffff);
    DrawTextA(hdc, text, strlen(text), rc, 0x25);
    w->m_ddSurface->ReleaseDC(hdc);
}

// @early-stop
RVA(0x001644a0, 0x1b0)
i32 CDDrawSurfaceChildA::SetGeometry(i32 w, i32 h, i32 bpp) {
    CDDrawSurfaceMgr* mgr = OwnerMgr();
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    CDDrawPtrCollections* pool = mgr->m_ptrColl;
    i32 mode = 0x11;
    if (w <= 0x140) {
        mode = 0x51;
    }
    i32 hr;
    if (mgr->m_flags & 0x10) {

        AddrWord<char> windowSlot;
        windowSlot.m_word = 2;
        hr =
            pool->CreateDevice(static_cast<void*>(mgr->m_hWnd), windowSlot.m_addr, w, h, bpp, mode);
    } else {
        hr = pool->CreateDevice(
            static_cast<void*>(mgr->m_hWnd),
            static_cast<void*>(0),
            w,
            h,
            bpp,
            mode
        );
    }
    if (hr == 0) {
        i32 err = pool->m_lastError;
        if (err != 0) {
            switch (err) {
                case 0x3e9: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80e9;
                    }
                    return 0;
                }
                case 0x3ea: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ea;
                    }
                    return 0;
                }
                case 0x3eb: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80eb;
                    }
                    return 0;
                }
                case 0x3ec: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ec;
                    }
                    return 0;
                }
                case 0x3ed: {
                    CDDrawSurfaceMgr* m = OwnerMgr();
                    if (m->m_lastError == 0) {
                        m->m_lastError = 0x80ed;
                    }
                    return 0;
                }
            }
            CDDrawSurfaceMgr* md = OwnerMgr();
            if (md->m_lastError == 0) {
                md->m_lastError = 0xbb9;
            }
            return 0;
        }
        CDDrawSurfaceMgr* m4 = OwnerMgr();
        if (m4->m_lastError == 0) {
            m4->m_lastError = 0xbb9;
        }
        return 0;
    }
    CDDrawSurfaceMgr* m2 = OwnerMgr();
    i32 amode = 1;
    if (m2->m_flags & 2) {
        amode = 2;
    }
    CDDSurface* surf = pool->Create24BitPaletteSurface(amode);
    m_surface = surf;
    if (surf != 0 && surf->IsValid()) {
        return 1;
    }
    CDDrawSurfaceMgr* m3 = OwnerMgr();
    if (m3->m_lastError == 0) {
        m3->m_lastError = 0xbba;
    }
    return 0;
}

RVA(0x00164650, 0x3)
void CDDrawSurfacePair::BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size) {}

RVA(0x00164660, 0x46)
i32 CDrawSubWorker::Probe() {
    CDDSurface* s = m_surface;
    if (s != 0) {
        IDirectDrawSurface* dd = s->m_ddSurface;
        if (dd == 0 || dd->IsLost() != 0) {
            s = m_surface;
            if (s->m_ddSurface->Restore() != 0) {
                s = m_surface;
                if (s->m_ddSurface->Restore() != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

RVA(0x001646b0, 0xde)
i32 CDDrawSurfaceChildA::SetGeom(i32 w, i32 h, i32 bpp) {
    if (m_width == w && m_height == h && m_bpp == bpp) {
        return 1;
    }
    CDDrawPtrCollections* pool = OwnerMgr()->m_ptrColl;
    if (pool == 0) {
        return 0;
    }
    pool->RemoveItemA(m_surface);
    m_surface = 0;
    if (pool->ConfigureSurface(w, h, bpp, 0, 0) != 0) {
        return 0;
    }
    i32 amode = 1;
    if (OwnerMgr()->m_flags & 2) {
        amode = 2;
    }
    m_surface = pool->Create24BitPaletteSurface(amode);
    if (m_surface == 0) {
        return 0;
    }
    if (!m_surface->IsValid()) {
        return 0;
    }
    if (w > 0 && h > 0 && (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)) {
        m_bpp = bpp;
        m_width = w;
        m_height = h;
        m_srcRect[0] = 0;
        m_srcRect[1] = 0;
        m_srcRect[2] = w;
        m_srcRect[3] = h;
        return 1;
    }
    return 0;
}

// @early-stop
RVA(0x00164790, 0x41)
i32 CResolveNode::SetPosition(i32 x, i32 y) {
    m_screenX = x;
    m_plotDX = 0;
    m_plotDY = 0;
    m_stateFlags = 0;
    m_flashCountdown = 0;
    m_drawFillArg = 0;
    m_drawActive = 0;
    m_screenY = y;
    m_flashInterval = 0x32;
    m_drawFillCmd = 1;
    m_level = OwnerMgr()->m_level;
    return 1;
}

RVA(0x001647e0, 0x48)
i32 CResolveNode::Init(
    CDDrawSurfaceMgr* owner,
    i32 field04,
    i32 resolveX,
    i32 resolveY,
    i32 field40,
    i32 field08
) {
    m_ownerCtx = owner;
    m_id = field04;
    m_flags = field08;
    m_drawFillArg = 0;
    m_drawActive = 0;
    m_drawFillCmd = 1;
    SetPosition(resolveX, resolveY);
    m_stateFlags = field40;
    return 1;
}

RVA(0x00165210, 0xa2)
void CDDrawWorkerCache::Unload() {
    CObject* val = 0;
    POSITION pos = m_workers.GetStartPosition();
    CString key;
    if (pos != 0) {
        do {
            m_workers.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CDDrawWorker*>(val));
            }
        } while (pos != 0);
    }
    m_workers.RemoveAll();
}

RVA(0x001652c0, 0x92)
void* CDDrawWorkerCache::CreateWorker(GameObjNotifyFn factory, const char* key, i32 flags) {

    AnimWorkerObj* w = new AnimWorkerObj(OwnerMgr(), m_workers.GetCount());

    if (w->Init(factory, flags) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_workers[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165360, 0xf1)
CString CDDrawWorkerCache::FindKeyOfValue(CObject* target) {
    CObject* val = 0;
    POSITION pos = m_workers.GetStartPosition();
    CString key;
    while (pos != 0) {
        m_workers.GetNextAssoc(pos, key, val);

        if (val != 0
            && static_cast<AnimWorkerObj*>(val)->m_notify
                   == static_cast<AnimWorkerObj*>(target)->m_notify) {
            return key;
        }
    }
    CString empty;
    return empty;
}

// @early-stop
RVA(0x00165460, 0x156)
i32 CAniElement::Build(void* ctx, CAniSource* src, i32 flags) {
    m_flags = flags;
    m_scale = 1.0f;
    m_total = 0;
    m_flags = src->m_flags | flags;

    const char* cursor = src->m_data;
    if (src->m_namelen != 0) {
        m_name = static_cast<char*>(operator new(src->m_namelen + 2));
        u32 n;
        for (n = 0; n < src->m_namelen; n++) {
            m_name[n] = *cursor++;
        }
        m_name[n] = 0;
    } else {
        m_name = 0;
    }

    CAniRecordView* rec = 0;
    i32 i;
    for (i = 0; i < src->m_count; i++) {
        rec = new CAniRecordView;

        Pix16CPtr head;
        head.m_chars = cursor;
        if (rec->Parse(ctx, head.m_swords) == 0) {
            goto fail;
        }
        m_records.SetAtGrow(m_records.GetSize(), static_cast<CObject*>(rec));
        cursor += g_aniParsedNameLen + 0x14;
        m_total += rec->GetSize();
    }
    return 1;

fail:
    if (rec != 0) {
        delete rec;
    }
    for (i = 0; i < m_records.GetSize(); i++) {
        CObject* p = m_records.GetAt(i);
        if (p != 0) {
            delete (static_cast<CAniRecordView*>(p));
        }
    }
    if (m_name != 0) {
        ::operator delete(m_name);
        m_name = 0;
    }
    m_records.SetSize(0, -1);
    return 0;
}

RVA(0x001655c0, 0x53)
i32 CAniElement::Configure(void* ctx, void* entry, i32 flags) {
    if ((static_cast<CParseSource*>(entry))->GetEntryTag() != 0x414e49) {
        return 0;
    }
    m_flags = flags;
    void* src = (static_cast<CParseSource*>(entry))->BeginParse();
    if (src == 0) {
        return 0;
    }
    i32 r = Build(ctx, static_cast<CAniSource*>(src), 0);
    (static_cast<CParseSource*>(entry))->EndParse();
    return r;
}

RVA(0x00165620, 0x101)
i32 CAniElement::LoadFile(void* ctx, void* filename, i32 unused) {
    CFile fr;
    if (fr.Open(static_cast<const char*>(filename), 0, 0) == 0) {
        return 0;
    }
    u32 size = fr.GetLength();
    void* buf = ::operator new(size);
    if (fr.Read(buf, size) == 0) {
        ::operator delete(buf);
        return 0;
    }
    i32 r = Build(ctx, static_cast<CAniSource*>(buf), 0);
    ::operator delete(buf);
    return r;
}

RVA(0x00165730, 0x4c)
void CAniElement::DeleteAll() {
    for (i32 i = 0; i < m_records.GetSize(); i++) {
        CAniRecordView* el = static_cast<CAniRecordView*>(m_records.GetAt(i));
        if (el != 0) {
            delete el;
        }
    }
    if (m_name != 0) {
        ::operator delete(m_name);
        m_name = 0;
    }
    m_records.SetSize(0, -1);
}

RVA(0x00165810, 0xa9)
void CDDrawWorkerMapSmall::Unload() {
    CObject* val = 0;
    POSITION pos = m_map1.GetStartPosition();
    CString key;
    if (pos != 0) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CAniRecordBase2*>(val));
            }
        } while (pos != 0);
    }
    m_map1.RemoveAll();
    m_cachedWorker = 0;
}

RVA(0x001658c0, 0xcc)
void* CDDrawWorkerMapSmall::LoadPaletteFromSource(CParseSource* src, const char* key, i32 flags) {
    char* data = src->BeginParse();
    if (data == 0) {
        return 0;
    }
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromRgb(data, flags) == 0) {
        src->EndParse();
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    src->EndParse();
    char buf[0x50];
    if (key != 0) {
        strcpy(buf, key);
    } else {
        strcpy(buf, src->m_name);
    }
    m_map1[buf] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165990, 0x77)
void* CDDrawWorkerMapSmall::CreateWorkerFromData(void* data, const char* key, i32 flags) {
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromRgb(data, flags) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_map1[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165a10, 0x77)
void* CDDrawWorkerMapSmall::CreateWorkerFromFile(char* path, const char* key, i32 flags) {
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->LoadPaletteFromFile(path, flags) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }
    m_map1[key] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165a90, 0xf4)
void* CDDrawWorkerMapSmall::LoadSizedPaletteFromSource(CParseSource* src, i32 key, i32 flags) {
    if (src->GetEntryTag() != IMGTAG_XCP) {
        return 0;
    }
    char* data = src->BeginParse();
    if (data == 0) {
        return 0;
    }

    AddrWord<char> handle;
    handle.m_addr = const_cast<char*>(src->m_keyHandle);
    CAniRecordBase2* w = new CAniRecordBase2(m_map1.GetCount(), m_ownerCtx);
    if (w->CreatePaletteFromTrailingData(data, handle.m_word, flags) == 0) {
        if (w != 0) {
            delete w;
        }
        return 0;
    }

    AddrWord<char> keyArg;
    keyArg.m_word = key;
    char buf[0x50];
    if (keyArg.m_addr != 0) {
        strcpy(buf, keyArg.m_addr);
    } else {
        strcpy(buf, src->m_name);
    }
    m_map1[buf] = static_cast<CObject*>(w);
    return w;
}

RVA(0x00165b90, 0xa9)
void CDDrawWorkerMapSmall::ResetSlots() {
    CObject* val = 0;
    POSITION pos = m_map1.GetStartPosition();
    CString key;
    if (pos != 0) {
        do {
            m_map1.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CAniRecordBase2*>(val));
            }
        } while (pos != 0);
    }
    m_map1.RemoveAll();
    m_cachedWorker = 0;
}

RVA(0x00165c40, 0xe7)
i32 CDDrawWorkerMapSmall::RemoveByValue(CObject* obj) {
    CAniRecordBase2* w = static_cast<CAniRecordBase2*>(obj);
    if (m_cachedWorker == w) {
        m_cachedWorker = 0;
    }
    CObject* val = 0;
    POSITION pos = m_map1.GetStartPosition();
    CString key;
    while (pos != 0) {
        m_map1.GetNextAssoc(pos, key, val);
        if (val == obj) {
            m_map1.RemoveKey(key);
            if (w != 0) {
                delete w;
            }
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00165d30, 0x5f)
i32 CDDrawWorkerMapSmall::RemoveByKey(const char* key) {
    CObject* val = 0;
    m_map1.Lookup(key, val);

    CAniRecordBase2* w = static_cast<CAniRecordBase2*>(val);
    if (val == 0) {
        return 0;
    }
    if (m_cachedWorker == w) {
        m_cachedWorker = 0;
    }
    m_map1.RemoveKey(key);
    delete w;
    return 1;
}

RVA(0x00165e30, 0x27)
i32 CFileMemBase::SetName(const char* name, i32 a, i32 b) {
    m_name = name;
    m_mode = a;
    m_4 = b;
    return 1;
}

RVA(0x00165e60, 0x82)
i32 CFileMem::Open() {
    if (m_name.GetLength() == 0) {
        return 0;
    }

    if (WantRead()) {
        CFile* io = &m_file;
        if (!io->Open(m_name, 0, 0)) {
            return 0;
        }
        m_length = io->GetLength();
        m_offset = 0;
        return 1;
    }

    CFile* out = &m_file;
    if (!out->Open(m_name, 0x1001, 0)) {
        return 0;
    }
    m_length = 0;
    m_offset = 0;
    return 1;
}

RVA(0x00165ef0, 0xf)
i32 CFileMem::Ready() {
    CFile* io = &m_file;
    io->Close();
    return 1;
}

RVA(0x00165f00, 0x48)
i32 CFileMem::Read(void* buf, i32 n) {
    if (buf == 0) {
        return 0;
    }
    if (n == 0) {
        return 0;
    }
    CFile* io = &m_file;
    if (io->Read(buf, n) != static_cast<u32>(n)) {
        return 0;
    }
    m_offset += n;
    return 1;
}

RVA(0x00165f50, 0x45)
i32 CFileMem::Write(const void* buf, i32 n) {
    if (buf == 0) {
        return 0;
    }
    if (n == 0) {
        return 0;
    }
    CFile* io = &m_file;
    io->Write(buf, n);
    m_length += n;
    m_offset += n;
    return 1;
}

RVA(0x00165fa0, 0x93)
void CDDrawWorkerA::RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    {

        char c = m_pixelValue;
        i32 y = m_screenY;
        i32 x = m_screenX;
        CDDSurface* s = b->m_surface;
        char* base = static_cast<char*>(s->Lock(0));
        if (base != 0) {
            base[s->m_bytesPerPixel * x + s->m_pitch * y] = c;
            s->m_ddSurface->Unlock(0);
        }
    }
    {
        char c = m_pixelValue;
        i32 y = m_screenY;
        i32 x = m_screenX;
        CDDSurface* s = a->m_surface;
        char* base = static_cast<char*>(s->Lock(0));
        if (base != 0) {
            base[s->m_bytesPerPixel * x + y * s->m_pitch] = c;
            s->m_ddSurface->Unlock(0);
        }
    }
}

// @early-stop
RVA(0x00166040, 0x66)
i32 CDDrawWorkerB::Helper(const char* key, i32 idx) {
    CObject* obj = 0;
    OwnerMgr()->m_imageRegistry->m_10map.Lookup(key, obj);

    CDDrawWorker* p = static_cast<CDDrawWorker*>(obj);
    CImage* v;
    if (p != 0 && idx >= p->m_minIndex && idx <= p->m_maxIndex) {

        v = static_cast<CImage*>(p->m_items.GetAt(idx));
    } else {
        v = 0;
    }
    m_frame = v;
    return v != 0;
}

RVA(0x001660b0, 0x33)
void CDDrawWorkerB::RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) {
    m_frame->RenderImage(this, a);
    if (b->m_surface != 0 && (b->m_flags & 0x20000) == 0) {
        m_frame->RenderImage(this, b);
    }
}

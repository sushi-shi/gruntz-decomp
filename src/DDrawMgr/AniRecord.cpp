#include <rva.h>
#include <AddrWord.h>
#include <Pix16.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Wap32/Object.h>
#include <Mfc.h>
#include <Gruntz/AniRecordView.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/AniRecordBase2.h>
#include <DDrawMgr/AniRecordViews.h>
#include <string.h>
#include <DDrawMgr/AniRecord.h>

VTBL(CAniRecordView, 0x001f02c0);
VTBL(CAniRecordBase2, 0x001f02d8);
DATA(0x002bf3c4)
i32 g_aniParsedNameLen = 0;

RVA_COMPGEN(0x00165780, 0x1e, ??_GCAniRecordView@@UAEPAXI@Z)
RVA(0x001657a0, 0x66)
CAniRecordView::~CAniRecordView() {
    CAniRecordView* r = this;
    if (r->m_indices != 0) {
        ::operator delete(r->m_indices);
    }
    r->m_owner = 0xffff;
    r->m_count = 0;
    r->m_indices = 0;
}

RVA(0x00165d90, 0xb)
i32 CAniRecordBase2::IsLoaded() {
    return m_buf != 0;
}

RVA(0x00165da0, 0x6)
i32 CAniRecordBase2::GetClassId() {
    return 0x15;
}

RVA_COMPGEN(0x00165db0, 0x1e, ??_GCAniRecordBase2@@UAEPAXI@Z)
RVA(0x00165dd0, 0x5b)
CAniRecordBase2::~CAniRecordBase2() {

    Unload();
}

RVA(0x00168c60, 0xa0)
i32 CAniRecordView::Parse(void* ctx, const i16* src) {
    const i16* p = src;
    m_flags = static_cast<u16>(*p++);
    m_08 = *p++;
    m_owner = *p++;
    m_palette = *p++;
    m_seedFrame = *p++;
    m_frameCount = *p++;
    m_1c = *p++;
    m_20 = *p++;
    m_24 = *p++;
    m_28 = static_cast<u16>(*p++);
    m_indices = 0;
    m_count = 0;
    g_aniParsedNameLen = 0;
    if (m_flags & 0x2) {

        Pix16CPtr np;
        np.m_swords = p;
        const char* name = np.m_chars;
        g_aniParsedNameLen = static_cast<i32>(strlen(name)) + 1;
        ResolveIndices(static_cast<CDDrawSubMgrLeafScan*>(ctx), name);
    }
    return 1;
}

// @early-stop
RVA(0x00168d00, 0x14c)
void CAniRecordView::ResolveIndices(CDDrawSubMgrLeafScan* owner, const char* str) {
    if (owner == 0 || str == 0) {
        return;
    }
    CStringArray tokens;
    char tok[0x80];
    i32 n = 0;
    const char* s = str;
    while (*s != 0) {
        char c = *s;
        if (c > 0x21) {
            tok[n++] = c;
        } else {
            tok[n] = 0;
            if (n > 0) {
                tokens.SetAtGrow(tokens.GetSize(), tok);
            }
            n = 0;
        }
        s++;
    }
    tok[n] = 0;
    if (n > 0) {
        tokens.SetAtGrow(tokens.GetSize(), tok);
    }
    m_count = tokens.GetSize();
    if (m_count > 0) {
        m_indices = static_cast<i32*>(operator new(static_cast<u32>((m_count * 4))));
        for (i32 i = 0; i < m_count; i++) {

            CString t = tokens.GetAt(i);
            void* v = 0;
            owner->m_10.Lookup(t, v);

            AddrWord idx;
            idx.m_addr = v;
            m_indices[i] = idx.m_word;
        }
    }
}

RVA(0x00168e50, 0x1e)
i32 CAniRecordView::GetSize() {
    i32 n = m_frameCount;
    i32 size = 0x16;
    if (n > 0) {
        if (m_flags & 0x1) {
            size = n * 22;
        } else {
            size = n;
        }
    }
    return size;
}

RVA_COMPGEN(0x00168e70, 0x27, ?GetAt@CStringArray@@QBE?AVCString@@H@Z)

RVA(0x00168ea0, 0x40)
i32 CAniRecordBase2::AllocBufMakeB2(char* path, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->MakeB2(path, 0x44);
    m_buf = buf;
    if (buf == 0) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

RVA(0x00168ee0, 0x40)
i32 CAniRecordBase2::AllocBufMakeB(void* data, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->MakeB(data, 0x44);
    m_buf = buf;
    if (buf == 0) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

RVA(0x00168f20, 0x40)
i32 CAniRecordBase2::AllocBufCreate(i32 handle, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->Create(handle, 0x44);
    m_buf = buf;
    if (buf == 0) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

RVA(0x00168f60, 0x45)
i32 CAniRecordBase2::AllocBufMakeB3(void* data, i32 size, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->MakeB3(data, size, 0x44);
    m_buf = buf;
    if (buf == 0) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

RVA(0x00168fb0, 0x1f)
void CAniRecordBase2::Unload() {
    CDDPalette* buf = m_buf;
    if (buf != 0) {
        OwnerMgr()->m_ptrColl->RemoveItemB(buf);
        m_buf = 0;
    }
}

RVA(0x00168fd0, 0x24)
i32 CAniRecordBase2::PushPalette() {
    CDDrawSurfaceChildA* sd = OwnerMgr()->m_drawTarget->m_frontPair;
    if (sd->m_bpp != 8) {
        return 1;
    }
    return sd->m_surface->SetPalette(m_buf, 0);
}

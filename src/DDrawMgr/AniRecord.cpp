#include <rva.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>  // the record owner (m_ptrColl/m_drawTarget)
#include <DDrawMgr/DDrawSubMgrPages.h>  // m_drawTarget full type (m_frontPair)
#include <DDrawMgr/DDrawSurfacePair.h>  // the front pair (m_bpp/m_surface)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // the token-map ctx (m_10)
#include <Wap32/Object.h>
#include <Mfc.h>                    // real MFC CStringArray / CMapStringToPtr / CString / CObject
#include <Gruntz/AniRecordView.h>   // the primary-facet class (CAniRecordView : CObject)
#include <DDrawMgr/DDSurface.h>     // CDDSurface::SetPalette (PushPalette, reloc-masked)
#include <DDrawMgr/DirectDrawMgr.h> // canonical CDDPalette (the +0x10 work buffer's real class)
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections - the real +0x1c pool allocator
#include <DDrawMgr/AniRecordBase2.h>      // the canonical secondary/base facet (dtor 0x165dd0 here)
#include <DDrawMgr/AniRecordViews.h> // honest by-offset owner/surface models (@identity-TODO, no RTTI)
#include <string.h>                  // strlen (inline repnz scas)
#include <DDrawMgr/AniRecord.h> // own exported globals (ex Globals.h)

VTBL(CAniRecordView, 0x001f02c0);
VTBL(CAniRecordBase2, 0x001f02d8); // ??_7 (14 slots)
DATA(0x002bf3c4)
i32 g_aniParsedNameLen = 0; // 0x6bf3c4

void* operator new(u32 n);
void operator delete(void* p);

// The record's owner nodes CAniRecordOwner (record+0x0c) and CAniMapOwner (the token-map
// owner) are honest by-offset models with no recoverable RTTI identity; their defs live
// in <DDrawMgr/AniRecordViews.h> (@identity-TODO) so they no longer count as .cpp-local
// views. CAniRecordOwner->m_pool is the real CDDrawPtrCollections the Alloc* leaves use.

RVA(0x001657a0, 0x66)
CAniRecordView::~CAniRecordView() {
    CAniRecordView* r = this;
    if (r->m_indices != 0) {
        ::operator delete(r->m_indices);
    }
    r->m_owner = reinterpret_cast<CDDrawSurfaceMgr*>(0xffff); // sentinel
    r->m_count = 0;
    r->m_indices = 0;
    // implicit grand-base re-stamp (masks 0x5e8cb4) folds in here as the last store.
}

RVA(0x00165d90, 0x9)
i32 CAniRecordBase2::IsLoaded() {
    return m_buf != 0;
}

RVA(0x00165da0, 0x6)
i32 CAniRecordBase2::GetClassId() {
    return 0x15;
}

RVA(0x00165dd0, 0x5b)
CAniRecordBase2::~CAniRecordBase2() {
    // Own-slot FreeBuf: in the dtor cl devirtualizes to the direct 0x168fb0 call.
    FreeBuf();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    // implicit grand-base re-stamp (masks 0x5e8cb4) folds in here as the last store.
}

RVA(0x00168c60, 0xa0)
i32 CAniRecordView::Parse(void* ctx, const i16* src) {
    const i16* p = src;
    m_flags = static_cast<u16>(*p++);
    m_08 = *p++;
    m_owner = reinterpret_cast<CDDrawSurfaceMgr*>(*p++); // serialized handle
    m_buf = *p++;
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
        const char* name = reinterpret_cast<const char*>(p);
        g_aniParsedNameLen = static_cast<i32>(strlen(name)) + 1;
        ResolveIndices(static_cast<CDDrawSubMgrLeafScan*>(ctx), name);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x168d00: whitespace-split `str` into tokens, look each token up by name in the
// owner's CMapStringToPtr (owner+0x10), and store the resolved indices in a
// RezAlloc'd array (m_indices, length m_count). /GX frame for the local
// CStringArray + per-token CString temporaries.
// @early-stop
// /GX CStringArray + tokenizer: complete reconstruction at ~97%. The whole body
// (tokenize / SetAtGrow / RezAlloc / per-token GetAt+Lookup+store / EH unwind) is
// byte-correct; the residual is the named-local slot coin-flip in the per-token
// loop (retail puts the Lookup-out at [esp+0x10] and the CString temp at [esp+0x14];
// cl flips them - not decl-order-steerable) + reloc operand names. Documented
// regalloc/slot wall - parked for the final sweep.
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
            // The direct MFC call: MSVC5's budget does NOT inline the by-value-CString
            // accessor - it emits its own out-of-line ?GetAt@CStringArray COMDAT and
            // calls it, exactly retail's 0x168e70 shape. (The ex-CAniStrArray shim's
            // "cannot re-emit the real method" wall was WRONG.)
            CString t = tokens.GetAt(i);
            void* v = 0;
            owner->m_10.Lookup(t, v);
            m_indices[i] = reinterpret_cast<i32>(v);
        }
    }
}

RVA(0x00168e50, 0x1e)
i32 CAniRecordView::GetSize() {
    i32 n = m_frameCount;
    if (n <= 0) {
        return 0x16;
    }
    if (m_flags & 0x1) {
        return n * 22;
    }
    return n;
}

// ---------------------------------------------------------------------------
// 0x168e70 IS the real ?GetAt@CStringArray COMDAT, emitted by THIS TU's direct
// tokens.GetAt(i) call (MSVC5 declines to inline the by-value-CString accessor).
// The ex-CAniStrArray shim + its hand copy are GONE; the COMDAT is name-pinned.
RVA_COMPGEN(0x00168e70, 0x27, ?GetAt@CStringArray@@QBE?AVCString@@H@Z)

RVA(0x00168ea0, 0x40)
void* CAniRecordBase2::AllocBufMakeB2(i32 size, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->MakeB2(size, 0x44);
    m_buf = reinterpret_cast<i32>(buf);
    if (buf == 0) {
        return static_cast<void*>(0); // tail returns 1 only on the success path below
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return reinterpret_cast<void*>(1);
}

RVA(0x00168ee0, 0x40)
void* CAniRecordBase2::AllocBufMakeB(i32 size, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->MakeB(reinterpret_cast<void*>(size), 0x44);
    m_buf = reinterpret_cast<i32>(buf);
    if (buf == 0) {
        return static_cast<void*>(0);
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return reinterpret_cast<void*>(1);
}

RVA(0x00168f20, 0x40)
void* CAniRecordBase2::AllocBufCreate(i32 handle, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->Create(handle, 0x44);
    m_buf = reinterpret_cast<i32>(buf);
    if (buf == 0) {
        return static_cast<void*>(0);
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return reinterpret_cast<void*>(1);
}

RVA(0x00168f60, 0x45)
void* CAniRecordBase2::AllocBufMakeB3(i32 a, i32 size, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_ptrColl->MakeB3(a, size, 0x44);
    m_buf = reinterpret_cast<i32>(buf);
    if (buf == 0) {
        return static_cast<void*>(0);
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return reinterpret_cast<void*>(1);
}

RVA(0x00168fb0, 0x1f)
void CAniRecordBase2::FreeBuf() {
    i32 buf = m_buf;
    if (buf != 0) {
        OwnerMgr()->m_ptrColl->RemoveItemB(reinterpret_cast<CDDPalette*>(buf));
        m_buf = 0;
    }
}

// The owner-image / surface-descriptor chain PushPalette walks (AniImageHost ->
// AniSurfDesc -> target CDDSurface) is modeled by-offset in <DDrawMgr/AniRecordViews.h>
// (@identity-TODO: the concrete image/descriptor classes are not yet RTTI-recovered).

RVA(0x00168fd0, 0x24)
i32 CAniRecordBase2::PushPalette() {
    CDDrawSurfacePair* sd = OwnerMgr()->m_drawTarget->m_frontPair; // the ex AniImageHost/AniSurfDesc chain, all canon
    if (sd->m_bpp != 8) {
        return 1;
    }
    return sd->m_surface->SetPalette(reinterpret_cast<CDDPalette*>(m_buf), 0);
}

// 0x16b230 was a mis-homed, mis-identified "gap orphan" here. The old @identity-TODO
// guessed a CLevelPlane plane-geometry Init, but the retail disasm disproves that: it is
// the MSVC C++ iostreams library method streambuf::xsgetn(char*, int) - a bulk get that
// reads byte-at-a-time via the virtual underflow (slot 8, off 0x20) when m_08 != 0, else
// rep-movs block-copies from the get-area [m_28,m_2c). It is referenced ONLY as slot 6
// (off 0x18) of the streambuf / filebuf / strstreambuf vtables (0x1f042c / 0x1f03fc /
// 0x1f0344 - inherited unchanged), i.e. it belongs to the CRT iostream cluster, not to
// AniRecord. Evicted from this TU and carved into config/library_labels.csv
// (iostream-cluster-bail), alongside its siblings streambuf::xsputn/setbuf/pbackfail.

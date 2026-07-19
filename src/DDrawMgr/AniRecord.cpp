#include <rva.h>
#include <Wap32/Object.h>
// AniRecord.cpp - the 0x34-byte 'ANI' animation FRAME RECORD cataloged by
// CAniElement (src/Gruntz/AniElement.cpp) into a CObArray of these. The
// orchestrator placeholders tomalla-39 / _40 / _41 are ALL this ONE class
// (consolidated here):
//
//   tomalla-39: 0x168c60 Parse (read a packed 9-short record header off a
//                    binary cursor, then resolve the name string), 0x168e50
//                    GetSize (frame-count -> byte accumulator).
//   tomalla-40: 0x168d00 ResolveIndices (whitespace-split the name string
//                    into tokens, look each up in the owner's CMapStringToPtr, and
//                    store the resolved indices in a RezAlloc'd array). /GX frame.
//   tomalla-41: 0x165dd0 the base-2 (CAniRecordBase2) destructor (frees the
//                    +0x10 buffer-virtual, resets the base subobject to CObject),
//                    0x168ee0 / 0x168fb0 the two buffer (de)allocation virtuals
//                    that go through the owner's pool allocator (owner+0x1c).
//
// The class is multiply-derived: a primary base whose vftable is g_aniRecordVtbl
// @0x5f02c0 (5 slots, slot1 dtor 0x165780/0x1657a0) and a secondary CObject base
// whose vftable is CAniRecordBase2 @0x5f02d8 (14 slots, = 0x5f02c0+0x18; slot1 dtor
// 0x165db0/0x165dd0). Both share the CObject-like grand-base vftable @0x5e8cb4. vtable-4
// is NULL for both (verified) -> no RTTI COL, so the two base dtors are modeled as REAL
// polymorphic types deriving from a shared empty-dtor grand-base (CAniRecordObjBase);
// cl emits the implicit ??_7 stamps (reloc-masked, /GR stays off). The leaf record
// (CAniRecord) stays a non-polymorphic data-layout class (explicit m_vptr @+0x00) so its
// matched leaves keep their offsets. eh-dtor-implicit-vptr-stamp-first.md (sub-case 2).
//
// This TU carries a /GX EH frame (flags="eh"): the ResolveIndices parser and the
// base-2 destructor have SEH frames for their destructible locals; the leaves
// (Parse / GetSize / the two buffer virtuals) stay frameless and byte-exact.
// ---------------------------------------------------------------------------
#include <Mfc.h>                    // real MFC CStringArray / CMapStringToPtr / CString / CObject
#include <Gruntz/AniRecordView.h>   // the primary-facet class (CAniRecordView : CObject)
#include <DDrawMgr/DDSurface.h>     // CDDSurface::SetPalette (PushPalette, reloc-masked)
#include <DDrawMgr/DirectDrawMgr.h> // canonical CDDPalette (the +0x10 work buffer's real class)
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections - the real +0x1c pool allocator
#include <DDrawMgr/AniRecordBase2.h>      // the canonical secondary/base facet (dtor 0x165dd0 here)
#include <DDrawMgr/AniRecordViews.h> // honest by-offset owner/surface models (@identity-TODO, no RTTI)
#include <string.h>                  // strlen (inline repnz scas)
#include <Globals.h>

// The three vftables (g_aniRecordVtbl @0x5f02c0, CAniRecordBase2 @0x5f02d8, the shared
// grand-base @0x5e8cb4) are no longer manual DATA() externs: the base classes below are
// real polymorphic types, so cl emits the implicit ??_7 + grand-base re-stamps (reloc-
// masked against the target's differently-named symbols). All three are still DATA()-bound
// in other TUs (CAniElement / CDDrawWorkerMapSmall / the CObject family) so the target stays named.

// g_aniParsedNameLen (0x6bf3c4): the parsed name length the catalog builder uses
// to advance the record stream cursor; Parse sets it (strlen of the name). Owned by
// this TU; DEFINED here (.bss zero-init), reference extern stays in <Globals.h>.
// (REHOME DD-Drain-1)
DATA(0x002bf3c4)
i32 g_aniParsedNameLen = 0; // 0x6bf3c4

// Global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);
// operator delete (0x1b9b82, ??3@YAXPAX@Z): the engine's Rez heap free IS the global
// operator delete (FID-verified library label). C++ linkage (potentially-throwing) so
// MSVC5 keeps the /GX base-subobject unwind frame; called both by the auto-generated
// ??_G scalar-deleting dtors (slot 1) and the explicit member-teardown frees below.
void operator delete(void* p);

// ---------------------------------------------------------------------------
// The pool allocator the buffer virtuals route through: the owner (record+0x0c)
// holds at +0x1c the real CDDrawPtrCollections pool allocator (canonical
// <DDrawMgr/DDrawPtrCollections.h>): its MakeB/MakeB2/MakeB3/Create build a
// CDDPalette work buffer, RemoveItemB frees it. (Was a fake local CAniRecordPool
// view whose Alloc*/Free names masked the real 0x142f40..0x1430c0 / 0x142f10
// CDDrawPtrCollections methods.)

// The record's owner nodes CAniRecordOwner (record+0x0c) and CAniMapOwner (the token-map
// owner) are honest by-offset models with no recoverable RTTI identity; their defs live
// in <DDrawMgr/AniRecordViews.h> (@identity-TODO) so they no longer count as .cpp-local
// views. CAniRecordOwner->m_pool is the real CDDrawPtrCollections the Alloc* leaves use.

// The freshly-allocated +0x10 palette buffer is a real CDDPalette (canonical
// <DDrawMgr/DirectDrawMgr.h>); second-stage init captures the current system
// palette (CaptureSystemPalette @0x1485b0, DirPal.cpp). The ex-"DirPal" local
// view folded onto the canonical class (wave3-J).

// ---------------------------------------------------------------------------
// The 0x34-byte record. Layout recovered from Parse + the dtors + CAniElement's
// new-site. Modeled with NO virtuals (manual-vtable, dual-base), so the offsets
// stay exactly where retail puts them.

// ---------------------------------------------------------------------------
// The shared WAP grand-base (vftable @0x5e8cb4, 5 slots: 0x1bef01 / scalar-dtor /
// 0x0028ec / 0x00106e / 0x004034). Modeled as a REAL polymorphic base with an EMPTY
// virtual dtor, so cl emits ONLY the implicit grand-base vptr re-stamp (the LAST store
// at the derived dtor's tail). The header is the implicit vptr @+0x00 then +0x04/+0x08/
// +0x0c the base-2 dtor resets.
// HELD (vtable_hierarchy correction): CAniRecordBase2 carries slot 6 = 0x001c08, the
// CWapObj-family marker, so it derives CObject -> CWapObj -> CAniRecordBase2 (NOT CObject
// directly). Left on this fabricated grand-base pending CWapObj modeling; do NOT flatten
// to `: public CObject` (that would steal CWapObj's slots 5/6). Only CAniRecordPrimary
// (pure 5-slot, no CWapObj) was re-based to the real CObject.

// ---------------------------------------------------------------------------
// 0x165dd0: the SECONDARY base (CAniRecordBase2 @0x5f02d8, 14 slots) destructor.
// /GX. Real virtual: cl stamps ??_7 (masks 0x5f02d8) at ENTRY (stamp-first), frees the
// +0x10 work buffer (FreeBuf), resets the CObject header (m_04=-1, m_08=0, m_0c=0), then
// the implicit grand-base re-stamp (masks 0x5e8cb4) folds LAST. The 9 extra slots (5..13)
// are declared-only (reloc-masked); the buffer (de)allocation virtuals live as the regular
// CAniRecordView methods below (slots 7/10/11/12 = FreeBuf/AllocBufMakeB/AllocBufMakeB2/
// AllocBufMakeB3). The class def is the SHARED canonical <DDrawMgr/AniRecordBase2.h>
// (also the CDDrawWorkerMapSmall keyed "map worker" - one class, one vtable).

// ---------------------------------------------------------------------------
// 0x1657a0: the PRIMARY base (g_aniRecordVtbl @0x5f02c0, 5 slots == the grand-base
// layout, no extra slots) destructor. /GX. Real virtual: cl stamps ??_7 (masks 0x5f02c0)
// at ENTRY (stamp-first), frees the +0x30 resolved-index array (RezFree), clears the owner
// sentinel (0xffff) / count / array, then the implicit grand-base re-stamp folds LAST.
RVA(0x001657a0, 0x66)
CAniRecordView::~CAniRecordView() {
    CAniRecordView* r = this;
    if (r->m_indices != 0) {
        ::operator delete(r->m_indices);
    }
    r->m_owner = reinterpret_cast<CAniRecordOwner*>(0xffff);
    r->m_count = 0;
    r->m_indices = 0;
    // implicit grand-base re-stamp (masks 0x5e8cb4) folds in here as the last store.
}

RVA(0x00165dd0, 0x5b)
CAniRecordBase2::~CAniRecordBase2() {
    // The +0x10 work-buffer teardown is the CAniRecordView-bound body 0x168fb0
    // (the documented primary/secondary facet split - see AniRecordView.h); the
    // direct call emits the resolvable ?FreeBuf@CAniRecordView@@ symbol.
    reinterpret_cast<CAniRecordView*>(this)->FreeBuf();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    // implicit grand-base re-stamp (masks 0x5e8cb4) folds in here as the last store.
}

// ---------------------------------------------------------------------------
// 0x168c60: Parse a packed record header off the binary cursor `src` (a stream of
// little-endian 16-bit fields): m_flags (WORD), then eight movsx'd i16 -> i32, then
// m_28 (WORD). Zero the index array, clear g_aniParsedNameLen, then if m_flags has
// the "has name" bit (0x2) strlen the trailing name -> g_aniParsedNameLen and
// resolve the indices from it. Returns 1. Frameless leaf.
RVA(0x00168c60, 0xa0)
i32 CAniRecordView::Parse(void* ctx, const i16* src) {
    const i16* p = src;
    m_flags = static_cast<u16>(*p++);
    m_08 = *p++;
    m_owner = reinterpret_cast<CAniRecordOwner*>(*p++);
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
        ResolveIndices(static_cast<CAniMapOwner*>(ctx), name);
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
void CAniRecordView::ResolveIndices(CAniMapOwner* owner, const char* str) {
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
            // GetAt reaches the OUT-OF-LINE CStringArray::GetAt COMDAT (0x168e70), which
            // MFC models _AFXCOLL_INLINE - so it is bound as the layout-identical
            // CAniStrArray shim (vptr@0 / m_data@0x04 == m_pData). See AniRecordViews.h.
            CString t = (reinterpret_cast<CAniStrArray*>(&tokens))->GetAt(i);
            void* v = 0;
            owner->m_map.Lookup(t, v);
            m_indices[i] = reinterpret_cast<i32>(v);
        }
    }
}

// CAniRecordView::GetSize (0x168e50) - packed byte size of the frame table.
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
// CAniStrArray::GetAt (0x168e70) - the out-of-line CStringArray::GetAt COMDAT
// (a by-value CString-array element accessor; ResolveIndices @0x168d00 calls it,
// xref-proven). Decl in <DDrawMgr/AniRecordViews.h> (MFC models GetAt inline, so
// this out-of-line copy cannot be re-emitted as the real CStringArray method).
// Returns (via the RVO return slot) a copy of the CString at m_data[index].
SIZE_UNKNOWN(CAniStrArray);
RVA(0x00168e70, 0x27)
CString CAniStrArray::GetAt(int index) {
    return m_data[index];
}

// ---------------------------------------------------------------------------
// 0x168ea0: (re)allocate the +0x10 work buffer from the owner's pool with size
// 0x44 (CDDrawPtrCollections::MakeB2). On success, if the caller's flag bit 0x1 is set, mark the
// owner's +0x08 flags and run the buffer's second-stage init. Returns 1.
// Frameless leaf.
RVA(0x00168ea0, 0x40)
void* CAniRecordView::AllocBufMakeB2(i32 size, i32 flag) {
    CDDPalette* buf = m_owner->m_pool->MakeB2(size, 0x44);
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

// ---------------------------------------------------------------------------
// 0x168ee0: as 0x168ea0 but through CDDrawPtrCollections::MakeB (the canonical 0x44 allocator).
// Frameless leaf.
RVA(0x00168ee0, 0x40)
void* CAniRecordView::AllocBufMakeB(i32 size, i32 flag) {
    CDDPalette* buf = m_owner->m_pool->MakeB(reinterpret_cast<void*>(size), 0x44);
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

// ---------------------------------------------------------------------------
// 0x168f20 (slot 9): as 0x168ea0 but through CDDrawPtrCollections::Create. Frameless leaf.
RVA(0x00168f20, 0x40)
void* CAniRecordView::AllocBufCreate(i32 handle, i32 flag) {
    CDDPalette* buf = m_owner->m_pool->Create(handle, 0x44);
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

// ---------------------------------------------------------------------------
// 0x168f60: the three-arg buffer allocator (CDDrawPtrCollections::MakeB3, ret 0xc). Frameless leaf.
RVA(0x00168f60, 0x45)
void* CAniRecordView::AllocBufMakeB3(i32 a, i32 size, i32 flag) {
    CDDPalette* buf = m_owner->m_pool->MakeB3(a, size, 0x44);
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

// ---------------------------------------------------------------------------
// 0x168fb0: free the +0x10 work buffer back to the owner's pool (CDDrawPtrCollections::RemoveItemB) and
// clear it. Frameless leaf.
RVA(0x00168fb0, 0x1f)
void CAniRecordView::FreeBuf() {
    i32 buf = m_buf;
    if (buf != 0) {
        m_owner->m_pool->RemoveItemB(reinterpret_cast<CDDPalette*>(buf));
        m_buf = 0;
    }
}

// The owner-image / surface-descriptor chain PushPalette walks (AniImageHost ->
// AniSurfDesc -> target CDDSurface) is modeled by-offset in <DDrawMgr/AniRecordViews.h>
// (@identity-TODO: the concrete image/descriptor classes are not yet RTTI-recovered).

// ---------------------------------------------------------------------------
// CAniRecordView::PushPalette (0x168fd0, vtable slot 13 of CAniRecordBase2): if
// the owner image is 8bpp (surfDesc->m_18 == 8), push this record's palette buffer
// (m_buf) onto the owner's target surface via CDDSurface::SetPalette and return its
// result; otherwise return 1. __thiscall, no args (ret).
RVA(0x00168fd0, 0x24)
i32 CAniRecordView::PushPalette() {
    AniSurfDesc* sd = (reinterpret_cast<AniImageHost*>(m_owner->m_04))->m_10;
    if (sd->m_18 != 8) {
        return 1;
    }
    return sd->m_2c->SetPalette(reinterpret_cast<CDDPalette*>(m_buf), 0);
}

// TU-level SIZE rows for the by-offset owner/surface models (defs in
// <DDrawMgr/AniRecordViews.h>); sizes are unknown (only touched offsets modeled).
SIZE_UNKNOWN(CAniMapOwner);
SIZE_UNKNOWN(CAniRecordOwner);
SIZE_UNKNOWN(AniSurfDesc);
SIZE_UNKNOWN(AniImageHost);

// (Both facets' VTBL rows live with their canonical class defs in the headers:
// CAniRecordView in <Gruntz/AniRecordView.h>, CAniRecordBase2 in
// <DDrawMgr/AniRecordBase2.h>.)

// 0x16b230 was a mis-homed, mis-identified "gap orphan" here. The old @identity-TODO
// guessed a CLevelPlane plane-geometry Init, but the retail disasm disproves that: it is
// the MSVC C++ iostreams library method streambuf::xsgetn(char*, int) - a bulk get that
// reads byte-at-a-time via the virtual underflow (slot 8, off 0x20) when m_08 != 0, else
// rep-movs block-copies from the get-area [m_28,m_2c). It is referenced ONLY as slot 6
// (off 0x18) of the streambuf / filebuf / strstreambuf vtables (0x1f042c / 0x1f03fc /
// 0x1f0344 - inherited unchanged), i.e. it belongs to the CRT iostream cluster, not to
// AniRecord. Evicted from this TU and carved into config/library_labels.csv
// (iostream-cluster-bail), alongside its siblings streambuf::xsputn/setbuf/pbackfail.

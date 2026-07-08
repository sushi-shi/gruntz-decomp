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
#include <Mfc.h>                  // real MFC CStringArray / CMapStringToPtr / CString / CObject
#include <Gruntz/AniRecordView.h> // the primary-facet class (CAniRecordView : CObject)
#include <string.h>               // strlen (inline repnz scas)
#include <Globals.h>

// The three vftables (g_aniRecordVtbl @0x5f02c0, CAniRecordBase2 @0x5f02d8, the shared
// grand-base @0x5e8cb4) are no longer manual DATA() externs: the base classes below are
// real polymorphic types, so cl emits the implicit ??_7 + grand-base re-stamps (reloc-
// masked against the target's differently-named symbols). All three are still DATA()-bound
// in other TUs (CAniElement / CDDrawWorkerMapSmall / the CObject family) so the target stays named.

// g_aniParsedNameLen (0x6bf3c4): the parsed name length the catalog builder uses
// to advance the record stream cursor; Parse sets it (strlen of the name).

// Global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);
// operator delete: called by the auto-generated ??_G scalar-deleting dtors (slot 1).
void operator delete(void* p);
// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame
// in the primary dtor (same convention as RezBufferObjectDtor.cpp / BoundaryUpper*Eh.cpp).
void RezFree(void* p);

// ---------------------------------------------------------------------------
// The pool allocator the buffer virtuals route through: the owner (record+0x0c)
// holds at +0x1c a manager whose Alloc(handle, size) / Free(p) (re)allocate the
// record's +0x10 work buffer from a fixed pool. Modeled as a tiny __thiscall
// helper so `mov ecx,[owner+0x1c]; call` falls out with no stack cleanup.
class CAniRecordPool {
public:
    void* Alloc1_142fc0(i32 handle, i32 size);        // 0x142fc0
    void* Alloc2_142f40(i32 handle, i32 size);        // 0x142f40
    void* Alloc3_1430c0(i32 a, i32 handle, i32 size); // 0x1430c0
    void Free_142f10(void* p);                        // 0x142f10
};

// The owner node (record+0x0c). Its +0x08 is a flags word the buffer virtuals OR
// a bit into; its +0x1c is the pool above.
// DISPOSITION: CAniRecordOwner / CAniMapOwner are the record's owner nodes reached by
// this-offset from the record's parse/alloc paths; neither carries an RTTI vtable, so
// there is no recoverable class name. Kept as honest minimal by-offset models (no
// fabricated identity); resolvable only if their owning subsystem is later RTTI-pinned.
class CAniRecordOwner {
public:
    i32 m_00, m_04; // +0x00..+0x07
    i32 m_flags;    // +0x08  flags
    char _pad0c[0x1c - 0x0c];
    CAniRecordPool* m_pool; // +0x1c  the pool allocator
};

// The freshly-allocated +0x10 palette buffer; second-stage init captures the
// current system palette. The real class is DirPal (0x1485b0, DirPal.cpp).
struct DirPal {
    i32 CaptureSystemPalette(); // 0x1485b0
};

// The CMapStringToPtr the index resolver looks each token up in lives at owner2
// +0x10 (owner2 = ResolveIndices arg1). Real MFC layout via <Mfc.h>.
struct CAniMapOwner {
    char _pad00[0x10];
    CMapStringToPtr m_map; // +0x10
};

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
struct CAniRecordObjBase {
    virtual void GetRuntimeClass(); // [0] 0x1bef01 (shared GetRuntimeClass thunk)
    virtual ~CAniRecordObjBase();   // [1] scalar-deleting dtor
    virtual void Serialize();       // [2] 0x0028ec
    virtual void AssertValid();     // [3] 0x00106e
    virtual void Dump();            // [4] 0x004034

    i32 m_04, m_08, m_0c; // +0x04..+0x0f (CObject header)
};
// Empty body => folds as JUST the grand-base re-stamp at the derived dtor's tail.
inline CAniRecordObjBase::~CAniRecordObjBase() {}

// ---------------------------------------------------------------------------
// 0x165dd0: the SECONDARY base (CAniRecordBase2 @0x5f02d8, 14 slots) destructor.
// /GX. Real virtual: cl stamps ??_7 (masks 0x5f02d8) at ENTRY (stamp-first), frees the
// +0x10 work buffer (FreeBuf), resets the CObject header (m_04=-1, m_08=0, m_0c=0), then
// the implicit grand-base re-stamp (masks 0x5e8cb4) folds LAST. The 9 extra slots (5..13)
// are declared-only (reloc-masked); the buffer (de)allocation virtuals live as the regular
// CAniRecord methods below (slots 7/10/11/12 = FreeBuf/Alloc168ee0/Alloc168ea0/Alloc168f60).
struct CAniRecordBase2 : public CObject { // was : CAniRecordObjBase (merged intermediate)
    i32 m_04, m_08, m_0c; // +0x04..0x0f CObject-header fields (from merged CAniRecordObjBase)
    virtual ~CAniRecordBase2() OVERRIDE; // [1] overrides; UAE
    virtual void Slot05_165d90();        // [5] 0x165d90
    virtual void IsValidImage();         // [6] 0x001c08
    virtual void Slot07_168fb0(); // [7] 0x168fb0 (FreeBuf, bound as CAniRecord method - other slot)
    virtual void Slot08_165da0(); // [8] 0x165da0
    virtual void Slot09_168f20(); // [9] 0x168f20
    virtual void Alloc168ee0();   // [10] 0x168ee0 (= CAniRecordView::Alloc168ee0)
    virtual void Alloc168ea0();   // [11] 0x168ea0 (= CAniRecordView::Alloc168ea0)
    virtual void Alloc168f60();   // [12] 0x168f60 (= CAniRecordView::Alloc168f60)
    virtual void Slot13_168fd0(); // [13] 0x168fd0

    void FreeBuf_168fb0() {
        ((CAniRecordView*)this)->FreeBuf_168fb0();
    }
};

RVA(0x00165dd0, 0x5b)
CAniRecordBase2::~CAniRecordBase2() {
    FreeBuf_168fb0();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    // implicit grand-base re-stamp (masks 0x5e8cb4) folds in here as the last store.
}

// ---------------------------------------------------------------------------
// 0x1657a0: the PRIMARY base (g_aniRecordVtbl @0x5f02c0, 5 slots == the grand-base
// layout, no extra slots) destructor. /GX. Real virtual: cl stamps ??_7 (masks 0x5f02c0)
// at ENTRY (stamp-first), frees the +0x30 resolved-index array (RezFree), clears the owner
// sentinel (0xffff) / count / array, then the implicit grand-base re-stamp folds LAST.
RVA(0x001657a0, 0x66)
CAniRecordView::~CAniRecordView() {
    CAniRecordView* r = this;
    if (r->m_indices != 0) {
        RezFree(r->m_indices);
    }
    r->m_owner = (CAniRecordOwner*)0xffff;
    r->m_count = 0;
    r->m_indices = 0;
    // implicit grand-base re-stamp (masks 0x5e8cb4) folds in here as the last store.
}

// ---------------------------------------------------------------------------
// 0x168c60: Parse a packed record header off the binary cursor `src` (a stream of
// little-endian 16-bit fields): m_flags (WORD), then eight movsx'd i16 -> i32, then
// m_28 (WORD). Zero the index array, clear g_aniParsedNameLen, then if m_flags has
// the "has name" bit (0x2) strlen the trailing name -> g_aniParsedNameLen and
// resolve the indices from it. Returns 1. Frameless leaf.
RVA(0x00168c60, 0xa0)
i32 CAniRecordView::Parse_168c60(void* ctx, const i16* src) {
    const i16* p = src;
    m_flags = (u16)*p++;
    m_08 = *p++;
    m_owner = (CAniRecordOwner*)*p++;
    m_buf = *p++;
    m_14 = *p++;
    m_frameCount = *p++;
    m_1c = *p++;
    m_20 = *p++;
    m_24 = *p++;
    m_28 = (u16)*p++;
    m_indices = 0;
    m_count = 0;
    g_aniParsedNameLen = 0;
    if (m_flags & 0x2) {
        const char* name = (const char*)p;
        g_aniParsedNameLen = (i32)strlen(name) + 1;
        ResolveIndices_168d00((CAniMapOwner*)ctx, name);
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
void CAniRecordView::ResolveIndices_168d00(CAniMapOwner* owner, const char* str) {
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
        m_indices = (i32*)operator new((u32)(m_count * 4));
        for (i32 i = 0; i < m_count; i++) {
            CString t = tokens.GetAt(i);
            void* v = 0;
            owner->m_map.Lookup(t, v);
            m_indices[i] = (i32)v;
        }
    }
}

// ---------------------------------------------------------------------------
// 0x168e50: byte-size of the resolved frames. If the frame count m_18 is positive,
// return m_18*22 when the "scaled" flag (bit 0x1) is set, else m_18; otherwise the
// 0x16 empty-record constant. Frameless leaf.
// @early-stop
// scheduling: body shape (count guard / scaled lea*22 / plain count / 0x16 default)
// byte-correct; retail pins the count in edx and materializes the `mov eax,0x16`
// default right after the count load, while cl keeps the count in eax and schedules
// the constant on the fall-through ret (~87%). All 4 spellings tried (early-return,
// 0x16-default-temp, n<=0-first, n>0-first) flip the regalloc but none reproduce the
// edx-pin + eager-default. Not source-steerable - documented scheduling wall.
RVA(0x00168e50, 0x1e)
i32 CAniRecordView::GetSize_168e50() {
    i32 n = m_frameCount;
    if (n > 0) {
        if (m_flags & 0x1) {
            return n * 22;
        }
        return n;
    }
    return 0x16;
}

// ---------------------------------------------------------------------------
// 0x168ea0: (re)allocate the +0x10 work buffer from the owner's pool with size
// 0x44 (Alloc2_142f40). On success, if the caller's flag bit 0x1 is set, mark the
// owner's +0x08 flags and run the buffer's second-stage init. Returns 1.
// Frameless leaf.
RVA(0x00168ea0, 0x40)
void* CAniRecordView::Alloc168ea0(i32 size, i32 flag) {
    DirPal* buf = (DirPal*)m_owner->m_pool->Alloc2_142f40(size, 0x44);
    m_buf = (i32)buf;
    if (buf == 0) {
        return (void*)0; // tail returns 1 only on the success path below
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// 0x168ee0: as 0x168ea0 but through Alloc1_142fc0 (the canonical 0x44 allocator).
// Frameless leaf.
RVA(0x00168ee0, 0x40)
void* CAniRecordView::Alloc168ee0(i32 size, i32 flag) {
    DirPal* buf = (DirPal*)m_owner->m_pool->Alloc1_142fc0(size, 0x44);
    m_buf = (i32)buf;
    if (buf == 0) {
        return (void*)0;
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// 0x168f60: the three-arg buffer allocator (Alloc3_1430c0, ret 0xc). Frameless leaf.
RVA(0x00168f60, 0x45)
void* CAniRecordView::Alloc168f60(i32 a, i32 size, i32 flag) {
    DirPal* buf = (DirPal*)m_owner->m_pool->Alloc3_1430c0(a, size, 0x44);
    m_buf = (i32)buf;
    if (buf == 0) {
        return (void*)0;
    }
    if (flag & 0x1) {
        m_08 |= 0x1;
        buf->CaptureSystemPalette();
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// 0x168fb0: free the +0x10 work buffer back to the owner's pool (Free_142f10) and
// clear it. Frameless leaf.
RVA(0x00168fb0, 0x1f)
void CAniRecordView::FreeBuf_168fb0() {
    i32 buf = m_buf;
    if (buf != 0) {
        m_owner->m_pool->Free_142f10((void*)buf);
        m_buf = 0;
    }
}

// ---------------------------------------------------------------------------
// CAniStrArray::GetAt (0x168e70) - a by-value CString-array element accessor,
// re-homed from src/Stub/MallocConstructors. Returns (via the RVO return slot) a
// copy of the CString at this->m_data[index] (`lea &m_data[i]; CString::CString`
// copy-ctor 0x1b9ba3). xref (gruntz.analysis.xref): CAniRecordView::ResolveIndices
// (0x168d00). Modeled as the small string-array view CAniRecord indexes.
struct CAniStrArray {
    char m_00[4];             // +0x00
    CString* m_data;          // +0x04  CString array base (4-byte elements)
    CString GetAt(int index); // 0x168e70
};
SIZE_UNKNOWN(CAniStrArray);
RVA(0x00168e70, 0x27)
CString CAniStrArray::GetAt(int index) {
    return m_data[index];
}

SIZE_UNKNOWN(CAniMapOwner);
SIZE_UNKNOWN(CAniRecordBase2);
SIZE_UNKNOWN(DirPal);
SIZE_UNKNOWN(CAniRecordOwner);
SIZE_UNKNOWN(CAniRecordPool);

SIZE_UNKNOWN(CAniRecordObjBase);
VTBL(CAniRecordBase2, 0x001f02d8); // ??_7 (14 slots)
VTBL(CAniRecordView, 0x001f02c0);  // ??_7CAniRecordPrimary@@6B@ (5-slot CObject-derived)

// AniRecordView.h - the shared polymorphic view of the 0x34-byte 'ANI' frame
// record that CAniElement (Build) `new`s and catalogs in its CObArray.
//
// The REAL record is CAniRecord (src/Gruntz/AniRecord.cpp): a non-polymorphic,
// dual-base MANUAL-VTABLE record (primary CObject facet vtbl @0x5f02c0 + secondary
// 14-slot facet vtbl @0x5f02d8) modeled non-poly so its matched leaf offsets stay
// fixed. Because the real class is non-poly, its callers can't dispatch through it,
// and folding a virtual interface back onto it would poly-ify an MI manual-vtable
// record and shift those leaf offsets. So this view is the REQUIRED caller-side
// facet (verdict: one object, required codegen-model split - NOT a dedup miss; cf.
// [[vtable-realization-ctor-boundary]] CGameRegistry/CGruntzMgr).
//
// This models the record's PRIMARY 5-slot CObject facet (vtable @0x5f02c0). Callers:
//   * `new CAniRecordView`  - the ctor stamps the vptr + seeds m_owner/m_count/
//     m_indices; cl auto-emits ??_7CAniRecordView (reloc-masks 0x5f02c0), no manual
//     `m_vptr = &g_aniRecordVtbl` store (the ALL-VTABLES-real model).
//   * `rec->Parse_168c60(...)` / `rec->GetSize_168e50()` - non-virtual leaf entries.
//   * `delete rec` - the slot-1 scalar-deleting dtor dispatched as
//     `mov edx,[ecx]; call [edx+4]`. Modeled as an explicit named virtual (NOT a
//     C++ `~dtor` + `delete`) precisely to emit that call WITHOUT the compiler's
//     delete-null-check (the caller already guards `if (el != 0)`).
//
// Folds the former CAniElement.cpp-local `CAniRecordInit` (identical 5-slot new-site
// view) into this single shared definition.
#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H
#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // real MFC CObject (the primary-facet base)

class CAniMapOwner;
class CAniRecordOwner;

// The ANI frame record. cl inherits GetRuntimeClass/Serialize/AssertValid/Dump (the
// shared CObject defaults) and OVERRIDES only slot 1 (the real teardown dtor 0x1657a0
// that frees m_indices). Single owning class of the primary ??_7 @0x5f02c0 (VTBL in
// AniRecord.cpp). Holds the record data + its non-virtual leaf entries directly (no
// separate non-poly data twin / `(CAniRecord*)this` cross-cast).
struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE; // [1] 0x1657a0 real primary-facet teardown dtor

    i32 Parse_168c60(void* ctx, const i16* src);                      // 0x168c60
    RVA(0x00168e50, 0x1e)
    i32 GetSize_168e50() {
        i32 n = m_frameCount;
        if (n > 0) {
        if (m_flags & 0x1) {
        return n * 22;
        }
        return n;
        }
        return 0x16;
    }
    void ResolveIndices_168d00(CAniMapOwner* owner, const char* str); // 0x168d00
    void* Alloc168ee0(i32 size, i32 flag);                            // 0x168ee0
    void* Alloc168f20(i32 handle, i32 flag);                          // 0x168f20 (slot 9)
    void* Alloc168ea0(i32 size, i32 flag);                            // 0x168ea0
    void* Alloc168f60(i32 a, i32 size, i32 flag);                     // 0x168f60
    void FreeBuf_168fb0();                                            // 0x168fb0
    // 0x168fd0 (vtable slot 13): when the owner image is 8bpp, push the record's
    // palette buffer (m_buf) onto the owner's surface; else return 1.
    i32 Slot13_168fd0();

    inline CAniRecordView() {
        m_count = 0;
        m_indices = 0;
        m_owner = (CAniRecordOwner*)0xffff;
    }

    // vptr implicit at +0x00
    u16 m_flags;              // +0x04  status word (bit 1 scaled, bit 2 has-name)
    u16 m_06;                 // +0x06
    i32 m_08;                 // +0x08
    CAniRecordOwner* m_owner; // +0x0c  owner node (seeded 0xffff)
    i32 m_buf;                // +0x10  pool work buffer
    i32 m_14;                 // +0x14
    i32 m_frameCount;         // +0x18  frame count (GetSize)
    i32 m_1c;                 // +0x1c
    i32 m_20;                 // +0x20
    i32 m_24;                 // +0x24
    u16 m_28;                 // +0x28
    u16 m_2a;                 // +0x2a
    i32 m_count;              // +0x2c  resolved-index array length
    i32* m_indices;           // +0x30  resolved-index array
};

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_CANIRECORDVIEW_H

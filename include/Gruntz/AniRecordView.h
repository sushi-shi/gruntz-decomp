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
//   * `rec->ScalarDtor(1)` - the slot-1 scalar-deleting dtor dispatched as
//     `mov edx,[ecx]; call [edx+4]`. Modeled as an explicit named virtual (NOT a
//     C++ `~dtor` + `delete`) precisely to emit that call WITHOUT the compiler's
//     delete-null-check (the caller already guards `if (el != 0)`).
//
// Folds the former CAniElement.cpp-local `CAniRecordInit` (identical 5-slot new-site
// view) into this single shared definition.
#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H

#include <Ints.h>

struct CAniRecordView {
    virtual void FUN_005bef01();        // [0] 0x1bef01
    virtual void* ScalarDtor(u32 flag); // [1] 0x165780 scalar-deleting dtor slot
    virtual void FUN_004028ec();        // [2] 0x0028ec
    virtual void FUN_0040106e();        // [3] 0x00106e
    virtual void FUN_00404034();        // [4] 0x004034

    i32 Parse_168c60(void* ctx, const char* cursor); // 0x168c60 __thiscall
    i32 GetSize_168e50();                            // 0x168e50 __thiscall

    inline CAniRecordView() {
        m_count = 0;
        m_indices = 0;
        m_owner = 0xffff;
    }

    // vptr implicit at +0x00
    char m_pad04[0x8];  // +0x04..+0x0b
    i32 m_owner;        // +0x0c = 0xffff
    char m_pad10[0x1c]; // +0x10..+0x2b
    i32 m_count;        // +0x2c = 0
    i32 m_indices;      // +0x30 = 0
};

#endif // GRUNTZ_CANIRECORDVIEW_H

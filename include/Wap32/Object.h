// Object.h - Wap::CObject, the WAP engine's CObject grand-base (RTTI name
// "CObject", vtable RVA 0x1e8cb4 / VA 0x5e8cb4, COL type-descriptor `CObject`).
//
// This is the ONE authoritative WAP grand-base. The reconstruction previously
// spread it under ten local aliases - CWapObject plus nine `*Base` shims - which
// the vtable_hierarchy --name-audit flagged as all mapping to the same RTTI
// CObject @0x1e8cb4. Every WAP game/engine object derives (directly or through an
// intermediate base) from this 5-slot interface; its virtual table is the shared
// grand-base dtor vtable @0x5e8cb4 (named g_wapObjectDtorVtbl elsewhere). Deriving
// classes specify ONLY their own new virtuals + overrides - the five shared slots
// come from inheritance, never re-declared. cl auto-emits the derived ??_7 (the
// ctors stamp it vptr-first) and folds the grand-base vptr re-stamp (masks
// 0x5e8cb4) into the derived destructor, so no class needs a manual
// `*(void**)this = &g_*Vtbl` store.
//
// NAMESPACE: the game statically links MFC, whose global ::CObject would clash
// (C2011/C2872) with an unqualified engine CObject in the many MFC-including TUs.
// The engine grand-base therefore lives in `namespace Wap`. The Wap:: qualifier
// re-mangles the (declared-only, reloc-masked) vtable/method symbols but is
// matching-neutral: the class is used only as a base, its methods are external,
// and its emitted ??_7 is an unpaired COMDAT (0x1e8cb4 is named g_wapObjectDtorVtbl
// in the reconbatch2 unit - no VTBL() here).
//
// Slots 0/2/3/4 are the shared CObject-interface ILT jmp-thunks (0x1bef01 /
// 0x0028ec / 0x00106e / 0x004034); they stay declared-only so the emitted vtable's
// slot relocs reloc-mask against the retail addresses. Slot 1 is the scalar-
// deleting destructor. The FUN_<VA> slot names are the usual convention
// (cf. src/Gruntz/FinalVtables.cpp).
#ifndef WAP32_COBJECT_H
#define WAP32_COBJECT_H

namespace Wap {

    struct CObject {
        virtual void FUN_005bef01(); // slot 0  (0x1bef01)
        virtual ~CObject();          // slot 1  (base scalar-deleting dtor, 0x17f3 thunk)
        virtual void FUN_004028ec(); // slot 2  (0x0028ec)
        virtual void FUN_0040106e(); // slot 3  (0x00106e)
        virtual void FUN_00404034(); // slot 4  (0x004034)
        CObject() {}
    };
    // Empty body -> cl emits ONLY the implicit grand-base vptr re-stamp (masks 0x5e8cb4);
    // folded into a derived dtor it supplies the tail `mov [ecx],offset ??_7CObject`.
    inline CObject::~CObject() {}

} // namespace Wap

#endif // WAP32_COBJECT_H

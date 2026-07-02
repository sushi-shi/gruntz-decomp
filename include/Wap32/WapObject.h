// WapObject.h - CWapObject, the WAP engine's CObject-like grand-base.
//
// Every WAP game/engine object derives (directly or through an intermediate base)
// from this 5-slot interface; its virtual table is the shared grand-base dtor
// vtable @0x5e8cb4 (named g_wapObjectDtorVtbl elsewhere). Deriving classes specify
// ONLY their own new virtuals + overrides - the five shared slots come from
// inheritance, never re-declared. cl auto-emits the derived ??_7 (the ctors stamp
// it vptr-first) and folds the grand-base vptr re-stamp (masks 0x5e8cb4) into the
// derived destructor, so no class needs a manual `*(void**)this = &g_*Vtbl` store.
//
// Slots 0/2/3/4 are the shared CObject-interface ILT jmp-thunks (0x1bef01 /
// 0x0028ec / 0x00106e / 0x004034); they stay declared-only so the emitted vtable's
// slot relocs reloc-mask against the retail addresses. Slot 1 is the scalar-
// deleting destructor. No VTBL() here: 0x1e8cb4 is already named g_wapObjectDtorVtbl
// (reconbatch2 unit), so the cl-emitted ??_7CWapObject is an unpaired COMDAT
// (matching-neutral). The FUN_<VA> slot names are the usual convention
// (cf. src/Gruntz/FinalVtables.cpp).
#ifndef WAP32_WAPOBJECT_H
#define WAP32_WAPOBJECT_H

struct CWapObject {
    virtual void FUN_005bef01(); // slot 0  (0x1bef01)
    virtual ~CWapObject();       // slot 1  (base scalar-deleting dtor, 0x17f3 thunk)
    virtual void FUN_004028ec(); // slot 2  (0x0028ec)
    virtual void FUN_0040106e(); // slot 3  (0x00106e)
    virtual void FUN_00404034(); // slot 4  (0x004034)
    CWapObject() {}
};
// Empty body -> cl emits ONLY the implicit grand-base vptr re-stamp (masks 0x5e8cb4);
// folded into a derived dtor it supplies the tail `mov [ecx],offset ??_7CWapObject`.
inline CWapObject::~CWapObject() {}

#endif // WAP32_WAPOBJECT_H

// RezBufferObjectDtor.cpp - 0x17f330: the /GX destructor of a "DDraw worker"
// decode object. Stamp the most-derived vtable (0x5f07d8), free the +0x4 heap
// buffer, then (base subobject teardown) restamp the CObject base dtor vtable
// (0x5e8cb4). The destructible base subobject forces the /GX EH frame.
#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base
#include <rva.h>

// The most-derived vtable (0x5f07d8) is now the cl-emitted ??_7CRezBufferObject
// (VTBL below); the manual g_rezBufferObjectVtbl DATA-pin is gone. The CObject base
// dtor vtable (0x5e8cb4) is now restamped by the compiler-folded ~CObject (no
// manual g_wapObjectDtorVtbl reference remains here - the pin lives in ReconBatch2.cpp).

// The Rez heap free (0x1b9b82, __cdecl) the worker's +0x4 buffer is released
// through (reloc-masked rel32). C++ linkage (not extern "C") so MSVC5 treats it
// as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p);

// The CObject base subobject is CObject (Wap32/Object.h): empty dtor body; cl
// stamps ??_7Wap@@CObject (masks g_wapObjectDtorVtbl @0x5e8cb4) as the folded base.

// The worker: a +0x4 heap buffer freed on teardown.
struct CRezBufferObject : public CObject {
    char* m_4; // +0x04  heap buffer
    virtual ~CRezBufferObject() OVERRIDE;
    virtual void Serialize(CArchive& ar) OVERRIDE; // slot 2 (0x17f130, declared-only)
};

// ---------------------------------------------------------------------------
// 0x17f330 - ~CRezBufferObject (/GX): cl stamps the derived vptr (prologue), RezFree
// the +0x4 buffer, then folds the base subobject (restamps the base vptr). Real
// polymorphic hierarchy now -> the derived-vptr stamp is emitted in the prologue
// (before the m_4 load), matching retail's "stamp first".
// ---------------------------------------------------------------------------
RVA(0x0017f330, 0x51)
CRezBufferObject::~CRezBufferObject() {
    if (m_4) {
        RezFree(m_4);
    }
}
SIZE_UNKNOWN(CRezBufferObject);
VTBL(CRezBufferObject, 0x001f07d8); // ??_7CRezBufferObject@@6B@ (5-slot CObject-derived)
SIZE_UNKNOWN(CObject);
// ??_7CRezBufferObject (was g_rezBufferObjectVtbl @0x5f07d8, vtbl-cluster
// entry). cl auto-emits it from the real-polymorphic CRezBufferObject; retail's
// 5-slot datum is reloc-masked, so this VTBL is matching-neutral catalog tracking.

// RezBufferObjectDtor.cpp - 0x17f330: the /GX destructor of a "DDraw worker"
// decode object. Stamp the most-derived vtable (0x5f07d8), free the +0x4 heap
// buffer, then (base subobject teardown) restamp the CObject base dtor vtable
// (0x5e8cb4). The destructible base subobject forces the /GX EH frame.
#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base
#include <rva.h>
#include <Mfc.h>              // CArchive (Serialize's arg)
#include <Wap32/MfcArchive.h> // reloc-masked CArchive count/data accessor
#include <string.h>           // memset/memcpy -> rep stos/movs in the inlined SetSize
#include <new>                // placement new (ConstructElements' per-element ctor)
#include <Rez/RezBufferObject.h> // RezElem40 (the 0x28 CArray element type)

// The 40-byte (0x28) mesh-record element the CObArray holds. Its default ctor
// (0x17e300, reloc-masked, declared-only) is invoked per element by the inlined
// ConstructElements; keeping it a real ctor makes cl emit the `if(p) p->T::T()`
// placement-new guard the retail per-element loop shows.
// RezElem40 (the 0x28 CArray element) is modeled in <Rez/RezBufferObject.h>.

// The MFC ConstructElements<RezElem40>: zero the block, then default-construct each
// element. Out-of-line instance is 0x17e500 (reloc-masked; called by the grow path);
// cl inlines it into the in-place-grow path (memset + per-element ctor loop).
static inline void ConstructRezElems(RezElem40* p, i32 n) {
    memset(p, 0, n * sizeof(RezElem40));
    for (; n--; p++) {
        ::new ((void*)p) RezElem40;
    }
}

// The Rez heap alloc/free (operator new/delete): 0x1b9b46 / 0x1b9b82.
extern "C" void* RezAlloc(i32 n); // 0x1b9b46

// The most-derived vtable (0x5f07d8) is now the cl-emitted ??_7CRezBufferObject
// (VTBL below); the manual g_rezBufferObjectVtbl DATA-pin is gone. The CObject base
// dtor vtable (0x5e8cb4) is now restamped by the compiler-folded ~CObject (no
// manual g_wapObjectDtorVtbl reference remains here - the pin lives in ReconBatch2.cpp).

// The Rez heap free (0x1b9b82) the worker's +0x4 buffer is released through is the
// engine's ::operator delete (library ??3@YAXPAX@Z); C++ linkage keeps MSVC5's
// potentially-throwing treatment so the /GX base-subobject unwind frame stays. Was a
// fake `RezFree` decl whose ?RezFree@@YAXPAX@Z mangling matched neither the ??3 nor
// the _RezFree library label at 0x1b9b82, leaving the rel32 CALL reloc UNBOUND.

// The CObject base subobject is CObject (Wap32/Object.h): empty dtor body; cl
// stamps ??_7Wap@@CObject (masks g_wapObjectDtorVtbl @0x5e8cb4) as the folded base.

// CRezBufferObject (the CObArray of 40-byte mesh records) is now the shared canonical
// in <Rez/RezBufferObject.h>.

// ---------------------------------------------------------------------------
// 0x17f330 - ~CRezBufferObject (/GX): cl stamps the derived vptr (prologue), RezFree
// the +0x4 buffer, then folds the base subobject (restamps the base vptr). Real
// polymorphic hierarchy now -> the derived-vptr stamp is emitted in the prologue
// (before the m_pData load), matching retail's "stamp first".
// ---------------------------------------------------------------------------
RVA(0x0017f330, 0x51)
CRezBufferObject::~CRezBufferObject() {
    if (m_pData) {
        ::operator delete(m_pData);
    }
}

// ---------------------------------------------------------------------------
// 0x17f130 - CRezBufferObject::Serialize (slot 2): the MFC CArray<40-byte>::Serialize
// (SetSize inlined). Storing: WriteCount then Write the raw 0x28-stride block; loading:
// ReadCount, resize (alloc / grow-with-copy / shrink-in-place per the m_nSize/8 grow
// heuristic) and ConstructElements the new records, then Read the block. Twin of
// CFaderArray::Serialize (0x17e2a0) but with a non-trivial 0x28-byte element.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (serialize family, ~same as CFaderArray/TArray): the branch senses,
// SetSize load/grow/shrink logic, the 0x28 element stride, ConstructElements
// (inlined in-place / called on grow) and Read/Write are byte-faithful, but retail
// pins this->ebx / ar->esi while cl lands different colours through the realloc lea
// chains. Not source-steerable.
RVA(0x0017f130, 0x1ce)
void CRezBufferObject::Serialize(CArchive& ar) {
    MfcArchive* a = (MfcArchive*)&ar;
    if (a->IsStoring()) {
        a->WriteCount(m_nSize);
    } else {
        i32 n = a->ReadCount();
        if (n == 0) {
            if (m_pData != 0) {
                ::operator delete(m_pData);
                m_pData = 0;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == 0) {
            m_pData = (RezElem40*)RezAlloc(n * sizeof(RezElem40));
            ConstructRezElems(m_pData, n);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                ConstructRezElems(&m_pData[m_nSize], n - m_nSize);
            }
            m_nSize = n;
        } else {
            i32 grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newMax = m_nMaxSize + grow;
            if (n >= newMax) {
                newMax = n;
            }
            RezElem40* nd = (RezElem40*)RezAlloc(newMax * sizeof(RezElem40));
            memcpy(nd, m_pData, m_nSize * sizeof(RezElem40));
            ConstructRezElems(&nd[m_nSize], n - m_nSize);
            ::operator delete(m_pData);
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newMax;
        }
    }
    if (a->IsStoring()) {
        a->WriteData(m_pData, m_nSize * sizeof(RezElem40));
    } else {
        a->ReadData(m_pData, m_nSize * sizeof(RezElem40));
    }
}
// SIZE_UNKNOWN(CRezBufferObject) now lives with the class in <Rez/RezBufferObject.h>.
VTBL(CRezBufferObject, 0x001f07d8); // ??_7CRezBufferObject@@6B@ (5-slot CObject-derived)
SIZE_UNKNOWN(CObject);
// ??_7CRezBufferObject (was g_rezBufferObjectVtbl @0x5f07d8, vtbl-cluster
// entry). cl auto-emits it from the real-polymorphic CRezBufferObject; retail's
// 5-slot datum is reloc-masked, so this VTBL is matching-neutral catalog tracking.

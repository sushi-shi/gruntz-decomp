// SeverusWorkerDtor.cpp - 0x17f330: the /GX destructor of a "severus worker"
// decode object. Stamp the most-derived vtable (0x5f07d8), free the +0x4 heap
// buffer, then (base subobject teardown) restamp the CObject base dtor vtable
// (0x5e8cb4). The destructible base subobject forces the /GX EH frame.
#include <Ints.h>
#include <rva.h>

// The most-derived vtable (VA 0x5f07d8) and the CObject base dtor vtable
// (VA 0x5e8cb4, g_remusBaseDtorVtbl, pinned in many TUs). Both reloc-masked.
DATA(0x001f07d8)
extern void* g_severusWorkerVtbl;
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl;

// The Rez heap free (0x1b9b82, __cdecl) the worker's +0x4 buffer is released
// through (reloc-masked rel32). C++ linkage (not extern "C") so MSVC5 treats it
// as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p);

// The CObject base subobject, modeled polymorphically: empty dtor body; cl stamps
// ??_7SeverusWorkerBase (masks g_remusBaseDtorVtbl @0x5e8cb4) as the folded base.
struct SeverusWorkerBase {
    virtual ~SeverusWorkerBase(); // implicit vptr @ +0x00
};
inline SeverusWorkerBase::~SeverusWorkerBase() {}

// The worker: a +0x4 heap buffer freed on teardown.
struct CSeverusWorkerX : SeverusWorkerBase {
    char* m_4; // +0x04  heap buffer
    ~CSeverusWorkerX();
};

// ---------------------------------------------------------------------------
// 0x17f330 - ~CSeverusWorkerX (/GX): cl stamps the derived vptr (prologue), RezFree
// the +0x4 buffer, then folds the base subobject (restamps the base vptr). Real
// polymorphic hierarchy now -> the derived-vptr stamp is emitted in the prologue
// (before the m_4 load), matching retail's "stamp first".
// ---------------------------------------------------------------------------
RVA(0x0017f330, 0x51)
CSeverusWorkerX::~CSeverusWorkerX() {
    if (m_4) {
        RezFree(m_4);
    }
}

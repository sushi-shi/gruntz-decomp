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

// The CObject base subobject: its dtor restamps the base vptr (0x5e8cb4) last.
struct SeverusWorkerBase {
    void* m_vptr; // +0x00
    ~SeverusWorkerBase() {
        m_vptr = &g_remusBaseDtorVtbl;
    }
};

// The worker: a +0x4 heap buffer freed on teardown.
struct CSeverusWorkerX : SeverusWorkerBase {
    char* m_4; // +0x04  heap buffer
    ~CSeverusWorkerX();
};

// ---------------------------------------------------------------------------
// 0x17f330 - ~CSeverusWorkerX (/GX): stamp the derived vptr, RezFree the +0x4
// buffer, then (base subobject) restamp the base vptr.
// ---------------------------------------------------------------------------
// @early-stop
// /GX eh-dtor vptr-stamp scheduling wall (87.8%): the EH prologue, the trylevel=0
// write, the `if(m_4) RezFree(m_4)` guard+free, and both vptr stamps are all
// byte-faithful; the only residual is that retail schedules the derived-vptr
// stamp as the FIRST body op (before the m_4 load) while cl emits it between the
// `test`/`je` of the guard - the independent [esi] store vs [esi+4] load are
// freely reordered by the /GX scheduler (eh-dtor-vptr-stamp-vs-trylevel-order.md
// family, cf. InterfaceObject::~InterfaceObject). Logic 100% correct; deferred.
RVA(0x0017f330, 0x51)
CSeverusWorkerX::~CSeverusWorkerX() {
    *(void**)this = &g_severusWorkerVtbl;
    if (m_4) {
        RezFree(m_4);
    }
}

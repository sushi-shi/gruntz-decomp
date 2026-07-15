// TypeCollRuntime.cpp - CTypeCollRuntime::ScalarDelete (0x16ea20), the zDArray<CString>
// runtime collection's `??_G` scalar-deleting destructor. Retail inlined the real
// dtor into the deleting thunk, so the whole body lives here as one method: read the
// element buffer, stamp the CTypeCollRuntime vptr, destruct each CString, run the
// ~zDArray base teardown (0x16df40, reloc-masked), then conditionally RezFree.
#include <Gruntz/TypeCollRuntime.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <rva.h>

#include <Mfc.h> // CString (element type; ~CString @0x1b9cde)

// operator delete / RezFree (0x1b9b82, __cdecl one-arg); reloc-masked.

// ??_7CTypeCollRuntime@@6B@ (0x5f04e4) - the vtable the dtor re-stamps at entry
// (reloc-masked address operand; the class's own virtuals live in unmatched TUs).
DATA(0x001f04e4)
extern void* const CTypeCollRuntime_vtbl;

// @early-stop
// count-materialization residue (~87.5%). Full body + control flow + all reloc-masked
// calls (~CString @0x1b9cde per element, ~zDArray @0x16df40, RezFree, the vptr stamp)
// are byte-exact. Residual: retail materializes the loop count verbosely - keeps
// (m_hi-m_lo) in eax, copies count into ecx for the `test`, then rebuilds the edi loop
// counter via `dec eax; lea edi,[eax+1]`; this wine MSVC5 folds the count straight into
// edi (`inc edi; je`), 3 fewer insns. Not source-steerable (do-while / for / split-add
// all fold the same way) and the permuter finds no operand-order win.
RVA(0x0016ea20, 0x51)
void* CTypeCollRuntime::ScalarDelete(u32 flags) {
    CString* p = (CString*)m_base; // +0x10
    *(void**)this = (void*)&CTypeCollRuntime_vtbl;
    if (p) {
        i32 count = m_hi - m_lo + 1; // +0x0c - +0x08 + 1
        if (count != 0) {
            i32 i = count;
            do {
                p->~CString();
                p++;
            } while (--i);
        }
    }
    this->zDArray::~zDArray(); // 0x16df40 base teardown
    if (flags & 1) {
        RezFree(this);
    }
    return this;
}

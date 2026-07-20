#include <Gruntz/TypeCollRuntime.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <rva.h>

#include <Mfc.h> // CString (element type; ~CString @0x1b9cde)

DATA(0x001f04e4)
extern void* const CTypeCollRuntime_vtbl;

// @early-stop
// count-materialization residue (~87.5%). Full body + control flow + all reloc-masked
// calls (~CString @0x1b9cde per element, ~_zdvec @0x16df40, RezFree, the vptr stamp)
// are byte-exact. Residual: retail materializes the loop count verbosely - keeps
// (m_hi-m_lo) in eax, copies count into ecx for the `test`, then rebuilds the edi loop
// counter via `dec eax; lea edi,[eax+1]`; this wine MSVC5 folds the count straight into
// edi (`inc edi; je`), 3 fewer insns. Not source-steerable (do-while / for / split-add
// all fold the same way) and the permuter finds no operand-order win.
RVA(0x0016ea20, 0x51)
void* CTypeCollRuntime::ScalarDelete(u32 flags) {
    CString* p = reinterpret_cast<CString*>(m_base); // +0x10
    *reinterpret_cast<void**>(this) = const_cast<void**>(&CTypeCollRuntime_vtbl);
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
    this->_zdvec::~_zdvec(); // 0x16df40 base teardown
    if (flags & 1) {
        RezFree(this);
    }
    return this;
}

#include <Gruntz/TypeCollRuntime.h> // CTypeCollRuntime (+ VTBL)
#include <Rez/RezAlloc.h>           // RezAlloc/RezFree
#include <rva.h>

#include <Mfc.h> // CString (element type; ~CString @0x1b9cde)

// The real destructor (~CTypeCollRuntime, ??1) - the one member the devs wrote: cl
// stamps the ??_7CTypeCollRuntime vptr, destructs the m_base CString array, then
// auto-chains ~_zdvec (0x16df40). Defining it here makes this the vtable's key TU, so
// cl emits the ??_7 + the compiler-generated `??_G' scalar-deleting destructor (pinned
// at 0x16ea20 below). The out-of-line ??1 itself lands in an as-yet-unreconstructed TU,
// so it is not RVA-bound here.
CTypeCollRuntime::~CTypeCollRuntime() {
    CString* p = reinterpret_cast<CString*>(m_base); // +0x10
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
}

// 0x16ea20 - the cl-generated `??_G' scalar-deleting destructor.
// @early-stop
// structural: retail's typecollruntime TU INLINES the dtor into this ??_G (the class was
// constructed there); our reconstruction has only the isolated ??_G COMDAT, so cl emits
// it CALLING the out-of-line ??1 instead of inlining. Recovers to ~87.5% once the real
// construction TU (which makes the dtor inline-visible) is reconstructed.
RVA_COMPGEN(0x0016ea20, 0x51, ??_GCTypeCollRuntime@@UAEPAXI@Z)

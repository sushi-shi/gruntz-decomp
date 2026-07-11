// ZVec.cpp - the WAP32 `_zvec`/`zDArray<T>` dynamic-vector base (C:\Proj\incs).
// See include/Wap32/ZVec.h for the recovered layout + the RTTI-confirmed
// hierarchy  zDArray<int (CUserLogic::*)(void)> : _zdvec : _zvec : zErrHandling.
//
// REAL-POLYMORPHIC now: the virtual dtor lets cl emit ??_7zDArray and auto-stamp
// the implicit vptr at ~zDArray entry (VTBL binds it at the retail dtor-vtable RVA).
// The one manual store that survives is Destroy()'s re-stamp of the LIVE vtable
// (0x5e70fc) - a NON-ctor/dtor re-stamp cl cannot express (the vtable-realization-
// ctor-boundary wall), so it stays as an explicit `*(void**)this` store of the
// reloc-masked live-vtable datum.
#include <Wap32/ZVec.h>
#include <rva.h>
#include <Mfc.h> // CString (0x1b9b93 default ctor)
#include <new>   // placement CString ctor
#include <Globals.h>
#include <Bute/ButeTree.h>

#include <stdlib.h> // realloc (0x125180), free (0x120c30)
#include <string.h> // memcpy (0x121960), memset (rep stos)

// The live zDArray<...> vtable (0x5e70fc) Destroy() re-stamps (reloc-masked address
// operand). ~zDArray (0x16df40) and its emitted ??_7zDArray + the VTBL binding
// moved to the merged container TU (src/Gruntz/TypeKeyColl.cpp, wave2-H) along
// with _zvec::GrowTo (0x16da80) - their retail home.
DATA(0x001e70fc)
extern void* const zDArrayLiveTable; // 0x5e70fc

// Engine return-address capture helper that seeds the error token.
extern void* GetRetAddr(); // 0x16d990 (pop eax;push eax;ret: the call-site return addr)

// Per-element relocation applied to each freshly-grown member-pointer slot
// (a __thiscall on the slot: ecx=slot, no stack cleanup). 0x1b9b93.

// ---------------------------------------------------------------------------
// zDArray::Destroy() - re-stamp the live vtable, then run ~zDArray. 0x8750.
// @early-stop
// 21B dead-store oddity: retail reserves a stack slot (push ecx; mov [esp],
// m_base) for a discarded local then `call ~zDArray`; cl folds our local into a
// callee-saved reg (i32 form) or tail-jmps (void form). The spilled-but-unread
// m_base local is not source-recoverable. Logic (re-stamp + run ~zDArray) exact.
RVA(0x00008750, 0x15)
i32 zDArray::Destroy() {
    i32 tmp = m_base;
    *(void**)this = (void*)&zDArrayLiveTable; // re-stamp LIVE vtable (non-dtor wall)
    this->~zDArray();
    return tmp;
}

// zDArray::IndexToPtr(i) - the base accessor plus the per-slot member-ptr fixup
// over the freshly-grown region. 0x310f0.
RVA(0x000310f0, 0x8d)
i32 zDArray::IndexToPtr(i32 i) {
    i32 r;
    m_grown = 0;
    if (i >= m_lo && i <= m_hi) {
        r = m_base + (i - m_lo) * m_stride;
    } else if (GrowTo(i, 0)) {
        r = m_base + (i - m_lo) * m_stride;
    } else {
        i32 sentinel = g_zvecErrSentinel;
        g_retAddrBreadcrumb = GetRetAddr();
        m_err->Set((void*)this, sentinel, 0xc);
        r = m_spare;
    }
    char* slot = (char*)m_alloc;
    i32 n = m_grown;
    while (n-- != 0) {
        if (slot) {
            new ((void*)slot) CString();
        }
        slot += 4;
    }
    return r;
}

// _zvec::IndexToPtr(idx) - the plain accessor; grows on a bounds miss. 0x312a0.
// @early-stop
// regalloc wall: retail pins idx in esi / this in edi (arg-before-this) and
// merges the in-range + grow-success offset tails; our recompile mirrors the
// esi/edi assignment and duplicates the tail, ~83%. Logic exact. The derived
// override (0x310f0) matches because its trailing fixup loop shifts the reg
// pressure; the loop-less base accessor does not flip. Not source-steerable.
RVA(0x000312a0, 0x74)
i32 _zvec::IndexToPtr(i32 idx) {
    i32 lo = m_lo;
    m_grown = 0;
    if (idx >= lo && idx <= m_hi) {
        idx -= lo;
        idx *= m_stride;
        return idx + m_base;
    }
    if (GrowTo(idx, 0)) {
        i32 base = m_base;
        idx -= m_lo;
        idx *= m_stride;
        return idx + base;
    }
    i32 sentinel = g_zvecErrSentinel;
    g_retAddrBreadcrumb = GetRetAddr();
    m_err->Set((void*)this, sentinel, 0xc);
    return m_spare;
}

// ZVec.h + local class metadata (hosted at .cpp EOF; header untouched).
SIZE_UNKNOWN(_zvec);          // dynamic-vector base (partial: no true base chain)
SIZE_UNKNOWN(zDArray);        // derived; adds override, no storage
SIZE_UNKNOWN(zErrHandling);   // error-reporter subobject view
SIZE_UNKNOWN(zMemberPtrSlot); // member-ptr slot fixup view

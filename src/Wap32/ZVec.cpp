// ZVec.cpp - the WAP32 `_zvec`/`zDArray<T>` dynamic-vector base (C:\Proj\incs).
// See include/Wap32/ZVec.h for the recovered layout + the RTTI-confirmed
// hierarchy  zDArray<int (CUserLogic::*)(void)> : _zdvec : _zvec : zErrHandling.
//
// The class is polymorphic but its full vtable contents live in other TUs, so
// the vptr stores are modeled as manual stamps of the retail vtables (the
// transitional workaround; reloc-masked DATA() externs) rather than letting the
// compiler emit a divergent ??_7.
#include <Wap32/ZVec.h>
#include <rva.h>

// The retail vtables this cluster stamps (reloc-masked address operands).
DATA(0x001e70fc)
extern void* const g_zDArrayVtbl; // 0x5e70fc  live zDArray<...> vtable
DATA(0x001f04d4)
extern void* const g_zDArrayDtorVtbl; // 0x5f04d4  ~zDArray entry vtable

// Externs the cluster reaches (reloc-masked rel32 callees).
extern "C" void* realloc(void* p, u32 n);               // 0x125180
extern "C" void* memcpy(void* d, const void* s, u32 n); // 0x121960
extern "C" void* memset(void* d, i32 c, u32 n);         // (inlined to rep stos)
extern "C" void free(void* p);                          // 0x120c30

// The grow path calls memcpy out-of-line (not the rep-movs intrinsic).
#pragma function(memcpy)

// Two engine return-address capture helpers that seed the error token.
extern void* zErr_CaptureRetA(); // 0x16e0f0 (mov eax,[ebp+4])
extern void* zErr_CaptureRetB(); // 0x16d990 (pop/push/ret)

// Per-element relocation applied to each freshly-grown member-pointer slot
// (a __thiscall on the slot: ecx=slot, no stack cleanup). 0x1b9b93.
struct zMemberPtrSlot {
    void Init(); // 0x1b9b93
};

// The error-report globals + the "out of memory" message.
extern u32 g_zvecErrSentinel; // 0x6bf464
extern void* g_zvecErrToken;  // 0x6bf428
DATA(0x0021adf4)
extern const char s_out_of_memory[]; // 0x61adf4

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
    m_vptr = (void*)&g_zDArrayVtbl;
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
        g_zvecErrToken = zErr_CaptureRetB();
        m_err->Error(this, sentinel, 0xc);
        r = m_spare;
    }
    char* slot = (char*)m_alloc;
    i32 n = m_grown;
    while (n-- != 0) {
        if (slot) {
            ((zMemberPtrSlot*)slot)->Init();
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
    g_zvecErrToken = zErr_CaptureRetB();
    m_err->Error(this, sentinel, 0xc);
    return m_spare;
}

// _zvec::GrowTo(idx, at) - realloc the element band so index `idx` (relative to
// the `at` anchor) becomes addressable, shift/zero-fill, update bounds. 0x16da80.
// @early-stop
// regalloc wall (~80%): retail pins idx in ebx / this in esi / realloc result
// in ebp (arg-before-this); our recompile assigns idx->ebp / this->ebx, which
// cascades into the two-block branch distances. `#pragma function(memcpy)`
// recovered the out-of-line memcpy call (62%->80%); the residual is the
// register assignment, not source-steerable. Logic exact.
RVA(0x0016da80, 0x10b)
void* _zvec::GrowTo(i32 idx, i32 at) {
    void* p;
    if (idx < m_lo) {
        p = realloc((void*)m_base, (m_hi - (idx - at) + 1) * m_stride);
        if (!p) {
            g_zvecErrToken = zErr_CaptureRetA();
            m_err->Error(this, (u32)s_out_of_memory, 0x22);
            return 0;
        }
        i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
        i32 shift = m_lo - (idx - at);
        m_grown = shift;
        m_alloc = (i32)p;
        memcpy((char*)p + shift * m_stride, p, oldbytes);
        memset((char*)m_alloc, 0, m_grown * m_stride);
        m_lo = idx - at;
        m_base = (i32)p;
        return p;
    }
    i32 hinew = idx + at;
    p = realloc((void*)m_base, (hinew - m_lo + 1) * m_stride);
    if (!p) {
        g_zvecErrToken = zErr_CaptureRetA();
        m_err->Error(this, (u32)s_out_of_memory, 0x22);
        return 0;
    }
    i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
    char* fill = (char*)p + oldbytes;
    m_grown = hinew - m_hi;
    m_alloc = (i32)fill;
    memset(fill, 0, m_grown * m_stride);
    m_hi = hinew;
    m_base = (i32)p;
    return p;
}

// zDArray::~zDArray() - re-stamp the derived dtor vtable, free the band, chain
// to the base dtor. 0x16df40.
RVA(0x0016df40, 0x22)
zDArray::~zDArray() {
    i32 p = m_base;
    m_vptr = (void*)&g_zDArrayDtorVtbl;
    if (p) {
        free((void*)p);
    }
    // ~_zvec() base destructor is chained in by the compiler (mov ecx,esi; call).
}

// ZVec.h + local class metadata (hosted at .cpp EOF; header untouched).
SIZE_UNKNOWN(_zvec);          // dynamic-vector base (partial: no true base chain)
SIZE_UNKNOWN(zDArray);        // derived; adds override, no storage
SIZE_UNKNOWN(zErrHandling);   // error-reporter subobject view
SIZE_UNKNOWN(zMemberPtrSlot); // member-ptr slot fixup view

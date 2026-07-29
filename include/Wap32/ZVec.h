#ifndef GRUNTZ_WAP32_ZVEC_H
#define GRUNTZ_WAP32_ZVEC_H

#include <Ints.h>
#include <Wap32/zBitVec.h> // the canonical zErrHandling - _zvec's real {vptr, sink} head
#include <rva.h>
#include <AddrWord.h> // the immediate-in-a-pointer-slot sentinel

struct CVariantSlot; // fwd (pointer member m_err; full def at the overflow call sites)

// The UNTYPED byte vector. Its element pointer is `m_base + (idx - m_lo) * m_stride`
// with m_stride read from the object at RUN TIME (0x312c0 `imul esi,[edi+0x18]`), so
// `char*` is not a placeholder here - it is the only return type consistent with the
// instructions. Typed element access is the CALLER's job (see _zdvec / zDArray<T>);
// there is nothing to "type" on this class.
// The "band is never overflowed, do not allocate a scratch element" sentinel both
// _zdvec ctor call sites hand down. It lands in m_spare at 0x16de65 and the only
// thing that ever looks at it is `if (m_spare != 0)` at 0x16decd.
inline void* ZVecNoScratch() {
    // bare imm: retail pushes the literal 1 (zDArray's ctor @0x8717); never
    // dereferenced, so no object exists to point at - this is the whole sentinel,
    // an immediate riding a pointer slot (<AddrWord.h>).
    AddrWord sentinel;
    sentinel.m_word = 1;
    return sentinel.m_addr;
}

class _zvec : public zErrHandling {
public:
    // 0x16de30: allocate the [lo, hi] element band and scratch slot.
    _zvec(i32 stride, i32 lo, i32 hi, void* scratch);

    void* GrowTo(i32 idx, i32 at); // 0x16da80
    char* IndexToPtr(i32 idx);     // 0x312a0  (the plain base accessor)
    virtual ~_zvec() OVERRIDE;     // 0x16df40

    // vptr @+0x00 and the error sink @+0x04 come from zErrHandling (which names the
    // sink m_errSink; this class's code still reads it under that name).
    i32 m_lo;      // +0x08
    i32 m_hi;      // +0x0c
    char* m_base;  // +0x10  element band (byte-addressed: base + (idx-lo)*stride)
    char* m_spare; // +0x14  scratch element / error-path result
    i32 m_stride;  // +0x18
    char* m_alloc; // +0x1c  raw realloc base / per-element construction cursor
    i32 m_grown;   // +0x20  number of elements constructed by the last grow
};
SIZE(0x24);

// The CString-element vector. Same address math as the base, plus a fixup loop over
// the slots GrowTo just added. The loop is what pins the element type: 0x31156..0x31173
// walks `m_alloc` in steps of a CONSTANT 4 (not m_stride) for `m_grown` iterations and
// calls 0x1b9b93 == ??0CString@@QAE@XZ on each non-null slot. A 4-byte element with a
// CString default ctor IS a CString, so _zdvec is one concrete instantiation, not a
// generic. The same body appears INLINED at call sites that read the two globals
// directly (CInGameText::Update @0x997c0: `call <_zvec::IndexToPtr>` then
// `mov esi,[0x6bf66c]` / `mov eax,[0x6bf670]` == g_typeColl.m_alloc / .m_grown, then
// the identical `call ??0CString` loop) - so this is an inline/header member whose
// out-of-line COMDAT copy lives at 0x310f0.
//
// It still returns char*: the address math is the base's byte math (runtime m_stride),
// and the element type is reapplied by the caller (CTypeCollRuntime::SlotOf etc.).
//
// Both accessors are header-INLINE members that also got out-of-line COMDAT copies,
// which is why they show up three different ways in the image and why so many callers
// carry an "inlined IndexToPtr regalloc wall" @early-stop:
//   * 0x310f0 - the derived COMDAT, with the BASE body inlined into it (its callees are
//     GrowTo/GetRetAddr/Set/??0CString - it never calls 0x312a0);
//   * 0x312a0 - the base COMDAT alone (same callees minus ??0CString);
//   * fully expanded at a call site - CInGameText::Update @0x997c0 inlines the derived
//     member but spends its inline budget before the nested base one, so it emits
//     `call <0x312a0>` followed by the CString loop over the two globals.
class _zdvec : public _zvec {
public:
    // 0x16dda0: construct the allocating base, then seed the element cursor/count.
    _zdvec(i32 stride, i32 lo, i32 hi, void* scratch);
    char* IndexToPtr(i32 i); // 0x310f0 (base accessor + per-slot CString construction)
    // ~_zdvec is IMPLICIT: retail 0x16de00 is a bare 5-byte `jmp ??1_zvec` with NO
    // vptr restamp - only the compiler-generated trivial dtor produces that form.
};
SIZE(0x24);

template<class T> class zDArray : public _zdvec {
public:
    zDArray(i32 lo, i32 hi);
    virtual ~zDArray() OVERRIDE;

    T* Resolve(i32 id);      // out-of-line; the typed element slot
    T* ResolveEntry(i32 id); // the typed element slot
    // Same accessor, one inline level shallower: the grow-fail tail is left as the
    // out-of-line zErrHandling::Report call instead of expanded. cl5 spends its inline
    // budget from the OUTSIDE in - a one-key registrar (0x18d) expands the tail at both
    // of its lookups, a two-key registrar (0x2ac) keeps it outlined at three of four,
    // and the 19-key registrar outlines the whole accessor (`Resolve`).
    // docs/patterns/act-registrar-report-outline-budget.md
    T* ResolveEntryCallReport(i32 id);

    // THE one seam of the typed container. _zvec addresses its band by a RUNTIME
    // m_stride (0x312c0 `imul esi,[edi+0x18]`) - byte-forced, so the element type
    // can only go back on here; every typed accessor routes through this one line.
    static T* AsElem(char* p) {
        return reinterpret_cast<T*>(p);
    }
};
SIZE_UNKNOWN();

#endif // GRUNTZ_WAP32_ZVEC_H

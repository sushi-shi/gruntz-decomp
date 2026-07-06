// BoundaryTailEh.cpp - the /GX EH-frame siblings of BoundaryTail.cpp: the hard
// CString/CByteArray-heavy class-boundary leaf methods whose destructible stack
// local/member forces the MSVC5 /GX exception frame. Owning class names are
// placeholders; only OFFSETS + code shape are load-bearing.
#include <Mfc.h> // real MFC CString (embedded name member; ~CString @0x1b9cde)
#include <rva.h>

// The retail CFileMemBase base vtable (0x5efe68); bound by Io/FileMem.cpp.

// ---------------------------------------------------------------------------
// 0x1578b0 - CFileMemBase::~CFileMemBase: stamp the base vtable, call ResetBase
// (base vtable slot +0xc), then destruct the CString name member on unwind.
// @early-stop
// manual-vtable EH-dtor wall (same as CFileMem::~CFileMem @0x157980 in
// Io/FileMem.cpp, ~59%): retail dispatches ResetBase as `call ds:[vtbl+0xc]` -- a
// __thiscall fn-ptr through the absolute vtable slot 0x5efe74 -- which MSVC5
// cannot spell (a __thiscall fn-ptr typedef is C4234; a __cdecl slot ptr would
// push/clean `this`). A direct member call keeps the logic exact but emits a
// `call rel32` instead, so the dispatch byte + the EH trylevel store sequencing
// diverge. See docs/patterns/eh-dtor-inline-member-vtable-stamp-thisadjust.md.
// ---------------------------------------------------------------------------
struct CFileMemBase {
    void* m_vtbl;     // +0x0
    char _4[0xc - 4]; // +0x4,+0x8 scalars
    CString m_name;   // +0xc
    void ResetBase(); // 0x157a40 (base vtable slot +0xc)
    ~CFileMemBase();
};
SIZE_UNKNOWN(CFileMemBase);
RVA(0x001578b0, 0x51)
CFileMemBase::~CFileMemBase() {
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    ResetBase();
}
